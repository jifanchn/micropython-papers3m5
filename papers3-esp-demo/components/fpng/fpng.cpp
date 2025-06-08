#include <cstdlib>
#include <cstring>
#include <stdio.h>

#include <assert.h>
#include "esp_heap_caps.h"
void convert_rgb888_to_rgb565(uint8_t *p, uint32_t len)
{
    int idx = 0;
    uint16_t c = 0;
    uint16_t *p16 = (uint16_t*)p;
    
    for (int i = 0; i < len; i += 3) {
        p16[idx] = (p[i + 0] << 8 & 0xf800) | (p[i + 1] << 3 & 0x07e0) | (p[i + 2] >> 3 & 0x001f);
        idx++;
    }
}

uint32_t *clen_table = NULL;
uint32_t *lit_table = NULL;

template <typename S>
static inline S maximum(S a, S b) { return (a > b) ? a : b; }
template <typename S>
static inline S minimum(S a, S b) { return (a < b) ? a : b; }

static inline uint32_t simple_swap32(uint32_t x) { return (x >> 24) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | (x << 24); }

static inline uint32_t swap32(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(x);
#else
    return simple_swap32(x);
#endif
}

#define READ_LE32(p) (*reinterpret_cast<const uint32_t *>(p))
#define WRITE_LE32(p, v) *reinterpret_cast<uint32_t *>(p) = (uint32_t)(v)
#define WRITE_LE64(p, v) *reinterpret_cast<uint64_t *>(p) = (uint64_t)(v)

#define READ_BE32(p) swap32(*reinterpret_cast<const uint32_t *>(p))

static const uint32_t g_bitmasks[17] = {0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF};

#define ENSURE_32BITS()                                                       \
    do                                                                        \
    {                                                                         \
        if (bit_buf_size < 32)                                                \
        {                                                                     \
            if ((src_ofs + 4) > src_len)                                      \
                return false;                                                 \
            bit_buf |= ((uint64_t)READ_LE32(pSrc + src_ofs)) << bit_buf_size; \
            src_ofs += 4;                                                     \
            bit_buf_size += 32;                                               \
        }                                                                     \
    } while (0)

#define GET_BITS(b, ll)                          \
    do                                           \
    {                                            \
        uint32_t l = ll;                         \
        assert(l && (l <= 32));                  \
        b = (uint32_t)(bit_buf & g_bitmasks[l]); \
        bit_buf >>= l;                           \
        bit_buf_size -= l;                       \
        ENSURE_32BITS();                         \
    } while (0)

#define SKIP_BITS(ll)      \
    do                     \
    {                      \
        uint32_t l = ll;   \
        assert(l <= 32);   \
        bit_buf >>= l;     \
        bit_buf_size -= l; \
        ENSURE_32BITS();   \
    } while (0)


#define GET_BITS_NE(b, ll) do { \
	uint32_t l = ll; assert(l && (l <= 32) && (bit_buf_size >= l)); \
	b = (uint32_t)(bit_buf & g_bitmasks[l]); \
	bit_buf >>= l; \
	bit_buf_size -= l; \
	} while(0)

#define SKIP_BITS_NE(ll) do { \
	uint32_t l = ll; assert(l <= 32 && (bit_buf_size >= l)); \
	bit_buf >>= l; \
	bit_buf_size -= l; \
	} while(0)

enum
{
    FPNG_DECODE_SUCCESS = 0, // file is a valid PNG file and written by FPNG and the decode succeeded

    FPNG_DECODE_NOT_FPNG, // file is a valid PNG file, but it wasn't written by FPNG so you should try decoding it with a general purpose PNG decoder

    FPNG_DECODE_INVALID_ARG, // invalid function parameter

    FPNG_DECODE_FAILED_NOT_PNG,              // file cannot be a PNG file
    FPNG_DECODE_FAILED_HEADER_CRC32,         // a chunk CRC32 check failed, file is likely corrupted or not PNG
    FPNG_DECODE_FAILED_INVALID_DIMENSIONS,   // invalid image dimensions in IHDR chunk (0 or too large)
    FPNG_DECODE_FAILED_DIMENSIONS_TOO_LARGE, // decoding the file fully into memory would likely require too much memory (only on 32bpp builds)
    FPNG_DECODE_FAILED_CHUNK_PARSING,        // failed while parsing the chunk headers, or file is corrupted
    FPNG_DECODE_FAILED_INVALID_IDAT,         // IDAT data length is too small and cannot be valid, file is either corrupted or it's a bug

    // fpng_decode_file() specific errors
    FPNG_DECODE_FILE_OPEN_FAILED,
    FPNG_DECODE_FILE_TOO_LARGE,
    FPNG_DECODE_FILE_READ_FAILED,
    FPNG_DECODE_FILE_SEEK_FAILED
};

enum
{
    DEFL_MAX_HUFF_TABLES = 3,
    DEFL_MAX_HUFF_SYMBOLS = 288,
    DEFL_MAX_HUFF_SYMBOLS_0 = 288,
    DEFL_MAX_HUFF_SYMBOLS_1 = 32,
    DEFL_MAX_HUFF_SYMBOLS_2 = 19,
    DEFL_LZ_DICT_SIZE = 32768,
    DEFL_LZ_DICT_SIZE_MASK = DEFL_LZ_DICT_SIZE - 1,
    DEFL_MIN_MATCH_LEN = 3,
    DEFL_MAX_MATCH_LEN = 258
};

static const int s_length_extra[] = { 0,0,0,0, 0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3, 4,4,4,4, 5,5,5,5, 0,    0,0 };
	static const int s_length_range[] = { 3,4,5,6, 7,8,9,10, 11,13,15,17, 19,23,27,31, 35,43,51,59, 67,83,99,115, 131,163,195,227, 258,    0,0 };
static const uint16_t g_run_len3_to_4[259] = 
	{
		0,
		0, 0, 4, 0, 0, 8, 0, 0, 12, 0, 0, 16, 0, 0, 20, 0, 0, 24, 0, 0, 28, 0, 0,
		32, 0, 0, 36, 0, 0, 40, 0, 0, 44, 0, 0, 48, 0, 0, 52, 0, 0, 56, 0, 0,
		60, 0, 0, 64, 0, 0, 68, 0, 0, 72, 0, 0, 76, 0, 0, 80, 0, 0, 84, 0, 0,
		88, 0, 0, 92, 0, 0, 96, 0, 0, 100, 0, 0, 104, 0, 0, 108, 0, 0, 112, 0, 0,
		116, 0, 0, 120, 0, 0, 124, 0, 0, 128, 0, 0, 132, 0, 0, 136, 0, 0, 140, 0, 0,
		144, 0, 0, 148, 0, 0, 152, 0, 0, 156, 0, 0, 160, 0, 0, 164, 0, 0, 168, 0, 0,
		172, 0, 0, 176, 0, 0, 180, 0, 0, 184, 0, 0, 188, 0, 0, 192, 0, 0, 196, 0, 0,
		200, 0, 0, 204, 0, 0, 208, 0, 0, 212, 0, 0, 216, 0, 0, 220, 0, 0, 224, 0, 0,
		228, 0, 0, 232, 0, 0, 236, 0, 0, 240, 0, 0, 244, 0, 0, 248, 0, 0, 252, 0, 0,
		256, 0, 0, 260, 0, 0, 264, 0, 0, 268, 0, 0, 272, 0, 0, 276, 0, 0, 280, 0, 0,
		284, 0, 0, 288, 0, 0, 292, 0, 0, 296, 0, 0, 300, 0, 0, 304, 0, 0, 308, 0, 0,
		312, 0, 0, 316, 0, 0, 320, 0, 0, 324, 0, 0, 328, 0, 0, 332, 0, 0, 336, 0, 0,
		340, 0, 0, 
		344,
	};

const uint32_t FPNG_DECODER_TABLE_BITS = 12;
const uint32_t FPNG_DECODER_TABLE_SIZE = 1 << FPNG_DECODER_TABLE_BITS;

void fpng_buffer_init()
{
    if (clen_table != NULL || lit_table != NULL) {
        printf("fpng buffer already allocated\n");
        return;
    }
    clen_table = (uint32_t *)heap_caps_calloc(sizeof(uint32_t), FPNG_DECODER_TABLE_SIZE, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
    lit_table = (uint32_t *)heap_caps_calloc(sizeof(uint32_t), FPNG_DECODER_TABLE_SIZE, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
}


static bool build_decoder_table(uint32_t num_syms, uint8_t *pCode_sizes, uint32_t *pTable)
{
    uint32_t num_codes[16];

    memset(num_codes, 0, sizeof(num_codes));
    for (uint32_t i = 0; i < num_syms; i++)
    {
        assert(pCode_sizes[i] <= FPNG_DECODER_TABLE_SIZE);
        num_codes[pCode_sizes[i]]++;
    }

    uint32_t next_code[17];
    next_code[0] = next_code[1] = 0;
    uint32_t total = 0;
    for (uint32_t i = 1; i <= 15; i++)
        next_code[i + 1] = (uint32_t)(total = ((total + ((uint32_t)num_codes[i])) << 1));

    if (total != 0x10000)
    {
        uint32_t j = 0;

        for (uint32_t i = 15; i != 0; i--)
            if ((j += num_codes[i]) > 1)
                return false;

        if (j != 1)
            return false;
    }

    uint32_t rev_codes[DEFL_MAX_HUFF_SYMBOLS];

    for (uint32_t i = 0; i < num_syms; i++)
        rev_codes[i] = next_code[pCode_sizes[i]]++;

    memset(pTable, 0, sizeof(uint32_t) * FPNG_DECODER_TABLE_SIZE);

    for (uint32_t i = 0; i < num_syms; i++)
    {
        const uint32_t code_size = pCode_sizes[i];
        if (!code_size)
            continue;

        uint32_t old_code = rev_codes[i], new_code = 0;
        for (uint32_t j = code_size; j != 0; j--)
        {
            new_code = (new_code << 1) | (old_code & 1);
            old_code >>= 1;
        }

        uint32_t j = 1 << code_size;

        while (new_code < FPNG_DECODER_TABLE_SIZE)
        {
            pTable[new_code] = i | (code_size << 9);
            new_code += j;
        }
    }

    return true;
}

static bool prepare_dynamic_block(
    const uint8_t *pSrc, uint32_t src_len, uint32_t &src_ofs,
    uint32_t &bit_buf_size, uint64_t &bit_buf,
    uint32_t *pLit_table, uint32_t num_chans)
{
    static const uint8_t s_bit_length_order[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    uint32_t num_lit_codes, num_dist_codes, num_clen_codes;

    GET_BITS(num_lit_codes, 5);
    num_lit_codes += 257;

    GET_BITS(num_dist_codes, 5);
    num_dist_codes += 1;

    uint32_t total_codes = num_lit_codes + num_dist_codes;
    if (total_codes > (DEFL_MAX_HUFF_SYMBOLS_0 + DEFL_MAX_HUFF_SYMBOLS_1))
        return false;

    uint8_t code_sizes[DEFL_MAX_HUFF_SYMBOLS_0 + DEFL_MAX_HUFF_SYMBOLS_1];
    memset(code_sizes, 0, sizeof(code_sizes));

    GET_BITS(num_clen_codes, 4);
    num_clen_codes += 4;

    uint8_t clen_codesizes[DEFL_MAX_HUFF_SYMBOLS_2];
    memset(clen_codesizes, 0, sizeof(clen_codesizes));

    for (uint32_t i = 0; i < num_clen_codes; i++)
    {
        uint32_t len = 0;
        GET_BITS(len, 3);
        clen_codesizes[s_bit_length_order[i]] = (uint8_t)len;
    }

    // uint32_t clen_table[FPNG_DECODER_TABLE_SIZE];//todo
    if (!build_decoder_table(DEFL_MAX_HUFF_SYMBOLS_2, clen_codesizes, clen_table))
        return false;

    uint32_t min_code_size = 15;

    for (uint32_t cur_code = 0; cur_code < total_codes;)
    {
        uint32_t sym = clen_table[bit_buf & (FPNG_DECODER_TABLE_SIZE - 1)];
        uint32_t sym_len = sym >> 9;
        if (!sym_len)
            return false;
        SKIP_BITS(sym_len);
        sym &= 511;

        if (sym <= 15)
        {
            // Can't be a fpng Huffman table
            if (sym > FPNG_DECODER_TABLE_BITS)
                return false;

            if (sym)
                min_code_size = minimum(min_code_size, sym);

            code_sizes[cur_code++] = (uint8_t)sym;
            continue;
        }

        uint32_t rep_len = 0, rep_code_size = 0;

        switch (sym)
        {
        case 16:
        {
            GET_BITS(rep_len, 2);
            rep_len += 3;
            if (!cur_code)
                return false;
            rep_code_size = code_sizes[cur_code - 1];
            break;
        }
        case 17:
        {
            GET_BITS(rep_len, 3);
            rep_len += 3;
            rep_code_size = 0;
            break;
        }
        case 18:
        {
            GET_BITS(rep_len, 7);
            rep_len += 11;
            rep_code_size = 0;
            break;
        }
        }

        if ((cur_code + rep_len) > total_codes)
            return false;

        for (; rep_len; rep_len--)
            code_sizes[cur_code++] = (uint8_t)rep_code_size;
    }

    uint8_t lit_codesizes[DEFL_MAX_HUFF_SYMBOLS_0];

    memcpy(lit_codesizes, code_sizes, num_lit_codes);
    memset(lit_codesizes + num_lit_codes, 0, DEFL_MAX_HUFF_SYMBOLS_0 - num_lit_codes);

    uint32_t total_valid_distcodes = 0;
    for (uint32_t i = 0; i < num_dist_codes; i++)
        total_valid_distcodes += (code_sizes[num_lit_codes + i] == 1);

    // 1 or 2 because the first version of FPNG only issued 1 valid distance code, but that upset wuffs. So we let 1 or 2 through.
    if ((total_valid_distcodes < 1) || (total_valid_distcodes > 2))
        return false;

    if (code_sizes[num_lit_codes + (num_chans - 1)] != 1)
        return false;

    if (total_valid_distcodes == 2)
    {
        // If there are two valid distance codes, make sure the first is 1 bit.
        if (code_sizes[num_lit_codes + num_chans] != 1)
            return false;
    }

    if (!build_decoder_table(num_lit_codes, lit_codesizes, pLit_table))
        return false;

    // Add next symbol to decoder table, when it fits
    for (uint32_t i = 0; i < FPNG_DECODER_TABLE_SIZE; i++)
    {
        uint32_t sym = pLit_table[i] & 511;
        if (sym >= 256)
            continue;

        uint32_t sym_bits = (pLit_table[i] >> 9) & 15;
        if (!sym_bits)
            continue;
        assert(sym_bits <= FPNG_DECODER_TABLE_BITS);

        uint32_t bits_left = FPNG_DECODER_TABLE_BITS - sym_bits;
        if (bits_left < min_code_size)
            continue;

        uint32_t next_bits = i >> sym_bits;
        uint32_t next_sym = pLit_table[next_bits] & 511;
        uint32_t next_sym_bits = (pLit_table[next_bits] >> 9) & 15;
        if ((!next_sym_bits) || (bits_left < next_sym_bits))
            continue;

        pLit_table[i] |= (next_sym << 16) | (next_sym_bits << (16 + 9));
    }

    return true;
}

static bool fpng_pixel_zlib_raw_decompress(
    const uint8_t *pSrc, uint32_t src_len, uint32_t zlib_len,
    uint8_t *pDst, uint32_t w, uint32_t h,
    uint32_t src_chans, uint32_t dst_chans)
{
    assert((src_chans == 3) || (src_chans == 4));
    assert((dst_chans == 3) || (dst_chans == 4));

    const uint32_t src_bpl = w * src_chans;
    const uint32_t dst_bpl = w * dst_chans;
    const uint32_t dst_len = dst_bpl * h;

    uint32_t src_ofs = 2;
    uint32_t dst_ofs = 0;
    uint32_t raster_ofs = 0;
    uint32_t comp_ofs = 0;

    for (;;)
    {
        if ((src_ofs + 1) > src_len)
            return false;

        const bool bfinal = (pSrc[src_ofs] & 1) != 0;
        const uint32_t btype = (pSrc[src_ofs] >> 1) & 3;
        if (btype != 0)
            return false;

        src_ofs++;

        if ((src_ofs + 4) > src_len)
            return false;
        uint32_t len = pSrc[src_ofs + 0] | (pSrc[src_ofs + 1] << 8);
        uint32_t nlen = pSrc[src_ofs + 2] | (pSrc[src_ofs + 3] << 8);
        src_ofs += 4;

        if (len != (~nlen & 0xFFFF))
            return false;

        if ((src_ofs + len) > src_len)
            return false;

        // Raw blocks are a relatively uncommon case so this isn't well optimized.
        // Supports 3->4 and 4->3 byte/pixel conversion.
        for (uint32_t i = 0; i < len; i++)
        {
            uint32_t c = pSrc[src_ofs + i];

            if (!raster_ofs)
            {
                // Check filter type
                if (c != 0)
                    return false;

                assert(!comp_ofs);
            }
            else
            {
                if (comp_ofs < dst_chans)
                {
                    if (dst_ofs == dst_len)
                        return false;

                    pDst[dst_ofs++] = (uint8_t)c;
                }

                if (++comp_ofs == src_chans)
                {
                    if (dst_chans > src_chans)
                    {
                        if (dst_ofs == dst_len)
                            return false;

                        pDst[dst_ofs++] = (uint8_t)0xFF;
                    }

                    comp_ofs = 0;
                }
            }

            if (++raster_ofs == (src_bpl + 1))
            {
                assert(!comp_ofs);
                raster_ofs = 0;
            }
        }

        src_ofs += len;

        if (bfinal)
            break;
    }

    if (comp_ofs != 0)
        return false;

    // Check for zlib adler32
    if ((src_ofs + 4) != zlib_len)
        return false;

    return (dst_ofs == dst_len);
}

static bool fpng_pixel_zlib_decompress_3(
    const uint8_t *pSrc, uint32_t src_len, uint32_t zlib_len,
    uint8_t *pDst, uint32_t w, uint32_t h)
{
    assert(src_len >= (zlib_len + 4));

    const uint32_t dst_bpl = w * 3;

    if (zlib_len < 7)
        return false;

    // check zlib header
    if ((pSrc[0] != 0x78) || (pSrc[1] != 0x01))
        return false;

    uint32_t src_ofs = 2;

    // if ((pSrc[src_ofs] & 6) == 0)
    //     return fpng_pixel_zlib_raw_decompress(pSrc, src_len, zlib_len, pDst, w, h, 3, 3);

    if ((src_ofs + 4) > src_len)
        return false;
    uint64_t bit_buf = READ_LE32(pSrc + src_ofs);
    src_ofs += 4;

    uint32_t bit_buf_size = 32;

    uint32_t bfinal, btype;
    GET_BITS(bfinal, 1);
    GET_BITS(btype, 2);

    // Must be the final block or it's not valid, and type=2 (dynamic)
    if ((bfinal != 1) || (btype != 2))
        return false;

    // uint32_t lit_table[FPNG_DECODER_TABLE_SIZE];//todo
    if (!prepare_dynamic_block(pSrc, src_len, src_ofs, bit_buf_size, bit_buf, lit_table, 3))
        return false;

    const uint8_t *pPrev_scanline = nullptr;
    uint8_t *pCur_scanline = pDst;

    for (uint32_t y = 0; y < h; y++)
    {
        // At start of PNG scanline, so read the filter literal
        assert(bit_buf_size >= FPNG_DECODER_TABLE_BITS);
        uint32_t filter = lit_table[bit_buf & (FPNG_DECODER_TABLE_SIZE - 1)];
        uint32_t filter_len = (filter >> 9) & 15;
        if (!filter_len)
            return false;
        SKIP_BITS(filter_len);
        filter &= 511;

        uint32_t expected_filter = (y ? 2 : 0);
        if (filter != expected_filter)
            return false;

        uint32_t x_ofs = 0;
        uint8_t prev_delta_r = 0, prev_delta_g = 0, prev_delta_b = 0;
        do
        {
            assert(bit_buf_size >= FPNG_DECODER_TABLE_BITS);
            uint32_t lit0_tab = lit_table[bit_buf & (FPNG_DECODER_TABLE_SIZE - 1)];

            uint32_t lit0 = lit0_tab;
            uint32_t lit0_len = (lit0_tab >> 9) & 15;
            if (!lit0_len)
                return false;
            SKIP_BITS(lit0_len);

            if (lit0 & 256)
            {
                lit0 &= 511;

                // Can't be EOB - we still have more pixels to decompress.
                if (lit0 == 256)
                    return false;

                // Must be an RLE match against the previous pixel.
                uint32_t run_len = s_length_range[lit0 - 257];
                if (lit0 >= 265)
                {
                    uint32_t e;
                    GET_BITS_NE(e, s_length_extra[lit0 - 257]);

                    run_len += e;
                }

                // Skip match distance - it's always the same (3)
                SKIP_BITS_NE(1);

                // Matches must always be a multiple of 3/4 bytes
                assert((run_len % 3) == 0);

                
                // Check for valid run lengths
                if (!g_run_len3_to_4[run_len])
                    return false;

                const uint32_t x_ofs_end = x_ofs + run_len;

                // Matches cannot cross scanlines.
                if (x_ofs_end > dst_bpl)
                    return false;

                if (pPrev_scanline)
                {
                    if ((prev_delta_r | prev_delta_g | prev_delta_b) == 0)
                    {
                        memcpy(pCur_scanline + x_ofs, pPrev_scanline + x_ofs, run_len);
                        x_ofs = x_ofs_end;
                    }
                    else
                    {
                        do
                        {
                            pCur_scanline[x_ofs] = (uint8_t)(pPrev_scanline[x_ofs] + prev_delta_r);
                            pCur_scanline[x_ofs + 1] = (uint8_t)(pPrev_scanline[x_ofs + 1] + prev_delta_g);
                            pCur_scanline[x_ofs + 2] = (uint8_t)(pPrev_scanline[x_ofs + 2] + prev_delta_b);
                            x_ofs += 3;
                        } while (x_ofs < x_ofs_end);
                    }
                }
                else
                {
                    do
                    {
                        pCur_scanline[x_ofs] = prev_delta_r;
                        pCur_scanline[x_ofs + 1] = prev_delta_g;
                        pCur_scanline[x_ofs + 2] = prev_delta_b;
                        x_ofs += 3;
                    } while (x_ofs < x_ofs_end);
                }
            }
            else
            {
                uint32_t lit1, lit2;

                uint32_t lit1_spec_len = (lit0_tab >> (16 + 9));
                uint32_t lit2_len;
                if (lit1_spec_len)
                {
                    lit1 = (lit0_tab >> 16) & 511;
                    SKIP_BITS_NE(lit1_spec_len);

                    assert(bit_buf_size >= FPNG_DECODER_TABLE_BITS);
                    lit2 = lit_table[bit_buf & (FPNG_DECODER_TABLE_SIZE - 1)];
                    lit2_len = (lit2 >> 9) & 15;
                    if (!lit2_len)
                        return false;
                }
                else
                {
                    assert(bit_buf_size >= FPNG_DECODER_TABLE_BITS);
                    lit1 = lit_table[bit_buf & (FPNG_DECODER_TABLE_SIZE - 1)];
                    uint32_t lit1_len = (lit1 >> 9) & 15;
                    if (!lit1_len)
                        return false;
                    SKIP_BITS_NE(lit1_len);

                    lit2_len = (lit1 >> (16 + 9));
                    if (lit2_len)
                        lit2 = lit1 >> 16;
                    else
                    {
                        assert(bit_buf_size >= FPNG_DECODER_TABLE_BITS);
                        lit2 = lit_table[bit_buf & (FPNG_DECODER_TABLE_SIZE - 1)];
                        lit2_len = (lit2 >> 9) & 15;
                        if (!lit2_len)
                            return false;
                    }
                }

                SKIP_BITS(lit2_len);

                // Check for matches
                if ((lit1 | lit2) & 256)
                    return false;
                
                if (pPrev_scanline)
                {
                    pCur_scanline[x_ofs] = (uint8_t)(pPrev_scanline[x_ofs] + lit0);
                    pCur_scanline[x_ofs + 1] = (uint8_t)(pPrev_scanline[x_ofs + 1] + lit1);
                    pCur_scanline[x_ofs + 2] = (uint8_t)(pPrev_scanline[x_ofs + 2] + lit2);
                }
                else
                {
                    pCur_scanline[x_ofs] = (uint8_t)lit0;
                    pCur_scanline[x_ofs + 1] = (uint8_t)lit1;
                    pCur_scanline[x_ofs + 2] = (uint8_t)lit2;
                }
                x_ofs += 3;

                prev_delta_r = (uint8_t)lit0;
                prev_delta_g = (uint8_t)lit1;
                prev_delta_b = (uint8_t)lit2;

                // See if we can decode one more pixel.
                uint32_t spec_next_len0_len = lit2 >> (16 + 9);
                if ((spec_next_len0_len) && (x_ofs < dst_bpl))
                {
                    lit0 = (lit2 >> 16) & 511;
                    if (lit0 < 256)
                    {
                        SKIP_BITS_NE(spec_next_len0_len);

                        assert(bit_buf_size >= FPNG_DECODER_TABLE_BITS);
                        lit1 = lit_table[bit_buf & (FPNG_DECODER_TABLE_SIZE - 1)];
                        uint32_t lit1_len = (lit1 >> 9) & 15;
                        if (!lit1_len)
                            return false;
                        SKIP_BITS(lit1_len);

                        lit2_len = (lit1 >> (16 + 9));
                        if (lit2_len)
                            lit2 = lit1 >> 16;
                        else
                        {
                            assert(bit_buf_size >= FPNG_DECODER_TABLE_BITS);
                            lit2 = lit_table[bit_buf & (FPNG_DECODER_TABLE_SIZE - 1)];
                            lit2_len = (lit2 >> 9) & 15;
                            if (!lit2_len)
                                return false;
                        }

                        SKIP_BITS_NE(lit2_len);

                        // Check for matches
                        if ((lit1 | lit2) & 256)
                            return false;

                        if (pPrev_scanline)
                        {
                            pCur_scanline[x_ofs] = (uint8_t)(pPrev_scanline[x_ofs] + lit0);
                            pCur_scanline[x_ofs + 1] = (uint8_t)(pPrev_scanline[x_ofs + 1] + lit1);
                            pCur_scanline[x_ofs + 2] = (uint8_t)(pPrev_scanline[x_ofs + 2] + lit2);
                        }
                        else
                        {
                            pCur_scanline[x_ofs] = (uint8_t)lit0;
                            pCur_scanline[x_ofs + 1] = (uint8_t)lit1;
                            pCur_scanline[x_ofs + 2] = (uint8_t)lit2;
                        }
                        x_ofs += 3;

                        prev_delta_r = (uint8_t)lit0;
                        prev_delta_g = (uint8_t)lit1;
                        prev_delta_b = (uint8_t)lit2;

                    } // if (lit0 < 256)

                } // if ((spec_next_len0_len) && (x_ofs < bpl))
            }

        } while (x_ofs < dst_bpl);

        pPrev_scanline = pCur_scanline;
        pCur_scanline += dst_bpl;

    } // y

    // The last symbol should be EOB
    assert(bit_buf_size >= FPNG_DECODER_TABLE_BITS);
    uint32_t lit0 = lit_table[bit_buf & (FPNG_DECODER_TABLE_SIZE - 1)];
    uint32_t lit0_len = (lit0 >> 9) & 15;
    if (!lit0_len)
        return false;
    lit0 &= 511;
    if (lit0 != 256)
        return false;

    bit_buf_size -= lit0_len;
    bit_buf >>= lit0_len;

    uint32_t align_bits = bit_buf_size & 7;
    bit_buf_size -= align_bits;
    bit_buf >>= align_bits;

    if (src_ofs < (bit_buf_size >> 3))
        return false;
    src_ofs -= (bit_buf_size >> 3);

    // We should be at the very end, because the bit buf reads ahead 32-bits (which contains the zlib adler32).
    if ((src_ofs + 4) != zlib_len)
        return false;

    return true;
}

int fpng_decode_memory(const uint8_t *pImage, uint32_t image_size, uint8_t *out, uint32_t width, uint32_t height)
{
    if (pImage == NULL || out == NULL)
    {
        return -1;
    }

    if (clen_table == NULL || lit_table == NULL) {
        printf("Your forgot init fpng buffer first!\n");
        return -1;
    }

    uint32_t idat_ofs = 50, idat_len = image_size - 50 - 24;

    const uint8_t *pIDAT_data = pImage + idat_ofs + sizeof(uint32_t) * 2;
    const uint32_t src_len = image_size - (idat_ofs + sizeof(uint32_t) * 2);

    bool decomp_status;
    decomp_status = fpng_pixel_zlib_decompress_3(pIDAT_data, src_len, idat_len, out, width, height);

    if (!decomp_status)
    {
        // Something went wrong. Either the file data was corrupted, or it doesn't conform to one of our zlib/Deflate constraints.
        // The conservative thing to do is indicate it wasn't written by us, and let the general purpose PNG decoder handle it.
        return FPNG_DECODE_NOT_FPNG;
    }

    return FPNG_DECODE_SUCCESS;
}

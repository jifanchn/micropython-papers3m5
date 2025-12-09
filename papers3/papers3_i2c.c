/*
 * Papers3 Shared I2C Manager Implementation
 */

#include "papers3_i2c.h"
#include "papers3_pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define TAG "papers3_i2c"
#define I2C_FREQ_HZ 100000 // 100kHz standard mode

static int s_i2c_ref_count = 0;
static SemaphoreHandle_t s_i2c_mutex = NULL;

esp_err_t papers3_i2c_init(void) {
    // Create mutex if not exists
    if (s_i2c_mutex == NULL) {
        s_i2c_mutex = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(s_i2c_mutex, portMAX_DELAY);
    
    if (s_i2c_ref_count == 0) {
        ESP_LOGI(TAG, "Initializing I2C Bus on Port %d (SDA=%d, SCL=%d)", 
                 PAPERS3_I2C_PORT, PAPERS3_I2C_SDA, PAPERS3_I2C_SCL);
        
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = PAPERS3_I2C_SDA,
            .scl_io_num = PAPERS3_I2C_SCL,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_FREQ_HZ,
            .clk_flags = 0,
        };
        
        esp_err_t ret = i2c_param_config(PAPERS3_I2C_PORT, &conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
            xSemaphoreGive(s_i2c_mutex);
            return ret;
        }
        
        ret = i2c_driver_install(PAPERS3_I2C_PORT, conf.mode, 0, 0, 0);
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "I2C driver already installed, continuing");
            ret = ESP_OK;
        }
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
            xSemaphoreGive(s_i2c_mutex);
            return ret;
        }
    }
    
    s_i2c_ref_count++;
    ESP_LOGD(TAG, "I2C Ref Count: %d", s_i2c_ref_count);
    
    xSemaphoreGive(s_i2c_mutex);
    return ESP_OK;
}

esp_err_t papers3_i2c_deinit(void) {
    if (s_i2c_mutex == NULL) return ESP_FAIL;

    xSemaphoreTake(s_i2c_mutex, portMAX_DELAY);
    
    if (s_i2c_ref_count > 0) {
        s_i2c_ref_count--;
        ESP_LOGD(TAG, "I2C Ref Count: %d", s_i2c_ref_count);
        
        if (s_i2c_ref_count == 0) {
            ESP_LOGI(TAG, "De-initializing I2C Bus");
            i2c_driver_delete(PAPERS3_I2C_PORT);
        }
    }
    
    xSemaphoreGive(s_i2c_mutex);
    return ESP_OK;
}

esp_err_t papers3_i2c_write(uint8_t addr, uint8_t reg_addr, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(PAPERS3_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t papers3_i2c_write_buffer(uint8_t addr, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    if (len > 0) {
        i2c_master_write(cmd, data, len, true);
    }
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(PAPERS3_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t papers3_i2c_read(uint8_t addr, uint8_t reg_addr, uint8_t *data, size_t len) {
    if (len == 0) return ESP_OK;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(PAPERS3_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

bool papers3_i2c_probe(uint8_t addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(PAPERS3_I2C_PORT, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    
    return (ret == ESP_OK);
}

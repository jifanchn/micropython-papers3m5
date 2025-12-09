/*
 * Papers3 Shared I2C Manager
 * Handles shared access to the I2C bus for Touch, RTC, and Gyro
 */

#ifndef PAPERS3_I2C_H
#define PAPERS3_I2C_H

#include "esp_err.h"
#include "driver/i2c.h"

// Initialize the shared I2C bus
// Increments a reference counter. Only initializes hardware on first call.
esp_err_t papers3_i2c_init(void);

// Deinitialize the shared I2C bus
// Decrements reference counter. Only frees hardware when count reaches 0.
esp_err_t papers3_i2c_deinit(void);

// Helper for I2C write
esp_err_t papers3_i2c_write(uint8_t addr, uint8_t reg_addr, uint8_t data);

// Helper for I2C write (buffer)
esp_err_t papers3_i2c_write_buffer(uint8_t addr, uint8_t *data, size_t len);

// Helper for I2C read
esp_err_t papers3_i2c_read(uint8_t addr, uint8_t reg_addr, uint8_t *data, size_t len);

// Helper to scan for a device
bool papers3_i2c_probe(uint8_t addr);

#endif // PAPERS3_I2C_H

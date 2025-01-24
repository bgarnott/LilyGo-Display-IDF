/**
 * @file      realy_config.h
 * @author    Brian Arnott (brian.arnott@gmail.com)
 * @license   MIT
 * @date      2025-01-16
 *
 */
#pragma once
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

void relay_config(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);

void relay_toggle(i2c_master_dev_handle_t *dev_handle, uint8_t relay);

#ifdef __cplusplus
}
#endif
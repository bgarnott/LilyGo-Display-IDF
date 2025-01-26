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

void relay_config(i2c_master_bus_handle_t *bus_handle);

void relay_toggle(uint8_t relay);

void relay_on(uint8_t relay);

void relay_off(uint8_t relay);

void crane_up();

void crane_down();

void crane_stop();

void vacuum_on();

void vacuum_off();

#ifdef __cplusplus
}
#endif
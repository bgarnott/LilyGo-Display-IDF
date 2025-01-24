/**
 * @file      joystick_config.h
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

typedef struct joystick_struct {
    uint8_t         pressed;
    uint8_t         x;
    uint8_t         y;
} joystick_struct_t;

void joystick_go(i2c_master_bus_handle_t *bus);

joystick_struct_t joystick_get_state(void);

#ifdef __cplusplus
}
#endif
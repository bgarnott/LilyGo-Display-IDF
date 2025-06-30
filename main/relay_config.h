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

#define RELAY1_GPIO 2
#define RELAY2_GPIO 3
#define RELAY3_GPIO 10 
#define RELAY4_GPIO 11
#define RELAY5_GPIO 43
#define RELAY6_GPIO 44
#define RELAY7_GPIO 45
#define RELAY8_GPIO 46

void relay_config();

void relay_on(uint8_t relay);

void relay_off(uint8_t relay);

void crane_up();

void crane_down();

void crane_stop();

void vacuum_on();

void vacuum_off();

void crowd_up();

void crowd_down();

void crowd_stop();

#ifdef __cplusplus
}
#endif
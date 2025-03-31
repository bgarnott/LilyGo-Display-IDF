/**
 * @file      sleep_config.c
 * @author    Brian Arnott (brian.arnott@gmail.com)
 * @license   MIT
 * @date      2025-01-16
 *
 */

#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "sleep_config.h"
#include "product_pins.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

static const char *TAG = "sleep_config";

void sleep_config(void)
{
    ESP_LOGI(TAG, "Starting sleep config");
    ESP_ERROR_CHECK(rtc_gpio_pulldown_en(GPIO_NUM_1));
    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(GPIO_NUM_1, 0));
}

void sleep_go(void)
{
    ESP_LOGI(TAG, "Starting sleep");
    esp_deep_sleep_start();
}





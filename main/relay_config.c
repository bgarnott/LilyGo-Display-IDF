/**
 * @file      relay_config.c
 * @author    Brian Arnott (brian.arnott@gmail.com)
 * @license   MIT
 * @date      2025-01-16
 *
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "product_pins.h"
#include "driver/i2c_master.h"
#include "soc/clk_tree_defs.h"
#include "i2c_driver.h"
#include "relay_config.h"

#define I2C_MASTER_FREQ_HZ          400000      /*!< I2C master clock frequency */


static const char *TAG = "relay_config";
/**
 * @brief Configure GPIO pins for relays
 */
void relay_config(void)
{
    gpio_config_t io_conf;
    
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 
          (1ULL << RELAY1_GPIO) 
        | (1ULL << RELAY2_GPIO) 
        | (1ULL << RELAY3_GPIO) 
        | (1ULL << RELAY4_GPIO)
//        | (1ULL << RELAY5_GPIO);
//        | (1ULL << RELAY6_GPIO)
        | (1ULL << RELAY7_GPIO)
        | (1ULL << RELAY8_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Relay GPIOs configured: %d, %d, %d, %d, %d, %d, %d, %d",
             RELAY1_GPIO, RELAY2_GPIO, RELAY3_GPIO, RELAY4_GPIO,
             RELAY5_GPIO, RELAY6_GPIO, RELAY7_GPIO, RELAY8_GPIO);    
}

void relay_on(uint8_t relay)
{
    uint32_t gpio_num = 0;

    switch (relay) {
        case 1:
            gpio_num = RELAY1_GPIO;
            break;
        case 2:
            gpio_num = RELAY2_GPIO;
            break;
        case 3:
            gpio_num = RELAY3_GPIO;
            break;
        case 4:
            gpio_num = RELAY4_GPIO;
            break;
        default:
            ESP_LOGW(TAG, "Invalid relay number: %d", relay);
            return;
    }

    gpio_set_level(gpio_num, 1);
    ESP_LOGI(TAG, "Relay %d (GPIO %d) set to ON", relay, gpio_num);
}

void relay_off(uint8_t relay)
{
    uint32_t gpio_num = 0;

    switch (relay) {
        case 1:
            gpio_num = RELAY1_GPIO;
            break;
        case 2:
            gpio_num = RELAY2_GPIO;
            break;
        case 3:
            gpio_num = RELAY3_GPIO;
            break;
        case 4:
            gpio_num = RELAY4_GPIO;
            break;
        default:
            ESP_LOGW(TAG, "Invalid relay number: %d", relay);
            return;
    }

    gpio_set_level(gpio_num, 0);
    ESP_LOGI(TAG, "Relay %d (GPIO %d) set to OFF", relay, gpio_num);
}

void crane_down()
{
    relay_on(1);
    relay_on(2);
    relay_off(3);
    ESP_LOGI(TAG, "Crane down");

}

void crane_up()
{
    relay_on(1);
    relay_off(2);
    relay_on(3);
    ESP_LOGI(TAG, "Crowd up");
}

void crane_stop()
{
    relay_off(1);
    relay_off(2);
    relay_off(3);
    ESP_LOGI(TAG, "Crowd stop");
}

void vacuum_on()
{
//    relay_on(4);
}

void vacuum_off()
{
//    relay_off(4);
}   

void crowd_down()
{
    ESP_LOGI(TAG, "Crowd down");
}

void crowd_up()
{
    ESP_LOGI(TAG, "Crowd up");
}

void crowd_stop()
{
    ESP_LOGI(TAG, "Crowd stop");
}
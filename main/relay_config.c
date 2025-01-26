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

enum RELAY_COMMANDS
{

    TOGGLE_RELAY_ONE = 0x01,
    TOGGLE_RELAY_TWO,
    TOGGLE_RELAY_THREE,
    TOGGLE_RELAY_FOUR,
    RELAY_ONE_STATUS,
    RELAY_TWO_STATUS,
    RELAY_THREE_STATUS,
    RELAY_FOUR_STATUS,
    TURN_ALL_OFF = 0xA,
    TURN_ALL_ON,
    TOGGLE_ALL,
    RELAY_ONE_PWM = 0x10,
    RELAY_TWO_PWM,
    RELAY_THREE_PWM,
    RELAY_FOUR_PWM
};

static const char *TAG = "relay_config";

static i2c_master_dev_handle_t dev_handle;

void relay_config(i2c_master_bus_handle_t *bus_handle)
{
    ESP_LOGI(TAG, "Starting relay config");
    
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x6d,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, &dev_handle));
}


void relay_toggle(uint8_t relay)
{
    uint8_t write_data;

    switch (relay)
    {
        case 1:
            write_data = TOGGLE_RELAY_ONE;
            break;
        case 2:
            write_data = TOGGLE_RELAY_TWO;
            break;
        case 3: 
            write_data = TOGGLE_RELAY_THREE;
            break;
        case 4:
            write_data = TOGGLE_RELAY_FOUR;
            break;      
        default:
            return;
    }

    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &write_data, 1, 1000));
}

void relay_on(uint8_t relay)
{
    uint8_t status;
    uint8_t command;

    ESP_LOGI(TAG, "Turning on relay %d", relay);

    if (relay == 1)
        command = RELAY_ONE_STATUS;
    else if (relay == 2)
        command = RELAY_TWO_STATUS;
    else if (relay == 3)
        command = RELAY_THREE_STATUS;
    else if (relay == 4)
        command = RELAY_FOUR_STATUS;
  
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, &command, 1, &status, 1, 1000));

    ESP_LOGI(TAG, "status of relay %d is %d", relay, status);

    if (status == 0)
        relay_toggle(relay);

}

void relay_off(uint8_t relay)
{
    uint8_t status;
    uint8_t command;

    ESP_LOGI(TAG, "Turning off relay %d", relay);

    if (relay == 1)
        command = RELAY_ONE_STATUS;
    else if (relay == 2)
        command = RELAY_TWO_STATUS;
    else if (relay == 3)
        command = RELAY_THREE_STATUS;
    else if (relay == 4)
        command = RELAY_FOUR_STATUS;
  
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, &command, 1, &status, 1, 1000));

    ESP_LOGI(TAG, "status of relay %d is %d", relay, status);

    if (status == 15)
        relay_toggle(relay);

}

void crane_down()
{
    relay_on(1);
    relay_on(2);
    relay_off(3);

}

void crane_up()
{
    relay_on(1);
    relay_off(2);
    relay_on(3);
}

void crane_stop()
{
    relay_off(1);
    relay_off(2);
    relay_off(3);
}

void vacuum_on()
{
    relay_on(4);
}

void vacuum_off()
{
    relay_off(4);
}   
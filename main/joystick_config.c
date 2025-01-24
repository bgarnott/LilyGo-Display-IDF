/**
 * @file      joystick_config.c
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
#include "joystick_config.h"


#define I2C_MASTER_FREQ_HZ          400000      /*!< I2C master clock frequency */

static joystick_struct_t joystick_state;

enum joystickRegisters {
  JOYSTICK_ID = 0x00,
  JOYSTICK_VERSION1, // 0x01
  JOYSTICK_VERSION2, // 0x02
  JOYSTICK_X_MSB, // 0x03
  JOYSTICK_X_LSB, // 0x04
  JOYSTICK_Y_MSB, // 0x05
  JOYSTICK_Y_LSB, // 0x06
  JOYSTICK_BUTTON, // 0x07
  JOYSTICK_STATUS, // 0x08 //1 - button clicked
  JOYSTICK_I2C_LOCK, // 0x09
  JOYSTICK_CHANGE_ADDRESS, // 0x0A
};

static const char *TAG = "joystick_config";

static void joystick_task(void *arg)
{
    ESP_LOGI(TAG, "Starting joystick task");
    i2c_master_bus_handle_t *bus_handle = (i2c_master_bus_handle_t *) arg;

    uint32_t task_delay_ms = 10;
    esp_err_t err = ESP_OK;
    i2c_master_dev_handle_t dev_handle;
    uint8_t write_data[2];
    uint8_t read_data[2];

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x20,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, &dev_handle));
    
    while (1) 
    {
        write_data[0] = JOYSTICK_X_MSB;
        i2c_master_transmit_receive(dev_handle, write_data, 1, read_data, 1, 1000);
        joystick_state.x = read_data[0];
        write_data[0] = JOYSTICK_Y_MSB;
        i2c_master_transmit_receive(dev_handle, write_data, 1, read_data, 1, 1000);
        joystick_state.y = read_data[0];
        write_data[0] = JOYSTICK_BUTTON;
        i2c_master_transmit_receive(dev_handle, write_data, 1, read_data, 1, 1000);
        if (read_data[0] == 1)
        {
            joystick_state.pressed = 0;
        }
        else
        {
            joystick_state.pressed = 1;
        }
        
        vTaskDelay(task_delay_ms);
    }
}

joystick_struct_t joystick_get_state(void)
{
    return joystick_state;
}


void joystick_go(i2c_master_bus_handle_t *bus)
{
    xTaskCreate(joystick_task, "JOYSTICK", 4*1024, bus, tskIDLE_PRIORITY, NULL);
}
/**
 * @file      i2c_driver.c
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  Shenzhen Xinyuan Electronic Technology Co., Ltd
 * @date      2024-01-07
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

#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          400000      /*!< I2C master clock frequency */
#define I2C_MASTER_SDA_IO           (gpio_num_t) BOARD_I2C_SDA
#define I2C_MASTER_SCL_IO           (gpio_num_t) BOARD_I2C_SCL

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

static const char *TAG = "i2c_driver";

i2c_master_bus_handle_t bus_handle;

void i2c_drv_scan()
{
    esp_err_t err = ESP_OK;
    uint8_t address = 0x00;
    printf("Scand I2C Devices:\n");
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            fflush(stdout);
            address = i + j;
            err = i2c_master_probe(bus_handle, address, 1000);
            if (err == ESP_OK) {
                printf("%02x ", address);
            } else if (err == ESP_ERR_TIMEOUT) {
                printf("UU ");
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }
    printf("\n\n\n");
}

bool i2c_drv_probe(uint8_t devAddr)
{
    return ESP_OK == i2c_master_probe(bus_handle, devAddr, 1000);
}


/**
 * @brief i2c master initialization
 */
esp_err_t i2c_driver_init(void)
{
    i2c_master_bus_config_t i2c_bus_config;
    memset(&i2c_bus_config, 0, sizeof(i2c_bus_config));
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.i2c_port = I2C_MASTER_NUM;
    i2c_bus_config.scl_io_num = I2C_MASTER_SCL_IO;
    i2c_bus_config.sda_io_num = I2C_MASTER_SDA_IO;
    i2c_bus_config.glitch_ignore_cnt = 7;
    i2c_bus_config.flags.enable_internal_pullup = true;
    return i2c_new_master_bus(&i2c_bus_config, &bus_handle);
}

static void joystick_task(void *arg)
{
    ESP_LOGI(TAG, "Starting joystick task");
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
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
    
    while (1) 
    {
        write_data[0] = JOYSTICK_X_MSB;
        i2c_master_transmit_receive(dev_handle, write_data, 1, read_data, 1, 1000);
        ESP_LOGI(TAG, "Joystick X: %d", read_data[0]);
        write_data[0] = JOYSTICK_Y_MSB;
        i2c_master_transmit_receive(dev_handle, write_data, 1, read_data, 1, 1000);
        ESP_LOGI(TAG, "Joystick Y: %d", read_data[0]);
        write_data[0] = JOYSTICK_BUTTON;
        i2c_master_transmit_receive(dev_handle, write_data, 1, read_data, 1, 1000);
        ESP_LOGI(TAG, "Joystick button: %d", read_data[0]);
        vTaskDelay(task_delay_ms);
    }
}

void joystick_go(void)
{
    xTaskCreate(joystick_task, "JOYSTICK", 4*1024, NULL, tskIDLE_PRIORITY, NULL);
}
/**
 * @file      main.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  Shenzhen Xinyuan Electronic Technology Co., Ltd
 * @date      2024-01-07
 *
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "amoled_driver.h"
#include "i2c_driver.h"
#include "power_driver.h"
#include "demos/lv_demos.h"
#include "product_pins.h"
#include "lv_examples.h"
#include "lvgl_config.h"
#include "joystick_config.h"
#include "relay_config.h"


static const char *TAG = "main";

extern "C" void app_main(void)
{
    i2c_master_bus_handle_t i2c_bus;
    ESP_LOGI(TAG, "------ Initialize I2C.");
    ESP_ERROR_CHECK(i2c_driver_init(&i2c_bus));

    ESP_LOGI(TAG, "------ Initialize PMU.");
    if (!power_driver_init()) {
        ESP_LOGE(TAG, "ERROR :No find PMU ....");
    }

    ESP_LOGI(TAG, "------ Initialize DISPLAY.");
    display_init();

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    ESP_LOGI(TAG, "Initialize LVGL Screen");
    lvgl_config();
    // alloc draw buffers used by LVGL

    ESP_LOGI(TAG, "Configure LVGL UI");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_lock(-1)) {
        config_gui();
        //lv_demo_music();
        lvgl_unlock();
    }

    ESP_LOGI(TAG, "Configure gpio");
    button_config();

    ESP_LOGI(TAG, "Start LVGL thread");
    lvgl_go();

    ESP_LOGI(TAG, "Start Button thread");
    button_go();

    ESP_LOGI(TAG, "Start joystick thread");
    ///i2c_drv_scan(&i2c_bus);
    joystick_go(&i2c_bus);

    ESP_LOGI(TAG, "Configure relay");
    i2c_master_dev_handle_t dev_handle;
    relay_config(&i2c_bus, &dev_handle);

    while(1)
    {
        relay_toggle(&dev_handle, 1);
        relay_toggle(&dev_handle, 2);
        relay_toggle(&dev_handle, 3);
        relay_toggle(&dev_handle, 4);
        vTaskDelay(100);
    }   
}

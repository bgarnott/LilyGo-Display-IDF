/**
 * @file      lvgl_config.c
 * @author    Brian Arnott (brian.arnott@gmail.com)
 * @license   MIT
 * @date      2025-01-16
 *
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "amoled_driver.h"
#include "product_pins.h"
#include "esp_err.h"
#include "esp_log.h"
#include "product_pins.h"
#include "driver/gpio.h"
#include "lvgl_config.h"
#include "joystick_config.h"
#include "relay_config.h"
#include "sleep_config.h"

#define LVGL_TICK_PERIOD_MS 1
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_STACK_SIZE (4 * 1024)
#define LVGL_TASK_PRIORITY 2

#define NUM_SCREENS 4

static const char *TAG = "lvgl_config";

static lv_indev_t * indev_button_up;
static lv_indev_t * indev_button_down;
static lv_indev_t * indev_button_left;
static lv_indev_t * indev_button_middle;
static lv_indev_t * indev_button_right;

static lv_indev_drv_t indev_button_up_drv;
static lv_indev_drv_t indev_button_down_drv;
static lv_indev_drv_t indev_button_left_drv;
static lv_indev_drv_t indev_button_middle_drv;
static lv_indev_drv_t indev_button_right_drv;

static SemaphoreHandle_t lvgl_mux = NULL;

static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)

static lv_disp_drv_t disp_drv;      // contains callback functions

static int vac_on = 0;

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    display_push_colors(area->x1, area->y1, w, h, (uint16_t *)color_map);
    lv_disp_flush_ready( drv );
}

static void increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

bool lvgl_lock(int timeout_ms)
{
    // Convert timeout in milliseconds to FreeRTOS ticks
    // If `timeout_ms` is set to -1, the program will block until the condition is met
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void lvgl_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}

static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    while (1) {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        if (lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            // Release the mutex
            lvgl_unlock();
        }
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

static void button_task(void *arg)
{
    ESP_LOGI(TAG, "Starting button task");
    uint32_t task_delay_ms = 10;
    int but1;
    int but2;
    uint32_t x = 0;
    while (1) {
        but1 = gpio_get_level(GPIO_NUM_21);
        but2 = gpio_get_level(GPIO_NUM_0);
        
        if (!but1)
        {
            //button_state.pressed = LV_INDEV_STATE_PRESSED;
            //button_state.key = LV_KEY_ENTER;
        }
        else if (!but2)
        {
            //button_state.pressed = LV_INDEV_STATE_PRESSED;
            //button_state.key = LV_KEY_NEXT;
        }
        else
        {
            //button_state.pressed = LV_INDEV_STATE_RELEASED;
        }
        gpio_set_level(GPIO_NUM_38, x%2);
        x++;
        vTaskDelay(task_delay_ms);
    }
}

void button_config(void)
{
    // configure the "button 2" GPIO pin on the T Display S3 (GPIO 21) to be a input
    // pin with a pulldown resistor.
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1 << BOARD_BUTTON2_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

void joystick_button_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    joystick_struct_t joystick_state = joystick_get_state();

    if (joystick_state.pressed)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGI(TAG, "Enter");
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void button_up_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    joystick_struct_t joystick_state = joystick_get_state();

    if (joystick_state.y > 200)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGI(TAG, "UP");
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void button_down_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    joystick_struct_t joystick_state = joystick_get_state();

    if (joystick_state.y < 50)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGI(TAG, "DOWN");
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void button_left_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    joystick_struct_t joystick_state = joystick_get_state();

    if (joystick_state.x > 200)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGI(TAG, "LEFT");
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void button_right_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    joystick_struct_t joystick_state = joystick_get_state();

    if (joystick_state.x < 50)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGI(TAG, "RIGHT");
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}


void config_up_button(void)
{
    static const lv_point_t points_array[] = {{100,30}};

    lv_indev_drv_init(&indev_button_up_drv);
    indev_button_up_drv.type = LV_INDEV_TYPE_BUTTON;
    indev_button_up_drv.read_cb = button_up_read;
    indev_button_up = lv_indev_drv_register(&indev_button_up_drv);
    lv_indev_set_button_points(indev_button_up, points_array);
}

void config_down_button(void)
{
    static const lv_point_t points_array[] = {{100,480}};

    lv_indev_drv_init(&indev_button_down_drv);
    indev_button_down_drv.type = LV_INDEV_TYPE_BUTTON;
    indev_button_down_drv.read_cb = button_down_read;
    indev_button_down = lv_indev_drv_register(&indev_button_down_drv);
    lv_indev_set_button_points(indev_button_down, points_array);
}

void config_left_button(void)
{
    static const lv_point_t points_array[] = {{10,AMOLED_HEIGHT/2}};

    lv_indev_drv_init(&indev_button_left_drv);
    indev_button_left_drv.type = LV_INDEV_TYPE_BUTTON;
    indev_button_left_drv.read_cb = button_left_read;
    indev_button_left = lv_indev_drv_register(&indev_button_left_drv);
    lv_indev_set_button_points(indev_button_left, points_array);
}

void config_right_button(void)
{
    static const lv_point_t points_array[] = {{AMOLED_WIDTH-10 ,AMOLED_HEIGHT/2}};

    lv_indev_drv_init(&indev_button_right_drv);
    indev_button_right_drv.type = LV_INDEV_TYPE_BUTTON;
    indev_button_right_drv.read_cb = button_right_read;
    indev_button_right = lv_indev_drv_register(&indev_button_right_drv);
    lv_indev_set_button_points(indev_button_right, points_array);
}

void config_middle_button(void)
{
    static const lv_point_t points_array[] = {{AMOLED_WIDTH/2 ,AMOLED_HEIGHT/2}};

    lv_indev_drv_init(&indev_button_middle_drv);
    indev_button_middle_drv.type = LV_INDEV_TYPE_BUTTON;
    indev_button_middle_drv.read_cb = joystick_button_read;
    indev_button_middle = lv_indev_drv_register(&indev_button_middle_drv);
    lv_indev_set_button_points(indev_button_middle, points_array);
}

void lvgl_config(void)
{
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(DISPLAY_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);

    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(DISPLAY_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf2);

    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISPLAY_BUFFER_SIZE);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = AMOLED_WIDTH;
    disp_drv.ver_res = AMOLED_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.full_refresh = 0;
    //disp_drv.sw_rotate = 1;
    //disp_drv.rotated =  LV_DISP_ROT_90;
    lv_disp_drv_register(&disp_drv);
    //config_keypad();
    config_up_button();
    config_down_button();
    config_left_button();
    config_right_button();

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick",
        .skip_unhandled_events = false
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);
}

void lvgl_go(void)
{
    xTaskCreate(lvgl_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

}

void button_go(void)
{
    xTaskCreate(button_task, "BUTTON", LVGL_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
}

static void btn_down_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSING) 
    {
        crane_down();
    }
    else if(code == LV_EVENT_PRESS_LOST) 
    {
        crane_stop();
    }
    else if(code == LV_EVENT_RELEASED) 
    {
        crane_stop();
    }
}

static void btn_up_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSING) 
    {
        crane_up();
    }
    else if(code == LV_EVENT_PRESS_LOST) 
    {
        crane_stop();
    }
    else if(code == LV_EVENT_RELEASED) 
    {
        crane_stop();
    }
}

static void btn_crowd_down_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSING) 
    {
        crowd_down();
    }
    else if(code == LV_EVENT_PRESS_LOST) 
    {
        crowd_stop();
    }
    else if(code == LV_EVENT_RELEASED) 
    {
        crowd_stop();
    }
}

static void btn_crowd_up_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSING) 
    {
        crowd_up();
    }
    else if(code == LV_EVENT_PRESS_LOST) 
    {
        crowd_stop();
    }
    else if(code == LV_EVENT_RELEASED) 
    {
        crowd_stop();
    }
}

static void btn_vac_on_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t *parent = lv_obj_get_parent(btn);

    if(code == LV_EVENT_LONG_PRESSED) 
    {
        if (vac_on)
        {
            vac_on = 0;
            vacuum_off();
            lv_obj_set_style_bg_color(parent, lv_color_hex(LV_PALETTE_GREY), 0); // Set parent background to gray
        }
        else
        {
            vac_on = 1;
            vacuum_on();
            lv_obj_set_style_bg_color(parent, lv_color_hex(LV_PALETTE_RED), 0); // Set parent background to red
        }
    }
}

static void btn_vac_off_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSING) 
    {
        vacuum_off();
    }
}

static void btn_power_off_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSING) 
    {
        sleep_go();
    }
}

void config_gui(void)
{
    static lv_coord_t col_dsc[] = {75, 74, 75, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, 60, 60, 60, 60, LV_GRID_TEMPLATE_LAST};

    lv_obj_t * obj = lv_obj_create(NULL);
    lv_obj_set_size(obj, AMOLED_WIDTH, AMOLED_HEIGHT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(LV_PALETTE_GREY), 0);

    /*Create a container with grid*/
    lv_obj_set_style_grid_column_dsc_array(obj, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(obj, row_dsc, 0);
    lv_obj_center(obj);
    lv_obj_set_layout(obj, LV_LAYOUT_GRID);

    // create the top button on the screen
    lv_obj_t * btn = lv_btn_create(obj);
    lv_obj_set_size(btn, 100, 50);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Up");
    lv_obj_add_event_cb(btn, btn_up_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_center(label);


    // create the right side button
    btn = lv_btn_create(obj); 
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 2, 1,
                         LV_GRID_ALIGN_STRETCH, 3, 2);
    lv_obj_add_event_cb(btn, btn_crowd_up_event_cb, LV_EVENT_ALL, NULL);
    label = lv_label_create(btn);
    lv_label_set_text(label, "CRD up");
    lv_obj_center(label);

    // create the left side button
    btn = lv_btn_create(obj); 
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 3, 2);
    lv_obj_add_event_cb(btn, btn_crowd_down_event_cb, LV_EVENT_ALL, NULL);
    label = lv_label_create(btn);
    lv_label_set_text(label, "CRD dn");
    lv_obj_center(label);

    // create the middle button
    btn = lv_btn_create(obj); 
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 3, 2);
    lv_obj_add_event_cb(btn, btn_vac_on_event_cb, LV_EVENT_ALL, NULL);
    label = lv_label_create(btn);
    lv_label_set_text(label, "Vac");
    lv_obj_center(label);

    // create the bottom button
    btn = lv_btn_create(obj);
    lv_obj_set_size(btn, 100, 50);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 7, 1);
    label = lv_label_create(btn);
    lv_label_set_text(label, "Down");
    lv_obj_add_event_cb(btn, btn_down_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_center(label);
    lv_scr_load(obj);
}

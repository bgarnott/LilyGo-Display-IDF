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

#define LVGL_TICK_PERIOD_MS 1
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_STACK_SIZE (4 * 1024)
#define LVGL_TASK_PRIORITY 2

static const char *TAG = "lvgl_config";

static button_struct_t button_state;

static lv_indev_t * indev_keypad;

static lv_indev_drv_t indev_drv;

static SemaphoreHandle_t lvgl_mux = NULL;

static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)

static lv_disp_drv_t disp_drv;      // contains callback functions

static lv_obj_t *screen_array[4];

static lv_group_t *group_array[4];

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
            button_state.pressed = LV_INDEV_STATE_PRESSED;
            button_state.key = LV_KEY_NEXT;
        }
        else if (!but2)
        {
            button_state.pressed = LV_INDEV_STATE_PRESSED;
            button_state.key = LV_KEY_ENTER;
        }
        else
        {
            button_state.pressed = LV_INDEV_STATE_RELEASED;
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

void keypad_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    data->key = button_state.key;      
    data->state = button_state.pressed;
}

void config_keypad(void)
{
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv);
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
    disp_drv.hor_res = AMOLED_HEIGHT;
    disp_drv.ver_res = AMOLED_WIDTH;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.full_refresh = DISPLAY_FULLRESH;
    //disp_drv.sw_rotate = 1;
    //disp_drv.rotated =  LV_DISP_ROT_90;
    lv_disp_drv_register(&disp_drv);
    config_keypad();

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

button_struct_t get_button_state()
{
    return button_state;
}

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    //ESP_LOGI(TAG, "callback called!!!!. Code is %d", code);

    if(code == LV_EVENT_PRESSING) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}


static void btn_next_screen_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        static int current_screen = 0;
        current_screen ++;
        lv_scr_load_anim(screen_array[current_screen % 4],LV_SCR_LOAD_ANIM_MOVE_BOTTOM,500,0,0);

        // need to set focus to the first button on the new screen
        lv_obj_t * btn = lv_obj_get_child(screen_array[current_screen%4], -1);
        lv_group_set_default(group_array[current_screen%4]);
        lv_indev_set_group(indev_keypad, group_array[current_screen%4]);
        lv_group_focus_obj(btn);
    }
}

void config_gui(void)
{
    // create 4 screens
    for (int i=0; i<4; i++)
    {
        lv_obj_t * obj = lv_obj_create(NULL);
        lv_obj_set_size(obj, AMOLED_WIDTH, AMOLED_HEIGHT);
        screen_array[i] = obj;

        lv_group_t * group = lv_group_create();
        group_array[i] = group;

        lv_obj_t * btn = lv_btn_create(screen_array[i]);     /*Add a button the current screen*/
        lv_obj_set_pos(btn, 10, 10);                            /*Set its position*/
        lv_obj_set_size(btn, 120, 50);                          /*Set its size*/
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

        lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
        lv_label_set_text(label, "Button");                     /*Set the labels text*/
        lv_obj_center(label);
        lv_group_add_obj(group_array[i], btn);

        btn = lv_btn_create(screen_array[i]);     /*Add a button the current screen*/
        lv_obj_set_pos(btn, 150, 100);                            /*Set its position*/
        lv_obj_set_size(btn, 120, 50);                          /*Set its size*/
        lv_obj_add_event_cb(btn, btn_next_screen_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/
        lv_group_add_obj(group_array[i], btn);

        label = lv_label_create(btn);          /*Add a label to the button*/
        lv_label_set_text(label, "Next screen");                     /*Set the labels text*/
        lv_obj_center(label);
    }

    // set screen zero as the active screen.
    lv_scr_load(screen_array[0]);
    // set group zero as the default group    
    lv_group_set_default(group_array[0]);
    lv_indev_set_group(indev_keypad, group_array[0]);
}

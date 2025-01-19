/**
 * @file      lvgl_config.h
 * @author    Brian Arnott (brian.arnott@gmail.com)
 * @license   MIT
 * @date      2025-01-16
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct button_struct {
    lv_indev_state_t    pressed;
    uint32_t            key;
} button_struct_t;

void button_config(void);

void button_go(void);

void lvgl_config(void);

void lvgl_go(void);

bool lvgl_lock(int timeout_ms);

void lvgl_unlock(void);

void config_gui(void);

#ifdef __cplusplus
}
#endif
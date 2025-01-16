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

void lvgl_config(void);

void lvgl_go(void);

bool lvgl_lock(int timeout_ms);

void lvgl_unlock(void);
#ifdef __cplusplus
}
#endif
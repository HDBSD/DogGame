#ifndef TITLE_DISPLAY_H
#define TITLE_DISPLAY_H
#include <stdint.h>
void title_display_init(void);
void title_display_frame(void);
void title_display_input_frame(void);
int title_display_native_event(uint32_t target, uint32_t event);
uint32_t title_display_find_asset(uint32_t id);
void title_menu_event(uint32_t item, uint32_t event);
#endif

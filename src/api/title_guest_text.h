#ifndef TITLE_GUEST_TEXT_H
#define TITLE_GUEST_TEXT_H
#include <stdint.h>
/* Current draw state: valid only on the title thread inside the guest text
 * entry point. Source replacement is temporary; restore it after the draw. */
uint32_t title_guest_text_current_box(void);
uint32_t title_guest_text_source(uint32_t box);
void title_guest_text_set_source(uint32_t box, uint32_t source);
char *title_guest_text_string(uint32_t address);
uint32_t title_guest_text_allocate_buffer(uint32_t bytes);
uint32_t title_guest_text_continuation(void);
void title_guest_text_set_continuation(uint32_t address);
void title_prompt_draw_original(void);
#endif

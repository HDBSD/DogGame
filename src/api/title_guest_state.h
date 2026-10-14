#ifndef TITLE_GUEST_STATE_H
#define TITLE_GUEST_STATE_H
#include <stdint.h>
/* Title-thread state API. Addresses and access widths are private to the
 * implementation; meanings were inherited from the existing title hooks. */
uint32_t title_state_menu_mode(void);
void title_state_set_menu_mode(uint32_t value);
uint32_t title_state_current_scene(void);
void title_state_set_current_scene(uint32_t value);
uint32_t title_state_player_powers(void);
void title_state_set_player_powers(uint32_t value);
uint32_t title_state_player_health(void);
void title_state_set_player_health(uint32_t value);
uint32_t title_state_player_model(void);
void title_state_set_player_model(uint32_t value);
uint32_t title_state_player_frame(void);
void title_state_set_player_frame(uint32_t value);
uint32_t title_state_active_camera(void);
void title_state_set_active_camera(uint32_t value);
uint8_t title_state_main_menu_started(void);
void title_state_set_main_menu_started(uint8_t value);

typedef struct TitlePlayerPosition
{
    float x, y, z;
    int available;
} TitlePlayerPosition;

uint32_t title_state_scene_id(uint32_t scene);
TitlePlayerPosition title_state_player_position(void);
void title_state_skip_boot_logo(void);
void title_state_select_save_slot(unsigned slot);
int title_state_map_options_open(void);
#endif

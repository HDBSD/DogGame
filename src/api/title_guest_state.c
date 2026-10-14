/* Global state adapter for the supported XBE. See REVERSE_ENGINEERING.md
 * for provenance and the distinction between recovered names and assumptions. */
#define RECOMP_GENERATED_CODE
#include "gen/recomp_types.h"
#include "title_guest_state.h"

uint32_t title_state_menu_mode(void)
{
    return MEM32(0x1B8D9C);
}

void title_state_set_menu_mode(uint32_t value)
{
    MEM32(0x1B8D9C) = value;
}

uint32_t title_state_current_scene(void)
{
    return MEM32(0x1BF88C);
}

void title_state_set_current_scene(uint32_t value)
{
    MEM32(0x1BF88C) = value;
}

uint32_t title_state_player_powers(void)
{
    return MEM32(0x1BF2E0);
}

void title_state_set_player_powers(uint32_t value)
{
    MEM32(0x1BF2E0) = value;
}

uint32_t title_state_player_health(void)
{
    return MEM32(0x1BF010);
}

void title_state_set_player_health(uint32_t value)
{
    MEM32(0x1BF010) = value;
}

uint32_t title_state_player_model(void)
{
    return MEM32(0x1BEF4C);
}

void title_state_set_player_model(uint32_t value)
{
    MEM32(0x1BEF4C) = value;
}

uint32_t title_state_player_frame(void)
{
    return MEM32(0x1BEF60);
}

void title_state_set_player_frame(uint32_t value)
{
    MEM32(0x1BEF60) = value;
}

uint32_t title_state_active_camera(void)
{
    return MEM32(0x1BED20);
}

void title_state_set_active_camera(uint32_t value)
{
    MEM32(0x1BED20) = value;
}

uint8_t title_state_main_menu_started(void)
{
    return MEM8(0x1C2029);
}

void title_state_set_main_menu_started(uint8_t value)
{
    MEM8(0x1C2029) = value;
}

uint32_t title_state_scene_id(uint32_t scene)
{
    return MEM32(scene + 0x6C);
}

TitlePlayerPosition title_state_player_position(void)
{
    uint32_t model = title_state_player_model();
    uint32_t matrix = model ? MEM32(model + 0x20) : 0;
    TitlePlayerPosition position = {0};
    if (matrix)
    {
        position.x = MEMF(matrix + 0x30);
        position.y = MEMF(matrix + 0x34);
        position.z = MEMF(matrix + 0x38);
        position.available = 1;
    }
    return position;
}

void title_state_skip_boot_logo(void)
{
    MEM32(0x1A4F0C) = 0;
}

void title_state_select_save_slot(unsigned slot)
{
    MEM32(0x1A5224) = 0;
    MEM32(0x1A5228) = slot - 1;
}

int title_state_map_options_open(void)
{
    return MEM32(0x1A3C68) == 1;
}

/* Preserve Xbox-address lookup after named adapters replace native symbol names. */
#include "title_guest_routines.h"
#include <string.h>

uint32_t title_guest_routine_address(const char *symbol)
{
    static const struct
    {
        const char *name;
        uint32_t address;
    } routines[] = {{"title_guest_check_warp_requirement", 0x00018320u},
                    {"title_guest_check_snack_requirement", 0x0001DCC0u},
                    {"title_guest_apply_player_damage", 0x0001D970u},
                    {"title_guest_update_player_with_debug_motion", 0x00028CB0u},
                    {"title_guest_pay_gate_snacks", 0x0002AD70u},
                    {"title_guest_run_save_dialogue_frame", 0x000740D0u},
                    {"title_guest_handle_refresh_interrupt", 0x00109530u},
                    {"title_guest_run_main_menu", 0x0006CEB0u},
                    {"title_guest_clear_camera_background", 0x000BDD70u},
                    {"title_guest_run_gameplay_frame", 0x0005DA70u},
                    {"title_guest_play_menu_movie", 0x0005B620u},
                    {"title_guest_handle_helmet_contact", 0x00024BF0u},
                    {"title_guest_update_camera_projection", 0x000A0260u},
                    {"title_guest_recovered_s005_135D0", 0x000135D0u},
                    {"title_guest_recovered_s005_53960", 0x00053960u},
                    {"title_guest_recovered_w028_enemy_setup", 0x0004DFD0u},
                    {"title_guest_recovered_o008_E9E00", 0x000E9E00u},
                    {"title_guest_recovered_o008_EA2C0", 0x000EA2C0u},
                    {"title_guest_recovered_o008_525B0", 0x000525B0u},
                    {"title_guest_trace_lightning_stun_1B340", 0x0001B340u},
                    {"title_guest_trace_lightning_stun_4CF30", 0x0004CF30u},
                    {"title_guest_initialize_particle_atlas", 0x00071E80u}};

    if (!symbol)
    {
        return 0;
    }
    for (unsigned index = 0; index < sizeof routines / sizeof routines[0]; index++)
    {
        if (!strcmp(symbol, routines[index].name))
        {
            return routines[index].address;
        }
    }
    return 0;
}

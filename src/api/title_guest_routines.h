#ifndef TITLE_GUEST_ROUTINES_H
#define TITLE_GUEST_ROUTINES_H
#include <stdint.h>
/* Guest ABI functions: parameters and results are in the emulated stack and
 * registers. Do not call these from a window callback. Their original code
 * remains instruction-based where its high-level behavior is not established. */
void title_guest_check_warp_requirement(void);
void title_guest_check_snack_requirement(void);
void title_guest_apply_player_damage(void);
void title_guest_update_player_with_debug_motion(void);
void title_guest_pay_gate_snacks(void);
void title_guest_run_save_dialogue_frame(void);
void title_guest_handle_refresh_interrupt(void);
void title_guest_run_main_menu(void);
void title_guest_clear_camera_background(void);
void title_guest_run_gameplay_frame(void);
void title_guest_play_menu_movie(void);
void title_guest_handle_helmet_contact(void);
void title_guest_update_camera_projection(void);
void title_guest_recovered_s005_135D0(void);
void title_guest_recovered_s005_53960(void);
void title_guest_recovered_w028_enemy_setup(void);
void title_guest_recovered_o008_E9E00(void);
void title_guest_recovered_o008_EA2C0(void);
void title_guest_recovered_o008_525B0(void);
void title_guest_trace_lightning_stun_1B340(void);
void title_guest_trace_lightning_stun_4CF30(void);
void title_guest_initialize_particle_atlas(void);
/* Crash diagnostics can map a named adapter symbol back to its XBE address. */
uint32_t title_guest_routine_address(const char *symbol);
#endif

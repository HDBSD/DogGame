#ifndef TITLE_GUEST_CALLS_H
#define TITLE_GUEST_CALLS_H
#include "gen/recomp_types.h"

/* Original routines share TLS guest registers. Save them around host-side
 * actions; call helpers balance arguments and the synthetic return address. */
typedef struct GuestRegisters
{
    uint32_t registers[9];
    RecompXmm simd_registers[8];
    double fpu_stack[8];
    int fpu_top, fpu_comparison, direction_flag;
    uint16_t fpu_control, fpu_condition;
} GuestRegisters;

GuestRegisters save_guest_registers(void);
void restore_guest_registers(const GuestRegisters *saved);
uint32_t call_guest_no_arguments(void (*function)(void));
uint32_t call_guest_one_argument(void (*function)(void), uint32_t argument);
void call_guest_player_update(void (*function)(void), uint32_t player, uint32_t scene, uint32_t elapsed_bits);
uint32_t read_scene_string(uint32_t address);
/* Caller preserves registers. This follows the native portal cleanup/load/setup order. */
void title_guest_warp_to_scene(uint32_t scene_id);
uint32_t title_guest_prepare_save_slots(void);
uint32_t title_guest_restore_selected_save(void);
uint32_t title_guest_start_new_game(void);
void title_guest_set_mode(uint32_t value);
void title_guest_set_gameplay_state(uint32_t value);
void title_guest_grant_snacks(uint32_t value);
int title_guest_is_menu_movie_caller(uint32_t caller);
int title_guest_is_new_game_movie_caller(uint32_t caller);
int title_guest_movie_is_initial_cinematic(uint32_t movie);
uint32_t title_guest_restored_scene_id(void);
void title_guest_set_result(uint32_t value);
#endif

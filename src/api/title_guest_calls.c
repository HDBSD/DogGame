/* ABI adapter. Evidence: the original title wrappers and generated calling
 * convention; register and stack contracts are documented in REVERSE_ENGINEERING.md. */
#define RECOMP_GENERATED_CODE
#include "title_guest_calls.h"
#include "gen/recomp_funcs.h"
#include "title_display.h"
#include "title_guest_state.h"
#include <string.h>

GuestRegisters save_guest_registers(void)
{
    GuestRegisters saved = {{eax, ecx, edx, esp, ebx, esi, edi, g_ebp, g_seh_ebp},
                            {xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7}};
    memcpy(saved.fpu_stack, g_fp_stack, sizeof saved.fpu_stack);
    saved.fpu_top = g_fp_top;
    saved.fpu_comparison = g_fp_cmp;
    saved.fpu_control = g_fp_control_word;
    saved.fpu_condition = g_fp_cc;
    saved.direction_flag = g_df;
    return saved;
}

void restore_guest_registers(const GuestRegisters *saved)
{
    eax = saved->registers[0];
    ecx = saved->registers[1];
    edx = saved->registers[2];
    esp = saved->registers[3];
    ebx = saved->registers[4];
    esi = saved->registers[5];
    edi = saved->registers[6];
    g_ebp = saved->registers[7];
    g_seh_ebp = saved->registers[8];
    xmm0 = saved->simd_registers[0];
    xmm1 = saved->simd_registers[1];
    xmm2 = saved->simd_registers[2];
    xmm3 = saved->simd_registers[3];
    xmm4 = saved->simd_registers[4];
    xmm5 = saved->simd_registers[5];
    xmm6 = saved->simd_registers[6];
    xmm7 = saved->simd_registers[7];
    memcpy(g_fp_stack, saved->fpu_stack, sizeof saved->fpu_stack);
    g_fp_top = saved->fpu_top;
    g_fp_cmp = saved->fpu_comparison;
    g_fp_control_word = saved->fpu_control;
    g_fp_cc = saved->fpu_condition;
    g_df = saved->direction_flag;
}

/* Guest callees consume the synthetic return address. Restore the host
 * caller's guest stack after collecting EAX; outer wrappers save registers. */
uint32_t call_guest_no_arguments(void (*fn)(void))
{
    uint32_t stack = esp;
    PUSH32(esp, 0);
    fn();
    uint32_t result = eax;
    esp = stack;
    return result;
}

uint32_t call_guest_one_argument(void (*fn)(void), uint32_t a)
{
    uint32_t stack = esp;
    PUSH32(esp, a);
    PUSH32(esp, 0);
    fn();
    uint32_t result = eax;
    esp = stack;
    return result;
}

uint32_t title_display_find_asset(uint32_t id)
{
    /* xSTFindAsset(id, sizeOut); 9CA10 is the asset-name lookup. */
    GuestRegisters saved = save_guest_registers();
    PUSH32(esp, 0);
    uint32_t result = call_guest_one_argument(sub_0009CAE0, id);
    restore_guest_registers(&saved);
    return result;
}

void title_menu_event(uint32_t item, uint32_t event)
{
    GuestRegisters saved = save_guest_registers();
    PUSH32(esp, event);
    PUSH32(esp, item);
    PUSH32(esp, item);
    PUSH32(esp, 0);
    sub_00014220();
    restore_guest_registers(&saved);
}

uint32_t read_scene_string(uint32_t va)
{
    return ((uint32_t)MEM8(va) << 24) | ((uint32_t)MEM8(va + 1) << 16) | ((uint32_t)MEM8(va + 2) << 8) |
           MEM8(va + 3);
}

void call_guest_player_update(void (*fn)(void), uint32_t player, uint32_t scene, uint32_t dt)
{
    PUSH32(esp, dt);
    PUSH32(esp, scene);
    PUSH32(esp, player);
    PUSH32(esp, 0);
    fn();
    esp += 12;
}

void title_guest_warp_to_scene(uint32_t scene_id)
{
    uint32_t powers = title_state_player_powers();
    /* Same cleanup/load/setup sequence as the title's portal transition. */
    call_guest_one_argument(sub_0005F3A0, 11);
    call_guest_no_arguments(sub_0005CAF0);
    MEM32(0x1B8B50) = 0;
    MEM32(0x1B8B54) = 0;
    MEM32(0x1B8B5C) = 0;
    call_guest_one_argument(sub_0005D2A0, scene_id);
    call_guest_no_arguments(sub_0005D330);
    uint32_t clock_low = call_guest_no_arguments(sub_000ADFA0);
    uint64_t game_clock = (((uint64_t)edx << 32) | clock_low) - 16;
    MEM32(0x1B8B68) = (uint32_t)game_clock;
    MEM32(0x1B8B6C) = (uint32_t)(game_clock >> 32);
    /* State 1 continues gameplay; state 0 exits the loop back to the menu. */
    call_guest_one_argument(sub_0005F2F0, 1);
    title_state_set_player_powers(powers);
}

uint32_t title_guest_prepare_save_slots(void)
{
    return call_guest_no_arguments(sub_00075090);
}

uint32_t title_guest_restore_selected_save(void)
{
    return call_guest_no_arguments(sub_00075810);
}

uint32_t title_guest_start_new_game(void)
{
    return call_guest_no_arguments(sub_0007CB10);
}

void title_guest_set_mode(uint32_t value)
{
    call_guest_one_argument(sub_0005F3A0, value);
}

void title_guest_set_gameplay_state(uint32_t value)
{
    call_guest_one_argument(sub_0005F2F0, value);
}

void title_guest_grant_snacks(uint32_t value)
{
    call_guest_one_argument(sub_0001DC90, value);
}

int title_guest_is_menu_movie_caller(uint32_t caller)
{
    return caller == 0x0006CE45u || caller == 0x0005B6F5u;
}

int title_guest_is_new_game_movie_caller(uint32_t caller)
{
    return caller == 0x000122C9u;
}

int title_guest_movie_is_initial_cinematic(uint32_t movie)
{
    return movie && !_stricmp((const char *)XBOX_PTR(movie), "cin1");
}

uint32_t title_guest_restored_scene_id(void)
{
    return read_scene_string(0x1A5250);
}

void title_guest_set_result(uint32_t value)
{
    eax = value;
}

/* Guest-address entry points used by generated dispatch. The named adapter
 * operations describe each maintained hook; recovered originals and their
 * instruction-level evidence live in the guest implementation. */
#include "api/title_guest_routines.h"

void sub_000740D0(void)
{
    title_guest_run_save_dialogue_frame();
}

void sub_00109530(void)
{
    title_guest_handle_refresh_interrupt();
}

void sub_0006CEB0(void)
{
    title_guest_run_main_menu();
}

void sub_000BDD70(void)
{
    title_guest_clear_camera_background();
}

void sub_0005DA70(void)
{
    title_guest_run_gameplay_frame();
}

void sub_0005B620(void)
{
    title_guest_play_menu_movie();
}

void sub_00024BF0(void)
{
    title_guest_handle_helmet_contact();
}

void sub_000A0260(void)
{
    title_guest_update_camera_projection();
}

void sub_000135D0(void)
{
    title_guest_recovered_s005_135D0();
}

void sub_00053960(void)
{
    title_guest_recovered_s005_53960();
}

void sub_0004DFD0(void)
{
    title_guest_recovered_w028_enemy_setup();
}

void sub_000E9E00(void)
{
    title_guest_recovered_o008_E9E00();
}

void sub_000EA2C0(void)
{
    title_guest_recovered_o008_EA2C0();
}

void sub_000525B0(void)
{
    title_guest_recovered_o008_525B0();
}

void sub_0001B340(void)
{
    title_guest_trace_lightning_stun_1B340();
}

void sub_0004CF30(void)
{
    title_guest_trace_lightning_stun_4CF30();
}

void sub_00071E80(void)
{
    title_guest_initialize_particle_atlas();
}

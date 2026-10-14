/* Guest-address entry points used by generated dispatch. The named adapter
 * operations describe each maintained hook; recovered originals and their
 * instruction-level evidence live in the guest implementation. */
#include "api/title_guest_routines.h"

void sub_00018320(void)
{
    title_guest_check_warp_requirement();
}

void sub_0001DCC0(void)
{
    title_guest_check_snack_requirement();
}

void sub_0001D970(void)
{
    title_guest_apply_player_damage();
}

void sub_00028CB0(void)
{
    title_guest_update_player_with_debug_motion();
}

void sub_0002AD70(void)
{
    title_guest_pay_gate_snacks();
}

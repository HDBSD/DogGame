#include "title_guest_state.h"
/* Player adapter: native animation continues during flight, while movement
 * bypasses collision and clears accumulated velocity. Source evidence is
 * recorded in REVERSE_ENGINEERING.md; do not infer damage types from a caller alone. */
#define RECOMP_GENERATED_CODE
#include "title_guest_player.h"
#include "title_guest_calls.h"
#include "gen/recomp_funcs.h"

uint32_t title_guest_player_frame(uint32_t player)
{
    return MEM32(player + 0x40);
}

uint32_t title_guest_player_model(uint32_t player)
{
    return MEM32(player + 0x2C);
}

int title_guest_damage_is_from_monster(uint32_t source, uint32_t caller)
{
    /* Proven NPC contact/attack calls into the common player damage routine.
  * Sources can be attack records rather than xBase entities, so checking an
  * assumed type byte alone misses both regular monsters and boss attacks. */
    switch (caller)
    {
        case 0x0002C19A:
        case 0x0002C28D: /* NPC contacts in player update */
        case 0x0002F791:
        case 0x0002FB22:
        case 0x0002FDB4:
        case 0x0003D21A:
        case 0x0003D545:
        case 0x0003D5E5:
        case 0x0003D685:
        case 0x000405FA:
        case 0x00040A13:
        case 0x00044970:
        case 0x0006A05D:
            return 1;
    }
    /* Live NPC body header has xBase type 2 (confirmed through both native
  * NPC contact and the generic damage wrapper at 28E80). Spawned enemy
  * attack entities are initialized as type 0x19 at 2E8EE. The generic
  * wrapper also serves hazards, so do not block it by caller alone. */
    if (source < 0x10000 || source >= 0x08000000)
    {
        return 0;
    }
    unsigned type = MEM8(source + 4);
    return type == 2 || type == 0x19;
}

void title_guest_clear_player_motion(void)
{
    uint32_t frame = title_state_player_frame();
    if (!frame)
    {
        return;
    }
    for (unsigned i = 0; i < 3; i++)
    {
        MEMF(frame + 0xE0 + i * 4) = MEMF(frame + 0x8C + i * 4) = 0;
        MEMF(frame + 0xC8 + i * 4) = 0;
    }
    MEM32(0x1BF094) = 0; /* clear the old floor/platform association */
    MEM32(0x1BF028) = 0;
    MEM32(0x1BF034) = 0;
}

void title_guest_apply_player_flight(uint32_t player, uint32_t scene, uint32_t frame, uint32_t dt_bits,
                                     float delta_x, float delta_y, float delta_z)
{
    GuestRegisters saved = save_guest_registers();
    call_guest_player_update(sub_000860C0, player, scene, dt_bits); /* native model/animation update */
    for (unsigned i = 0; i < 3; i++)
    {
        float delta = i == 0 ? delta_x : i == 1 ? delta_y : delta_z;
        MEMF(frame + 0x30 + i * 4) += delta;
        MEMF(frame + 0xC8 + i * 4) = delta;
        MEMF(frame + 0xE0 + i * 4) = MEMF(frame + 0x8C + i * 4) = 0;
    }
    MEM32(frame + 0xEC) = 0; /* allow native end-update to copy position/rotation */
    uint32_t collision = MEM32(player + 0x44);
    if (collision)
    {
        MEM8(collision + 0xB) = 0;
    }
    call_guest_player_update(sub_000861A0, player, scene, dt_bits); /* model matrices and bounds */
    restore_guest_registers(&saved);
}

TitlePlayerUpdate title_guest_current_player_update(void)
{
    TitlePlayerUpdate update = {MEM32(esp + 4), MEM32(esp + 8), MEM32(esp + 12), MEMF(esp + 12)};
    return update;
}

int title_guest_is_main_player(uint32_t player)
{
    return player == 0x1BEF20;
}

void title_guest_horizontal_camera_basis(float *right_x, float *right_z)
{
    *right_x = MEMF(0x1BEDE0);
    *right_z = MEMF(0x1BEDE8);
}

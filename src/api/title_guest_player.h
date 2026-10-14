#ifndef TITLE_GUEST_PLAYER_H
#define TITLE_GUEST_PLAYER_H
#include <stdint.h>

/* Title-thread API. Player addresses must already belong to the current scene. */
int title_guest_damage_is_from_monster(uint32_t source, uint32_t caller);
void title_guest_clear_player_motion(void);
uint32_t title_guest_player_frame(uint32_t player);
uint32_t title_guest_player_model(uint32_t player);
void title_guest_apply_player_flight(uint32_t player, uint32_t scene, uint32_t frame, uint32_t elapsed_bits,
                                     float delta_x, float delta_y, float delta_z);

typedef struct TitlePlayerUpdate
{
    uint32_t player, scene, elapsed_bits;
    float elapsed;
} TitlePlayerUpdate;

TitlePlayerUpdate title_guest_current_player_update(void);
int title_guest_is_main_player(uint32_t player);
void title_guest_horizontal_camera_basis(float *right_x, float *right_z);
#endif

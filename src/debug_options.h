#ifndef SD_DEBUG_OPTIONS_H
#define SD_DEBUG_OPTIONS_H
#include <stdint.h>
#include <stddef.h>

typedef struct TitleOptions
{
    int debug, nofmv, noaudio, save_slot, trace, profiler, fps_graph, help;
    char level[5];
} TitleOptions;

extern TitleOptions title_options;
int title_options_parse(int argc, const wchar_t *const *argv, char *error, unsigned capacity);
int title_options_init(void);
uint32_t title_level_id(const char *name);
int title_debug_boot(void);
void title_debug_init(void);
void title_debug_frame(void);
void title_debug_menu_frame(void);
int title_skip_movie(uint32_t caller, uint32_t movie);
int title_debug_monster_damage(uint32_t source, uint32_t caller);
void title_debug_damage_trace(uint32_t source, uint32_t caller, uint32_t amount, int blocked);
int title_debug_player_flight(void);
void title_debug_noclip_reset(void);
int title_debug_warp_unlocked(void);
int title_debug_snacks_unlocked(void);
#endif

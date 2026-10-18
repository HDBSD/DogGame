#ifndef TITLE_GUEST_MENU_H
#define TITLE_GUEST_MENU_H

#include <stdint.h>

/* Title modes and press bits, not Win32 keys or XInput button values. */
typedef enum TitleMenuMode
{
    TITLE_MENU_MAIN = 2,
    TITLE_MENU_MAP = 7
} TitleMenuMode;

typedef enum TitleMenuButton
{
    BUTTON_START = 0x00000001u,
    BUTTON_BACK = 0x00020000u /* The game's B action is used for menu Back. */
} TitleMenuButton;

/* Call on the title thread, after the game has updated controller input.
 * Remapping changes this frame's press edges; held buttons and host keys stay intact. */
uint32_t title_menu_current_mode(void);
int title_menu_map_page_visible(uint32_t scene, uint32_t menu_mode);
int menu_accepts_back(uint32_t menu_mode);
void remap_controller_press_edges(TitleMenuButton source, TitleMenuButton destination);
/* Title thread, immediately after controller polling. Queues the native Start
 * press in player one's decoded input; returns zero for an unreadable pad.
 * Does not change held input or force a menu mode. Caller gates gameplay. */
int title_menu_request_pause(void);

/* Observation only, title thread. Copies the current decoded player-one state;
 * not an atomic snapshot of the asynchronous USB/XAPI transport. No mutations.
 * Returns zero for an unreadable controller pointer. */
typedef struct TitleMenuInputSnapshot
{
    uint32_t mode;
    uint32_t held;
    uint32_t pressed;
    float main_elapsed;
    float save_elapsed;
    float ui_minimum_elapsed;
} TitleMenuInputSnapshot;
int title_menu_input_snapshot(TitleMenuInputSnapshot *snapshot);

#endif

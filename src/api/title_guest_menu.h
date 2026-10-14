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

#endif

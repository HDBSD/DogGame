/* Guest layout details for menu input. See README.md for the evidence and
 * recovery workflow; these addresses apply to the supported XBE revision. */
#define RECOMP_GENERATED_CODE
#include "gen/recomp_types.h"
#include "title_guest_menu.h"

enum
{
    GUEST_ADDRESS_MINIMUM = 0x00010000,
    GUEST_MEMORY_END = 0x04000000,
    MENU_MODE_ADDRESS = 0x001B8D9C,
    MAIN_MENU_STARTED_ADDRESS = 0x001C2029,
    CURRENT_SCENE_ADDRESS = 0x001BF88C,
    CONTROLLER_TABLE_ADDRESS = 0x001BEF10,
    CONTROLLER_COUNT = 4,
    GUEST_POINTER_SIZE = 4,
    CONTROLLER_BYTES_REQUIRED = 0x34,
    CONTROLLER_HELD_OFFSET = 0x2C,
    CONTROLLER_PRESSED_OFFSET = 0x30,
    MAIN_MENU_ELAPSED_ADDRESS = 0x001A4F14,
    SAVE_DIALOGUE_ELAPSED_ADDRESS = 0x001A5220,
    UI_MINIMUM_ELAPSED_ADDRESS = 0x0018131C,
    SCENE_BYTES_REQUIRED = 0x1B0,
    SCENE_FONT_COUNT_OFFSET = 0x100,
    SCENE_FONT_ITEMS_OFFSET = 0x1AC,
    FONT_ITEM_SIZE = 0x100,
    FONT_ITEM_FLAGS_OFFSET = 0x14,
    FONT_ITEM_VISIBLE = 1,
    MAXIMUM_FONT_ITEMS = 4096
};

/* Identity of the map's Back listener; its original asset name is unrecorded. */
static const uint32_t MAP_BACK_LISTENER_ID = 0xDF911682u;

static int guest_object_readable(uint32_t address, uint32_t bytes_required)
{
    /* Preserve the existing conservative upper bound, including its strict
     * inequality. These checks validate the guest range, not host pointers. */
    return bytes_required < GUEST_MEMORY_END && address >= GUEST_ADDRESS_MINIMUM &&
           address < GUEST_MEMORY_END - bytes_required;
}

uint32_t title_menu_current_mode(void)
{
    return MEM32(MENU_MODE_ADDRESS);
}

int title_menu_request_pause(void)
{
    uint32_t controller = MEM32(CONTROLLER_TABLE_ADDRESS);
    if (!guest_object_readable(controller, CONTROLLER_BYTES_REQUIRED))
    {
        return 0;
    }
    MEM32(controller + CONTROLLER_PRESSED_OFFSET) |= BUTTON_START;
    return 1;
}

int title_menu_map_page_visible(uint32_t scene, uint32_t menu_mode)
{
    if (menu_mode != TITLE_MENU_MAP || !guest_object_readable(scene, SCENE_BYTES_REQUIRED))
    {
        return 0;
    }

    unsigned item_count = MEM32(scene + SCENE_FONT_COUNT_OFFSET);
    uint32_t items = MEM32(scene + SCENE_FONT_ITEMS_OFFSET);
    if (item_count > MAXIMUM_FONT_ITEMS || !guest_object_readable(items, 0) ||
        item_count * FONT_ITEM_SIZE > GUEST_MEMORY_END - items)
    {
        return 0;
    }

    for (unsigned index = 0; index < item_count; index++)
    {
        uint32_t item = items + index * FONT_ITEM_SIZE;
        if (MEM32(item) == MAP_BACK_LISTENER_ID && (MEM8(item + FONT_ITEM_FLAGS_OFFSET) & FONT_ITEM_VISIBLE))
        {
            return 1;
        }
    }
    return 0;
}

int menu_accepts_back(uint32_t menu_mode)
{
    /* Start must still enter the title screen. Once accepted, the main menu
     * uses Back; the map uses it only while its Back listener is visible.
     * Listener visibility also covers nested warp pages. */
    if (menu_mode == TITLE_MENU_MAIN)
    {
        return MEM8(MAIN_MENU_STARTED_ADDRESS) != 0;
    }
    if (menu_mode == TITLE_MENU_MAP)
    {
        return title_menu_map_page_visible(MEM32(CURRENT_SCENE_ADDRESS), menu_mode);
    }
    return 0;
}

void remap_controller_press_edges(TitleMenuButton source, TitleMenuButton destination)
{
    for (unsigned index = 0; index < CONTROLLER_COUNT; index++)
    {
        uint32_t controller = MEM32(CONTROLLER_TABLE_ADDRESS + index * GUEST_POINTER_SIZE);
        if (!guest_object_readable(controller, CONTROLLER_BYTES_REQUIRED))
        {
            continue;
        }

        uint32_t pressed = MEM32(controller + CONTROLLER_PRESSED_OFFSET);
        if (pressed & (uint32_t)source)
        {
            MEM32(controller + CONTROLLER_PRESSED_OFFSET) =
                (pressed & ~(uint32_t)source) | (uint32_t)destination;
        }
    }
}

int title_menu_input_snapshot(TitleMenuInputSnapshot *snapshot)
{
    if (!snapshot)
    {
        return 0;
    }
    snapshot->mode = title_menu_current_mode();
    /* Main/save wrappers pass these deltas to sub_000791B0. Its entry at
     * 0x000791B0 compares against 0x0018131C and returns at 0x000794EB
     * when elapsed <= that threshold. This diagnoses the gate; it does not
     * assert that a skipped update explains a particular user's trace. */
    snapshot->main_elapsed = MEMF(MAIN_MENU_ELAPSED_ADDRESS);
    snapshot->save_elapsed = MEMF(SAVE_DIALOGUE_ELAPSED_ADDRESS);
    snapshot->ui_minimum_elapsed = MEMF(UI_MINIMUM_ELAPSED_ADDRESS);
    snapshot->held = snapshot->pressed = 0;
    uint32_t controller = MEM32(CONTROLLER_TABLE_ADDRESS);
    if (!guest_object_readable(controller, CONTROLLER_BYTES_REQUIRED))
    {
        return 0;
    }
    /* sub_00090F50, loc_00091192: pressed = new & ~old; +0x2C
     * receives new held state and +0x30 receives pressed, both dword stores.
     * Evidence is the existing translation, not a fresh Ghidra recovery. */
    snapshot->held = MEM32(controller + CONTROLLER_HELD_OFFSET);
    snapshot->pressed = MEM32(controller + CONTROLLER_PRESSED_OFFSET);
    return 1;
}

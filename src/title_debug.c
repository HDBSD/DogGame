/* Title-only debugging. Guest calls execute on the title thread, never the UI thread. */
#define RECOMP_GENERATED_CODE
#include "gen/recomp_types.h"
#include "gen/recomp_funcs.h"
#include "debug_options.h"
#include "title_display.h"
#include "api/title_guest_calls.h"
#include "api/title_guest_player.h"
#include "api/title_guest_state.h"
#include "title_debug_geometry.h"
#include <windows.h>
#include <Xinput.h>
#include <xbox/debug_overlay.h>
#include <xbox/display_options.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Commands cross the window/title thread boundary. Their names describe
 * actions; their numeric values are retained for the existing queue protocol. */
typedef enum TitleDebugAction
{
    DEBUG_QUEUE_WRITING = -1,
    DEBUG_ACTION_NONE = 0,
    DEBUG_ACTION_WARP_TO_SCENE = 1,
    DEBUG_ACTION_TOGGLE_POWER = 2,
    DEBUG_ACTION_TRACE_FRAME = 3,
    DEBUG_ACTION_PROFILE_RENDERER = 4,
    DEBUG_ACTION_TOGGLE_FPS_GRAPH = 5,
    DEBUG_ACTION_TRACE_TEN_SECONDS = 6,
    DEBUG_ACTION_TOGGLE_WARP_REQUIREMENT = 7,
    DEBUG_ACTION_TOGGLE_MONSTER_PROTECTION = 8,
    DEBUG_ACTION_TOGGLE_FLIGHT = 9,
    DEBUG_ACTION_TOGGLE_SNACK_REQUIREMENT = 10,
    DEBUG_ACTION_GRANT_SNACKS = 11,
    DEBUG_ACTION_TOGGLE_TRIGGERS = 12,
    DEBUG_ACTION_TOGGLE_COLLISION = 13
} TitleDebugAction;

static volatile LONG s_pause, s_open, s_selection, s_level_selection, s_power_selection;
static volatile LONG s_command, s_argument, s_power_bits, s_level;
static char s_levels[128][5];
static unsigned s_level_count;
static const char *s_powers[] = {"Bubble gum",     "Soap bubbles",    "Sticky boots", "Plungers",
                                 "Slippers",       "Lampshade",       "Black Knight", "Spring / double jump",
                                 "Lightning bolt", "Football helmet", "Umbrella",     "Shovel",
                                 "Lightning stun"};
/* Display indices deliberately differ from the game's original ability bits. */
static const unsigned s_power_bit[] = {0, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14};

enum
{
    POWER_COUNT = 13,
    DEBUG_ROWS = 14
};

volatile LONG title_debug_ignore_warp, title_debug_ignore_snacks;
volatile LONG title_debug_monster_safe, title_debug_noclip;
static DWORD s_trace_until, s_profile_until;
static volatile LONG s_pending_capture;
static DWORD s_capture_until;
static WORD s_last_buttons;

static void queue_action(TitleDebugAction action, LONG argument)
{
    /* One pending action; UI commands are serialized on the window thread. */
    if (InterlockedCompareExchange(&s_command, DEBUG_QUEUE_WRITING, DEBUG_ACTION_NONE))
    {
        return;
    }
    InterlockedExchange(&s_argument, argument);
    InterlockedExchange(&s_command, action);
}

static int level_compare(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

static void discover_levels(void)
{
    WIN32_FIND_DATAA dir, file;
    HANDLE dirs = FindFirstFileA("game/*", &dir);
    if (dirs == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        if (!(dir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || strlen(dir.cFileName) != 2 ||
            dir.cFileName[1] != '0')
        {
            continue;
        }
        char path[MAX_PATH];
        snprintf(path, sizeof path, "game/%s/*.HIP", dir.cFileName);
        HANDLE files = FindFirstFileA(path, &file);
        if (files == INVALID_HANDLE_VALUE)
        {
            continue;
        }
        do
        {
            if (strlen(file.cFileName) != 8 || s_level_count >= 128)
            {
                continue;
            }
            char id[5];
            for (int i = 0; i < 4; i++)
            {
                char c = file.cFileName[i];
                id[i] = (c >= 'a' && c <= 'z') ? c - 32 : c;
            }
            id[4] = 0;
            if (id[0] < 'A' || id[0] > 'Z' || id[1] < '0' || id[1] > '9' || id[2] < '0' || id[2] > '9' ||
                id[3] < '0' || id[3] > '9')
            {
                continue;
            }
            memcpy(s_levels[s_level_count++], id, 5);
        } while (FindNextFileA(files, &file));
        FindClose(files);
    } while (FindNextFileA(dirs, &dir));
    FindClose(dirs);
    qsort(s_levels, s_level_count, 5, level_compare);
}

static void close_debug(void)
{
    InterlockedExchange(&s_open, 0);
    s_capture_until = GetTickCount() + 250;
}

static int debug_key(unsigned key)
{
    if (!s_pause)
    {
        return 0;
    }
    if (key == VK_F1)
    {
        if (s_open)
        {
            close_debug();
        }
        else
        {
            for (unsigned i = 0; i < s_level_count; i++)
            {
                if (title_level_id(s_levels[i]) == (uint32_t)s_level)
                {
                    s_level_selection = i;
                    break;
                }
            }
            s_open = 1;
            xbox_DebugOverlayCapture(1);
        }
        return 1;
    }
    if (!s_open)
    {
        return 0;
    }
    if (key == VK_SPACE)
    {
        key = VK_RETURN;
    }
    else if (key == 'L')
    {
        key = VK_ESCAPE;
    }
    else if (key == 'W')
    {
        key = VK_UP;
    }
    else if (key == 'S')
    {
        key = VK_DOWN;
    }
    else if (key == 'A')
    {
        key = VK_LEFT;
    }
    else if (key == 'D')
    {
        key = VK_RIGHT;
    }
    LONG row = s_selection;
    if (key == VK_ESCAPE)
    {
        close_debug();
    }
    else if (key == VK_UP)
    {
        s_selection = (row + DEBUG_ROWS - 1) % DEBUG_ROWS;
    }
    else if (key == VK_DOWN)
    {
        s_selection = (row + 1) % DEBUG_ROWS;
    }
    else if (key == VK_LEFT || key == VK_RIGHT)
    {
        int delta = key == VK_RIGHT ? 1 : -1;
        if (row == 0 && s_level_count)
        {
            s_level_selection = (s_level_selection + (int)s_level_count + delta) % s_level_count;
        }
        if (row == 1)
        {
            s_power_selection = (s_power_selection + POWER_COUNT + delta) % POWER_COUNT;
        }
    }
    else if (key == VK_RETURN)
    {
        if (row == 0 && s_level_count)
        {
            queue_action(DEBUG_ACTION_WARP_TO_SCENE, (LONG)title_level_id(s_levels[s_level_selection]));
            close_debug();
        }
        else if (row == 1)
        {
            queue_action(DEBUG_ACTION_TOGGLE_POWER, s_power_bit[s_power_selection]);
        }
        else if (row == 2)
        {
            queue_action(DEBUG_ACTION_TRACE_FRAME, 0);
        }
        else if (row == 3)
        {
            queue_action(DEBUG_ACTION_PROFILE_RENDERER, 0);
        }
        else if (row == 4)
        {
            queue_action(DEBUG_ACTION_TRACE_TEN_SECONDS, 0);
        }
        else if (row == 5)
        {
            queue_action(DEBUG_ACTION_TOGGLE_FPS_GRAPH, 0);
        }
        else if (row >= 6 && row <= 10)
        {
            static const TitleDebugAction row_actions[] = {
                DEBUG_ACTION_TOGGLE_WARP_REQUIREMENT, DEBUG_ACTION_TOGGLE_MONSTER_PROTECTION,
                DEBUG_ACTION_TOGGLE_FLIGHT, DEBUG_ACTION_TOGGLE_SNACK_REQUIREMENT, DEBUG_ACTION_GRANT_SNACKS};
            queue_action(row_actions[row - 6], 0);
        }
        else if (row == 11)
        {
            queue_action(DEBUG_ACTION_TOGGLE_TRIGGERS, 0);
        }
        else if (row == 12)
        {
            queue_action(DEBUG_ACTION_TOGGLE_COLLISION, 0);
        }
        else
        {
            close_debug();
        }
    }
    return 1;
}

static void draw_debug_text(HDC dc, int x, int y, const char *s)
{
    TextOutA(dc, x, y, s, (int)strlen(s));
}

static void debug_draw(void *device, unsigned width, unsigned height)
{
    HDC dc = (HDC)device;
    XINPUT_STATE pad;
    /* Back opens the debug page only while paused; capture all guest input until
  * the closing button has been released to avoid dismissing the native menu. */
    if (XInputGetState(0, &pad) == ERROR_SUCCESS)
    {
        WORD b = pad.Gamepad.wButtons, edge = b & ~s_last_buttons;
        s_last_buttons = b;
        if (s_pause && (edge & XINPUT_GAMEPAD_BACK))
        {
            debug_key(VK_F1);
        }
        if (s_open)
        {
            if (edge & XINPUT_GAMEPAD_DPAD_UP)
            {
                debug_key(VK_UP);
            }
            if (edge & XINPUT_GAMEPAD_DPAD_DOWN)
            {
                debug_key(VK_DOWN);
            }
            if (edge & XINPUT_GAMEPAD_DPAD_LEFT)
            {
                debug_key(VK_LEFT);
            }
            if (edge & XINPUT_GAMEPAD_DPAD_RIGHT)
            {
                debug_key(VK_RIGHT);
            }
            if (edge & XINPUT_GAMEPAD_A)
            {
                debug_key(VK_RETURN);
            }
            if (edge & XINPUT_GAMEPAD_B)
            {
                debug_key(VK_ESCAPE);
            }
        }
        if (!s_open && GetTickCount() - s_capture_until < 0x80000000u &&
            !(b & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_BACK)))
        {
            xbox_DebugOverlayCapture(0);
        }
    }
    else if (!s_open && GetTickCount() - s_capture_until < 0x80000000u)
    {
        xbox_DebugOverlayCapture(0);
    }
    if (s_pending_capture)
    {
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255, 230, 70));
        draw_debug_text(dc, 12, 62, "Capture armed: close Pause to start recording gameplay");
    }
    DWORD now = GetTickCount();
    if ((s_trace_until && (LONG)(s_trace_until - now) > 0) ||
        (s_profile_until && (LONG)(s_profile_until - now) > 0))
    {
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255, 230, 70));
        draw_debug_text(dc, 12, 62,
                        (s_trace_until && (LONG)(s_trace_until - now) > 0)
                            ? "Render trace recording -> debug-render.log"
                            : "Renderer profile recording -> debug-render.log");
    }
    if (!s_pause)
    {
        if (s_open)
        {
            close_debug();
        }
        return;
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 230, 70));
    if (!s_open)
    {
        RECT r = {12, (LONG)height - 38, 300, (LONG)height - 10};
        FillRect(dc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
        draw_debug_text(dc, 20, (int)height - 33, "Debug  [F1 / controller Back]");
        return;
    }
    RECT r = {40, 158, (LONG)width - 40, (LONG)height - 10};
    FillRect(dc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    draw_debug_text(dc, 58, 164, "DEBUG MENU");
    SetTextColor(dc, RGB(190, 190, 190));
    draw_debug_text(dc, 58, 185, "Arrows / D-pad: select   Enter / A: apply   Esc / B: back");
    char rows[DEBUG_ROWS][160];
    snprintf(rows[0], 160, "Warp to level: %s  [left / right]",
             s_level_count ? s_levels[s_level_selection] : "No levels found");
    snprintf(rows[1], 160, "Power-up: %s = %s  [left / right]", s_powers[s_power_selection],
             (s_power_bits & (1 << s_power_bit[s_power_selection])) ? "ON" : "OFF");
    snprintf(rows[2], 160, "Trace next frame");
    snprintf(rows[3], 160, "Capture 10 second renderer profile");
    snprintf(rows[4], 160, "Capture 10 second render trace (slow)");
    snprintf(rows[5], 160, "FPS graph / level / coordinates: %s", title_options.fps_graph ? "ON" : "OFF");
    snprintf(rows[6], 160, "Ignore warp gate requirements: %s", title_debug_ignore_warp ? "ON" : "OFF");
    snprintf(rows[7], 160, "Undamageable by monsters: %s", title_debug_monster_safe ? "ON" : "OFF");
    snprintf(rows[8], 160, "Scooby noclip: %s  [Q/E or LT/RT: down/up]", title_debug_noclip ? "ON" : "OFF");
    snprintf(rows[9], 160, "Ignore snack gate requirements: %s", title_debug_ignore_snacks ? "ON" : "OFF");
    snprintf(rows[10], 160, "Add 100 snacks");
    snprintf(rows[11], 160, "Show triggers: %s", title_debug_geometry_enabled(0) ? "ON" : "OFF");
    snprintf(rows[12], 160, "World collision view: %s", title_debug_geometry_enabled(1) ? "ON" : "OFF");
    snprintf(rows[13], 160, "Return to pause menu");
    int visible = ((int)height - 230) / 21;
    if (visible < 1)
    {
        visible = 1;
    }
    if (visible > DEBUG_ROWS)
    {
        visible = DEBUG_ROWS;
    }
    int first = s_selection >= visible ? s_selection - visible + 1 : 0;
    for (int j = 0; j < visible && first + j < DEBUG_ROWS; j++)
    {
        int i = first + j;
        SetTextColor(dc, i == s_selection ? RGB(255, 230, 70) : RGB(240, 240, 240));
        draw_debug_text(dc, 58, 210 + j * 21, rows[i]);
    }
}

static void setup_debug(void)
{
    static int done;
    if (done || !title_options.debug)
    {
        return;
    }
    done = 1;
    discover_levels();
    xbox_DebugOverlayRegister(debug_draw, debug_key);
    xbox_DebugOverlayGraph(title_options.fps_graph);
    title_debug_geometry_init();
    fprintf(stderr, "[DEBUG] Controls enabled; FPS graph=%d; %u levels available.\n", title_options.fps_graph,
            s_level_count);
    if (title_options.trace)
    {
        nv2a_debug_trace_request();
    }
    if (title_options.profiler)
    {
        nv2a_debug_profile_request();
        s_profile_until = GetTickCount() + 10000;
    }
}

void title_debug_init(void)
{
    title_display_init();
    setup_debug();
}

void title_debug_menu_frame(void)
{
    title_display_frame();
    static unsigned frames;
    static int started;
    /* Let scene/UI setup and a few input polls complete before pressing Start. */
    if (title_options.nofmv && !started && title_state_menu_mode() == 2 && ++frames >= 4)
    {
        xbox_InputPulseStart();
        started = 1;
    }
}

int title_skip_movie(uint32_t caller, uint32_t movie)
{
    /* This is the menu movie wrapper, not in-game scripted cutscenes. */
    if ((title_options.nofmv || title_options.level[0] || title_options.save_slot) &&
        title_guest_is_menu_movie_caller(caller))
    {
        return 1;
    }
    /* H001's initial cinematic is a new-game boot event. A direct-level debug
  * launch skips this one sequence; ordinary new games keep it. */
    if (!strcmp(title_options.level, "H001") && title_guest_is_new_game_movie_caller(caller) &&
        title_guest_movie_is_initial_cinematic(movie))
    {
        return 1;
    }
    return 0;
}

int title_debug_boot(void)
{
    static int consumed;
    setup_debug();
    if (title_options.nofmv)
    {
        title_state_skip_boot_logo(); /* skip first-boot logo state */
    }
    if (consumed || (!title_options.level[0] && !title_options.save_slot))
    {
        return 0;
    }
    consumed = 1;
    GuestRegisters saved = save_guest_registers();
    uint32_t scene = 0;
    if (title_options.save_slot)
    {
        title_guest_prepare_save_slots();
        title_state_select_save_slot(title_options.save_slot);
        uint32_t result = title_guest_restore_selected_save();
        if (result != 1)
        {
            fprintf(stderr, "[DEBUG] Save slot %d could not be restored; returning to menu.\n",
                    title_options.save_slot);
            restore_guest_registers(&saved);
            return 0;
        }
        scene = title_guest_restored_scene_id();
    }
    else
    {
        title_guest_start_new_game();
    }
    if (title_options.level[0])
    {
        scene = title_level_id(title_options.level);
    }
    title_guest_set_mode(11);
    title_guest_set_gameplay_state(0);
    fprintf(stderr, "[DEBUG] Boot scene=%08X save=%d\n", scene, title_options.save_slot);
    restore_guest_registers(&saved);
    title_guest_set_result(scene);
    return 1;
}

void title_debug_frame(void)
{
    title_display_frame();
    if (!title_options.debug)
    {
        return;
    }
    setup_debug();
    /* Capture before pausing changes the audio workload. */
    {
        static int last_f8;
        int f8 = xbox_FramebufferFocused() && xbox_FramebufferKeyDown(VK_F8);
        if (f8 && !last_f8)
        {
            extern void apu_debug_audio_request(void);
            apu_debug_audio_request();
            fprintf(stderr, "[AUDIO-CAPTURE] F8 requested; scene mode=%u\n", title_state_menu_mode());
        }
        last_f8 = f8;
    }
    InterlockedExchange(&s_pause, title_state_menu_mode() == 7);
    if (title_state_menu_mode() == 11 && s_pending_capture)
    {
        LONG capture = InterlockedExchange(&s_pending_capture, 0);
        if (capture == 1)
        {
            nv2a_debug_profile_request();
            s_profile_until = GetTickCount() + 10000;
        }
        else
        {
            nv2a_debug_trace_seconds_request(10);
            s_trace_until = GetTickCount() + 10000;
        }
    }
    InterlockedExchange(&s_power_bits, title_state_player_powers());
    static uint32_t last_health;
    uint32_t health = title_state_player_health();
    if (title_debug_monster_safe && health < last_health)
    {
        fprintf(stderr, "[DEBUG-DAMAGE] Health fell %u -> %u mode=%u\n", last_health, health,
                title_state_menu_mode());
        fflush(stderr);
    }
    last_health = health;
    uint32_t current_scene = title_state_current_scene();
    if (current_scene)
    {
        InterlockedExchange(&s_level, title_state_scene_id(current_scene));
    }
    char location[160], id[5];
    uint32_t scene = (uint32_t)s_level;
    for (int i = 0; i < 4; i++)
    {
        id[i] = (char)(scene >> (24 - i * 8));
    }
    id[4] = 0;
    TitlePlayerPosition position = {0};
    if (current_scene)
    {
        position = title_state_player_position();
    }
    if (current_scene && position.available)
    {
        snprintf(location, sizeof location, "Level: %.4s | X: %.2f  Y: %.2f  Z: %.2f", id, position.x,
                 position.y, position.z);
    }
    else
    {
        snprintf(location, sizeof location, "Level: %.4s | Scooby: unavailable", current_scene ? id : "----");
    }
    xbox_DebugOverlayLocation(location);
    LONG action = InterlockedCompareExchange(&s_command, DEBUG_ACTION_NONE, DEBUG_ACTION_NONE);
    if (action <= DEBUG_ACTION_NONE ||
        InterlockedCompareExchange(&s_command, DEBUG_QUEUE_WRITING, action) != action)
    {
        return;
    }
    LONG argument = s_argument;
    InterlockedExchange(&s_command, DEBUG_ACTION_NONE);
    GuestRegisters saved = save_guest_registers();
    if (action == DEBUG_ACTION_WARP_TO_SCENE && title_state_menu_mode() == 7)
    {
        title_guest_warp_to_scene((uint32_t)argument);
        fprintf(stderr, "[DEBUG] Warped to %08X\n", (unsigned)argument);
    }
    else if (action == DEBUG_ACTION_TOGGLE_POWER && title_state_menu_mode() == 7 && argument >= 0 &&
             argument < 15)
    {
        title_state_set_player_powers(title_state_player_powers() ^ (1u << argument));
    }
    else if (action == DEBUG_ACTION_TRACE_FRAME)
    {
        nv2a_debug_trace_request();
    }
    else if (action == DEBUG_ACTION_PROFILE_RENDERER)
    {
        InterlockedExchange(&s_pending_capture, 1);
        close_debug();
    }
    else if (action == DEBUG_ACTION_TOGGLE_FPS_GRAPH)
    {
        title_options.fps_graph = !title_options.fps_graph;
        xbox_DebugOverlayGraph(title_options.fps_graph);
    }
    else if (action == DEBUG_ACTION_TRACE_TEN_SECONDS)
    {
        InterlockedExchange(&s_pending_capture, 2);
        close_debug();
    }
    else if (action == DEBUG_ACTION_TOGGLE_TRIGGERS || action == DEBUG_ACTION_TOGGLE_COLLISION)
    {
        title_debug_geometry_toggle(action == DEBUG_ACTION_TOGGLE_COLLISION);
    }
    else if (action == DEBUG_ACTION_TOGGLE_WARP_REQUIREMENT)
    {
        InterlockedExchange(&title_debug_ignore_warp, !title_debug_ignore_warp);
    }
    else if (action == DEBUG_ACTION_TOGGLE_MONSTER_PROTECTION)
    {
        InterlockedExchange(&title_debug_monster_safe, !title_debug_monster_safe);
        fprintf(stderr, "[DEBUG-DAMAGE] Monster protection %s\n", title_debug_monster_safe ? "ON" : "OFF");
        fflush(stderr);
    }
    else if (action == DEBUG_ACTION_TOGGLE_FLIGHT)
    {
        InterlockedExchange(&title_debug_noclip, !title_debug_noclip);
        title_debug_noclip_reset();
    }
    else if (action == DEBUG_ACTION_TOGGLE_SNACK_REQUIREMENT)
    {
        InterlockedExchange(&title_debug_ignore_snacks, !title_debug_ignore_snacks);
    }
    else if (action == DEBUG_ACTION_GRANT_SNACKS && current_scene)
    {
        title_guest_grant_snacks(100);
    }
    restore_guest_registers(&saved);
}

int title_debug_warp_unlocked(void)
{
    return title_options.debug && title_debug_ignore_warp;
}

int title_debug_snacks_unlocked(void)
{
    return title_options.debug && title_debug_ignore_snacks;
}

extern int xbox_FramebufferKeyDown(int vk);

void title_debug_damage_trace(uint32_t source, uint32_t caller, uint32_t amount, int blocked)
{
    static unsigned reports;
    if (!title_options.debug || (!title_debug_monster_safe && reports >= 8) || reports >= 128)
    {
        return;
    }
    reports++;
    fprintf(
        stderr,
        "[DEBUG-DAMAGE] protection=%ld noclip=%ld caller=%08X source=%08X amount=%u health=%u blocked=%d\n",
        title_debug_monster_safe, title_debug_noclip, caller, source, amount, title_state_player_health(),
        blocked);
    fflush(stderr);
}

int title_debug_monster_damage(uint32_t source, uint32_t caller)
{
    if (!title_options.debug)
    {
        return 0;
    }
    if (title_debug_noclip)
    {
        return 1;
    }
    return title_debug_monster_safe && title_guest_damage_is_from_monster(source, caller);
}

void title_debug_noclip_reset(void)
{
    title_guest_clear_player_motion();
}

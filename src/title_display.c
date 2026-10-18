#include "api/title_guest_state.h"
/* Display settings available in ordinary Options, independently of debugging. */
#include "title_display.h"
#include "api/title_guest_menu.h"
#include "api/title_guest_ui.h"
#include <windows.h>
#include <xbox/host_gamepad.h>
#include <xbox/display_options.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <xbox/debug_overlay.h>

/* Overlay state is separate from the original menu entries and event links. */
static int s_open;
static uint32_t s_native_row;
static uint32_t s_recomp_entry;
static uint32_t s_recomp_links;
static uint32_t s_quit_entry, s_quit_links, s_credits_links;
static volatile LONG s_recomp, s_recomp_back;
static int s_return_focus;

/* Use the native Back action in menus that expose it. The guest adapter
 * owns the recovered layout and changes only the current press edges. */
static void remap_start_to_back_if_needed(uint32_t menu_mode)
{
    if (!menu_accepts_back(menu_mode))
    {
        return;
    }

    remap_controller_press_edges(BUTTON_START, BUTTON_BACK);
}

/* Compare native menu observations with the host/USB trace. This does not
 * consume or rewrite input; asynchronous XAPI state may change between reads. */
static void trace_native_menu_input(const char *boundary)
{
    if (!getenv("DOGGAME_INPUT_TRACE"))
    {
        return;
    }
    TitleMenuInputSnapshot snapshot;
    int valid = title_menu_input_snapshot(&snapshot);
    fprintf(stderr, "[INPUT TRACE %llu] native boundary=%s mode=%u valid=%d held=%08X pressed=%08X capture=%d\n",
            (unsigned long long)GetTickCount64(), boundary, snapshot.mode, valid,
            snapshot.held, snapshot.pressed, xbox_DebugOverlayCaptured());
    if (snapshot.pressed)
    {
        uint32_t main_bits, save_bits, minimum_bits;
        memcpy(&main_bits, &snapshot.main_elapsed, sizeof(main_bits));
        memcpy(&save_bits, &snapshot.save_elapsed, sizeof(save_bits));
        memcpy(&minimum_bits, &snapshot.ui_minimum_elapsed, sizeof(minimum_bits));
        fprintf(stderr, "[INPUT TRACE %llu] native-edge-timing-v2 pressed=%08X main_bits=%08X save_bits=%08X minimum_bits=%08X\n",
                (unsigned long long)GetTickCount64(), snapshot.pressed, main_bits, save_bits, minimum_bits);
    }
}

void title_display_input_frame(void)
{
    trace_native_menu_input("after-poll");
    remap_start_to_back_if_needed(title_menu_current_mode());
}

/* The window thread sees a copied hit map. Guest reads and events run on
 * the title thread; queued clicks expire when the menu changes. */
static SRWLOCK s_hit_lock = SRWLOCK_INIT;
static MenuHit s_hits[256];
static unsigned s_hit_count;
static uint32_t s_hit_scene, s_hit_mode;
static MenuHit s_clicked;
static uint32_t s_click_scene, s_click_mode;
static int s_click_pending;

static int native_point(int x, int y, int activate)
{
    if (xbox_DebugOverlayCaptured())
    {
        return 0;
    }
    AcquireSRWLockExclusive(&s_hit_lock);
    int best = -1;
    float distance = 1e30f;
    for (unsigned i = 0; i < s_hit_count; i++)
    {
        MenuHit *h = &s_hits[i];
        if (x < h->x || x >= h->x + h->w || y < h->y || y >= h->y + h->h)
        {
            continue;
        }
        float dy = (y - h->y - h->h / 2) / h->h, dx = (x - h->x - h->w / 2) / h->w;
        float d = dy * dy + dx * dx;
        if (d < distance)
        {
            distance = d;
            best = (int)i;
        }
    }
    if (best >= 0 && (activate || s_click_pending != 2))
    {
        s_clicked = s_hits[best];
        s_click_scene = s_hit_scene;
        s_click_mode = s_hit_mode;
        s_click_pending = activate ? 2 : 1;
    }
    ReleaseSRWLockExclusive(&s_hit_lock);
    return best >= 0;
}

static int native_click(int x, int y)
{
    return native_point(x, y, 1);
}

static void native_hover(int x, int y)
{
    native_point(x, y, 0);
}

static void native_menu_frame(uint32_t scene, uint32_t mode)
{
    MenuHit hits[256], click;
    unsigned count = title_guest_ui_collect_hits(scene, mode, hits, s_quit_entry, s_native_row);
    uint32_t click_scene, click_mode;
    int pending;
    AcquireSRWLockExclusive(&s_hit_lock);
    pending = s_click_pending;
    click = s_clicked;
    click_scene = s_click_scene;
    click_mode = s_click_mode;
    s_click_pending = 0;
    ReleaseSRWLockExclusive(&s_hit_lock);
    if (pending && scene == click_scene && mode == click_mode && !s_open && !xbox_DebugOverlayCaptured())
    {
        unsigned i;
        for (i = 0; i < count; i++)
        {
            if (hits[i].item == click.item && hits[i].id == click.id)
            {
                break;
            }
        }
        if (i < count)
        {
            /* Back labels are global input listeners. Focusing one can close
             * the map through its focus links; activate Back without refocus. */
            if (click.event != 66 && !(title_guest_ui_item_state(click.item) & 2))
            {
                if (click.image && title_menu_map_page_visible(scene, mode))
                {
                    unsigned n = title_guest_ui_scene_image_count(scene);
                    uint32_t items = title_guest_ui_scene_images(scene);
                    for (unsigned j = 0; items && n <= 4096 && j < n; j++)
                    {
                        uint32_t item = items + j * 0xF8;
                        if (item != click.item && (title_guest_ui_item_state(item) & 2) &&
                            title_guest_ui_has_navigation(item))
                        {
                            title_menu_event(item, 95);
                        }
                    }
                }
                for (unsigned j = 0; j < count; j++)
                {
                    if (hits[j].image == click.image && hits[j].item != click.item &&
                        (title_guest_ui_item_state(hits[j].item) & 2))
                    {
                        title_menu_event(hits[j].item, hits[j].image ? 95 : 80);
                    }
                }
                title_menu_event(click.item, click.image ? 94 : 79);
            }
            if (pending == 2 && click.event && title_state_current_scene() == scene &&
                title_state_menu_mode() == mode)
            {
                title_menu_event(click.item, click.event);
            }
            scene = title_state_current_scene();
            mode = title_state_menu_mode();
            count = title_guest_ui_collect_hits(scene, mode, hits, s_quit_entry, s_native_row);
        }
    }
    AcquireSRWLockExclusive(&s_hit_lock);
    memcpy(s_hits, hits, count * sizeof *hits);
    s_hit_count = s_open ? 0 : count;
    s_hit_scene = scene;
    s_hit_mode = mode;
    ReleaseSRWLockExclusive(&s_hit_lock);
}

static volatile LONG s_options;
static unsigned s_selection;
/* Keep short taps across slow frames; this cursor never consumes guest input. */
static HostGamepadMenuReader s_menu_input;
static DWORD s_release_at;
static uint32_t s_hidden_scene;
static uint32_t s_cursor_item;
static uint32_t s_cursor_scene;
static unsigned s_cursor_visible;
static float s_cursor_y;
static unsigned s_hidden_rows;
static const uint32_t s_option_ids[] = {0x6855A343u, 0x8F2D6816u, 0x41F6FF46u, 0xF7B264FCu};
extern uint32_t xbox_HeapAlloc(uint32_t size, uint32_t alignment);

static void close_display(void)
{
    s_open = 0;
    s_release_at = GetTickCount() + 250;
}

static int display_key(unsigned key)
{
    if (!s_options)
    {
        return 0;
    }
    if (s_recomp && s_open == 2)
    {
        if (key == VK_ESCAPE || key == 'L' || key == 'X')
        {
            InterlockedExchange(&s_recomp_back, 1);
        }
        else if (key == VK_UP || key == 'W' || key == VK_LEFT || key == 'A')
        {
            s_selection = (s_selection + 4) % 5;
        }
        else if (key == VK_DOWN || key == 'S' || key == VK_RIGHT || key == 'D')
        {
            s_selection = (s_selection + 1) % 5;
        }
        else if (key == VK_RETURN || key == VK_SPACE || key == 'Z')
        {
            if (s_selection == 0)
            {
                s_selection = 0;
                s_open = 1;
            }
            else if (s_selection == 1)
            {
                xbox_DisplayMouseInteractionSet(!xbox_DisplayMouseInteraction());
            }
            else if (s_selection == 2)
            {
                xbox_DisplayDynamicPromptsSet(!xbox_DisplayDynamicPrompts());
            }
            else if (s_selection == 3)
            {
                xbox_DisplayFpsCapSet(!xbox_DisplayFpsCap());
            }
            else
            {
                InterlockedExchange(&s_recomp_back, 1);
            }
        }
        return 1;
    }
    if (!s_open)
    {
        if (key != 'R')
        {
            return 0;
        }
        s_selection = 0;
        s_open = 1;
        fprintf(stderr, "[DISPLAY] Resolution selector opened.\n");
        xbox_MenuOverlayCapture(1);
        return 1;
    }
    if (key == VK_ESCAPE || key == 'X' || key == 'L')
    {
        if (s_recomp)
        {
            s_open = 2;
            s_selection = 0;
        }
        else
        {
            close_display();
        }
    }
    else if (key == VK_UP || key == 'W')
    {
        s_selection = (s_selection + 4) % 5;
    }
    else if (key == VK_DOWN || key == 'S')
    {
        s_selection = (s_selection + 1) % 5;
    }
    else if (key == VK_LEFT || key == 'A' || key == VK_RIGHT || key == 'D' || key == VK_RETURN ||
             key == 'Z' || key == VK_SPACE)
    {
        int direction = (key == VK_LEFT || key == 'A') ? -1 : 1;
        if (s_selection == 0)
        {
            unsigned count = xbox_DisplayResolutionCount();
            xbox_DisplayResolutionSelect((xbox_DisplayResolutionSelected() + count + direction) % count);
        }
        else if (s_selection == 1)
        {
            xbox_DisplayWidescreenSet(!xbox_DisplayWidescreen());
        }
        else if (s_selection == 2)
        {
            xbox_DisplayInternalScalingSet(!xbox_DisplayInternalScaling());
        }
        else if (s_selection == 3)
        {
            xbox_DisplayBorderlessSet(!xbox_DisplayBorderless());
        }
        else
        {
            display_key(VK_ESCAPE);
        }
    }
    return 1;
}

static int display_click(int x, int y)
{
    if (!s_options)
    {
        return native_click(x, y);
    }
    if (!s_open)
    {
        if (!s_native_row && x >= 310 && x < 628 && y >= 428 && y < 462)
        {
            return display_key('R');
        }
        return native_click(x, y);
    }
    if (s_open == 2)
    {
        if (x >= 150 && x < 560 && y >= 110 && y < 310)
        {
            s_selection = (y - 110) / 40;
            display_key(VK_RETURN);
        }
        return 1;
    }
    if (x >= 150 && x < 570 && y >= 110 && y < 310)
    {
        s_selection = (y - 110) / 40;
        if (s_selection == 4)
        {
            display_key(VK_ESCAPE);
        }
        else if (x >= 345 && x < 375)
        {
            display_key(VK_LEFT);
        }
        else if (x >= 520 && x < 550)
        {
            display_key(VK_RIGHT);
        }
    }
    return 1;
}

static void display_hover(int x, int y)
{
    if (s_open)
    {
        if (s_open == 2)
        {
            if (x >= 150 && x < 560 && y >= 110 && y < 310)
            {
                s_selection = (y - 110) / 40;
            }
            return;
        }
        if (x >= 150 && x < 570 && y >= 110 && y < 310)
        {
            s_selection = (y - 110) / 40;
        }
        return;
    }
    native_hover(x, y);
}

static void label(HDC dc, int x, int y, const char *value)
{
    TextOutA(dc, x, y, value, (int)strlen(value));
}

/* Window-thread policy updates run independently of frame composition. */
static void display_input(void)
{
    for (unsigned pending = 0; pending < 128; pending++)
    {
        XINPUT_STATE pad;
        WORD buttons = 0;
        WORD edge = 0;
        if (host_gamepad_read_menu(0, &s_menu_input, &pad, &edge) == ERROR_SUCCESS)
        {
            buttons = pad.Gamepad.wButtons;
        }
        if (!s_options && s_open)
        {
            close_display();
        }
        if (s_options && !s_open && (edge & XINPUT_GAMEPAD_Y))
        {
            display_key('R');
        }
        else if (s_open)
        {
            if (edge & XINPUT_GAMEPAD_DPAD_UP)
            {
                display_key(VK_UP);
            }
            if (edge & XINPUT_GAMEPAD_DPAD_DOWN)
            {
                display_key(VK_DOWN);
            }
            if (edge & XINPUT_GAMEPAD_DPAD_LEFT)
            {
                display_key(VK_LEFT);
            }
            if (edge & XINPUT_GAMEPAD_DPAD_RIGHT)
            {
                display_key(VK_RIGHT);
            }
            if (edge & XINPUT_GAMEPAD_A)
            {
                display_key(VK_RETURN);
            }
            if (edge & XINPUT_GAMEPAD_B)
            {
                display_key(VK_ESCAPE);
            }
        }
        if (!s_open && (LONG)(GetTickCount() - s_release_at) >= 0 &&
            !(buttons & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_Y)) &&
            !(GetAsyncKeyState(VK_RETURN) & 0x8000) && !(GetAsyncKeyState('Z') & 0x8000) &&
            !(GetAsyncKeyState('X') & 0x8000) && !(GetAsyncKeyState('L') & 0x8000) &&
            !(GetAsyncKeyState(VK_SPACE) & 0x8000) && !(GetAsyncKeyState(VK_ESCAPE) & 0x8000))
        {
            xbox_MenuOverlayCapture(0);
        }
        if (edge && getenv("DOGGAME_INPUT_TRACE"))
        {
            fprintf(stderr, "[INPUT TRACE %llu] settings edge=%04X available=%ld open=%d\n",
                    (unsigned long long)GetTickCount64(), edge, (long)s_options, (int)s_open);
        }
        if (!edge)
        {
            break;
        }
    }
}

static void display_draw(void *device, unsigned width, unsigned height)
{
    (void)width;
    (void)height;
    HDC dc = (HDC)device;
    if (!s_options)
    {
        return;
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 230, 70));
    unsigned w, h;
    xbox_DisplayResolutionSize(xbox_DisplayResolutionSelected(), &w, &h);
    char value[96];
    if (!s_open)
    {
        if (s_native_row)
        {
            return;
        }
        RECT row = {310, 428, 628, 462};
        FillRect(dc, &row, (HBRUSH)GetStockObject(BLACK_BRUSH));
        snprintf(value, sizeof value, "Resolution: %ux%u  [R / Y]", w, h);
        label(dc, 318, 436, value);
        return;
    }
    HFONT font = CreateFontA(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FIXED_PITCH, "Terminal");
    HGDIOBJ old_font = font ? SelectObject(dc, font) : NULL;
    if (s_open == 2)
    {
        SetTextColor(dc, RGB(0, 190, 0));
        label(dc, 150, 65, "RECOMP SETTINGS");
        const char *rows[] = {
            "Resolution", xbox_DisplayMouseInteraction() ? "Mouse interaction: ON" : "Mouse interaction: OFF",
            xbox_DisplayDynamicPrompts() ? "Dynamic button prompts: ON" : "Dynamic button prompts: OFF",
            xbox_DisplayFpsCap() ? "30 FPS cap: ON" : "30 FPS cap: OFF", "Back  [Esc / B]"};
        for (unsigned i = 0; i < 5; i++)
        {
            SetTextColor(dc, i == s_selection ? RGB(40, 255, 0) : RGB(0, 160, 0));
            label(dc, 150, 110 + i * 40, rows[i]);
        }
        if (font)
        {
            SelectObject(dc, old_font);
            DeleteObject(font);
        }
        return;
    }
    SetTextColor(dc, RGB(0, 190, 0));
    label(dc, 150, 65, "RESOLUTION");
    const char *names[] = {"Resolution:", "Aspect ratio:", "Scaling mode:", "Display mode:"};
    for (unsigned i = 0; i < 4; i++)
    {
        SetTextColor(dc, i == s_selection ? RGB(40, 255, 0) : RGB(0, 160, 0));
        label(dc, 150, 110 + i * 40, names[i]);
        label(dc, 350, 110 + i * 40, "<");
        label(dc, 525, 110 + i * 40, ">");
        if (i == 0)
        {
            snprintf(value, sizeof value, "%ux%u", w, h);
        }
        else if (i == 1)
        {
            snprintf(value, sizeof value, "%s", xbox_DisplayWidescreen() ? "16:9" : "4:3");
        }
        else if (i == 2)
        {
            snprintf(value, sizeof value, "%s", xbox_DisplayInternalScaling() ? "Internal" : "Window");
        }
        else
        {
            snprintf(value, sizeof value, "%s", xbox_DisplayBorderless() ? "Borderless" : "Windowed");
        }
        label(dc, 385, 110 + i * 40, value);
    }
    SetTextColor(dc, s_selection == 4 ? RGB(40, 255, 0) : RGB(0, 160, 0));
    label(dc, 150, 270, "Back  [Esc / B]");
    if (font)
    {
        SelectObject(dc, old_font);
        DeleteObject(font);
    }
}

void title_display_init(void)
{
    xbox_MenuOverlayUpdateRegister(display_input);
    xbox_MenuOverlayRegister(display_draw, display_key, display_click);
    xbox_MenuOverlayHoverRegister(display_hover);
}

int title_display_native_event(uint32_t target, uint32_t event)
{
    if (target && target == s_quit_entry && event == 63 && title_state_menu_mode() == 2 &&
        (title_guest_ui_item_flags(target) & 1) && (title_guest_ui_item_state(target) & 8))
    {
        xbox_DisplayRequestQuit();
        return 1;
    }
    if (target && target == s_recomp_entry && event == 63 && title_state_menu_mode() == 2)
    {
        /* The entry's activation links now run the native Options transition.
         * Returning zero lets the dispatcher process those replaced links. */
        InterlockedExchange(&s_recomp, 1);
    }
    return 0;
}

static void install_quit(uint32_t scene, uint32_t mode)
{
    s_quit_entry = mode == 2 ? title_guest_ui_find_font(scene, 0xF7B264FCu) : 0;
    uint32_t credits = mode == 2 ? title_guest_ui_find_font(scene, 0x2CA9AD4Bu) : 0;
    if (!s_quit_entry || !credits)
    {
        return;
    }
    uint32_t asset = title_guest_ui_font_asset(s_quit_entry), source = title_guest_ui_font_asset(credits);
    if (!asset || !source)
    {
        return;
    }
    if (!s_quit_links || title_guest_ui_links(s_quit_entry) != s_quit_links)
    {
        uint32_t text_id = title_guest_ui_asset_text_id(asset), text = title_display_find_asset(text_id);
        unsigned count = title_guest_ui_link_count(credits);
        if (!text || title_guest_ui_text_length(text) < 9 || count > 253)
        {
            return;
        }
        if (!s_quit_links)
        {
            s_quit_links = xbox_HeapAlloc(3 * 32, 16);
        }
        if (!s_credits_links)
        {
            s_credits_links = xbox_HeapAlloc(255 * 32, 16);
        }
        if (!s_quit_links || !s_credits_links)
        {
            return;
        }
        /* Reuse the unused Sound Mode entity with the main-menu font style. */
        title_guest_ui_copy_bytes(asset + 0x18, source + 0x18, 0xA0 - 0x18);
        title_guest_ui_asset_text_id_set(asset, text_id);
        title_guest_ui_asset_y_set(asset, title_guest_ui_asset_y(source) + 40);
        title_guest_ui_font_style_set(s_quit_entry, title_guest_ui_font_style(credits));
        title_guest_ui_set_text(text, "Quit Game", 9);
        title_guest_ui_write_link(s_quit_links, 73, 80, 0xF7B264FCu);
        title_guest_ui_write_link(s_quit_links + 32, 73, 79, 0x2CA9AD4Bu);
        title_guest_ui_write_link(s_quit_links + 64, 79, 24, 0x8045CBACu);
        title_guest_ui_copy_bytes(s_credits_links, title_guest_ui_links(credits), count * 32);
        title_guest_ui_write_link(s_credits_links + count * 32, 74, 80, 0x2CA9AD4Bu);
        title_guest_ui_write_link(s_credits_links + (count + 1) * 32, 74, 79, 0xF7B264FCu);
        title_guest_ui_links_set(s_quit_entry, s_quit_links);
        title_guest_ui_link_count_set(s_quit_entry, 3);
        title_guest_ui_links_set(credits, s_credits_links);
        title_guest_ui_link_count_set(credits, count + 2);
    }
    int visible = title_state_main_menu_started() && !s_recomp && s_recomp_entry &&
                  (title_guest_ui_item_flags(s_recomp_entry) & 1) && (title_guest_ui_item_flags(credits) & 1);
    if (visible)
    {
        title_guest_ui_item_flags_set(s_quit_entry, title_guest_ui_item_flags(s_quit_entry) | (1));
        title_guest_ui_item_state_set(s_quit_entry, title_guest_ui_item_state(s_quit_entry) | (8));
    }
    else
    {
        title_guest_ui_item_flags_set(s_quit_entry, title_guest_ui_item_flags(s_quit_entry) & (0xFE));
        title_guest_ui_item_state_set(s_quit_entry, title_guest_ui_item_state(s_quit_entry) & (~10u));
    }
}

static int main_options_visible(uint32_t scene)
{
    if (!scene)
    {
        return 0;
    }
    /* MNU3 keeps mode2 while asset events show/hide its Options buttons.
     * The native UI-font array is scene+0x1AC, count+0x100, stride0x100.
     * These are the original asset IDs; xEnt's visible flag is byte+0x14. */
    uint32_t count = title_guest_ui_scene_font_count(scene), items = title_guest_ui_scene_fonts(scene);
    if (!items || count > 4096)
    {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t item = items + i * 0x100, id = title_guest_ui_object_id(item);
        if ((id == 0x6855A343u || id == 0x8F2D6816u || id == 0x41F6FF46u) &&
            (title_guest_ui_item_flags(item) & 1))
        {
            return 1;
        }
    }
    return 0;
}

void title_display_frame(void)
{
    trace_native_menu_input("frame-hook");
    /* Main-menu Options uses UI visibility, not the generic Options mode. */
    uint32_t mode = title_state_menu_mode();
    remap_start_to_back_if_needed(mode);
    /* This frame hook runs in both the gameplay and main-menu loops. */
    xbox_DisplayGameplayCursor(mode == 11);
    uint32_t scene = title_state_current_scene();
    s_recomp_entry = mode == 2 ? title_guest_ui_find_font(scene, 0xB488CA08u) : 0;
    if (s_recomp_entry)
    {
        uint32_t asset = title_guest_ui_font_asset(s_recomp_entry);
        uint32_t text = asset ? title_display_find_asset(title_guest_ui_asset_text_id(asset)) : 0;
        if (text && title_guest_ui_text_length(text) >= 15)
        {
            title_guest_ui_set_text(text, "Recomp Settings", 15);
        }
        uint32_t options = title_guest_ui_find_font(scene, 0xB5FB267Bu);
        if (options && (!s_recomp_links || title_guest_ui_links(s_recomp_entry) != s_recomp_links))
        {
            unsigned count = title_guest_ui_link_count(s_recomp_entry),
                     other = title_guest_ui_link_count(options), n = 0;
            uint32_t links = title_guest_ui_links(s_recomp_entry),
                     option_links = title_guest_ui_links(options);
            if (count + other <= 255)
            {
                if (!s_recomp_links)
                {
                    s_recomp_links = xbox_HeapAlloc(255 * 32, 16);
                }
                if (s_recomp_links)
                {
                    for (unsigned i = 0; i < count; i++)
                    {
                        if (title_guest_ui_link_source_event(links, i) != 63)
                        {
                            title_guest_ui_copy_bytes(s_recomp_links + 32 * n++, links + 32 * i, 32);
                        }
                    }
                    for (unsigned i = 0; i < other; i++)
                    {
                        if (title_guest_ui_link_source_event(option_links, i) == 63)
                        {
                            title_guest_ui_copy_bytes(s_recomp_links + 32 * n++, option_links + 32 * i, 32);
                        }
                    }
                    title_guest_ui_links_set(s_recomp_entry, s_recomp_links);
                    title_guest_ui_link_count_set(s_recomp_entry, n);
                }
            }
        }
    }
    /* Restore before eligibility checks, then temporarily hide the native
     * choices while drawing their replacement over the original CRT scene. */
    if (s_cursor_item && (!s_recomp || s_cursor_scene != scene))
    {
        if (s_cursor_scene == scene)
        {
            title_guest_ui_item_flags_set(s_cursor_item, (title_guest_ui_item_flags(s_cursor_item) & 0xFE) |
                                                             s_cursor_visible);
            uint32_t asset = title_guest_ui_image_asset(s_cursor_item);
            if (asset)
            {
                title_guest_ui_asset_y_set(asset, s_cursor_y);
            }
        }
        s_cursor_item = 0;
    }
    if (s_hidden_scene == scene)
    {
        for (unsigned i = 0; i < 4; i++)
        {
            uint32_t item = title_guest_ui_find_font(scene, s_option_ids[i]);
            if (item && (s_hidden_rows & (1u << i)))
            {
                title_guest_ui_item_flags_set(item, title_guest_ui_item_flags(item) | (1));
            }
        }
    }
    s_hidden_scene = 0;
    s_hidden_rows = 0;
    int main_visible = mode == 2 && main_options_visible(scene);
    /* Native Back finishes by restoring focus to Options later in its event
     * sequence. Transfer focus after that transition, on the following frame. */
    if (s_return_focus && mode == 2 && !main_visible && s_recomp_entry &&
        (title_guest_ui_item_flags(s_recomp_entry) & 1))
    {
        unsigned n = title_guest_ui_scene_font_count(scene);
        uint32_t items = title_guest_ui_scene_fonts(scene);
        for (unsigned i = 0; items && n <= 4096 && i < n; i++)
        {
            uint32_t item = items + i * 0x100;
            if (item != s_recomp_entry && (title_guest_ui_item_state(item) & 2))
            {
                title_menu_event(item, 80);
            }
        }
        title_menu_event(s_recomp_entry, 79);
        s_return_focus = 0;
    }
    /* Display settings now belong to Recomp Settings, not native Options. */
    s_native_row = 0;
    if (s_recomp && main_visible && !s_open)
    {
        s_open = 2;
        s_selection = 0;
        xbox_MenuOverlayCapture(1);
    }
    if (InterlockedExchange(&s_recomp_back, 0))
    {
        uint32_t back = title_guest_ui_find_font(scene, 0x6855A343u);
        close_display();
        InterlockedExchange(&s_recomp, 0);
        if (s_cursor_item && s_cursor_scene == scene)
        {
            title_guest_ui_item_flags_set(s_cursor_item, title_guest_ui_item_flags(s_cursor_item) & (0xFE));
            uint32_t asset = title_guest_ui_image_asset(s_cursor_item);
            if (asset)
            {
                title_guest_ui_asset_y_set(asset, s_cursor_y);
            }
            s_cursor_item = 0;
        }
        if (back)
        {
            title_menu_event(back, 66);
        }
        s_return_focus = 1;
        main_visible = main_options_visible(scene);
    }
    if (mode != 2)
    {
        InterlockedExchange(&s_recomp, 0);
        s_return_focus = 0;
    }
    if (mode == 2 && s_recomp)
    {
        /* Keep the native cursor's animation; only move its image asset.
         * Guest memory is updated here, never by the window thread. */
        uint32_t items = title_guest_ui_scene_images(scene), n = title_guest_ui_scene_image_count(scene);
        if (items && n <= 4096)
        {
            for (uint32_t i = 0; i < n; i++)
            {
                uint32_t item = items + i * 0xF8;
                /* Sound Mode's spare cursor must stay hidden. Move the active
             * Music Volume cursor instead, leaving its blink timer in control. */
                if (title_guest_ui_object_id(item) == 0xA13417C3u)
                {
                    title_guest_ui_item_flags_set(item, title_guest_ui_item_flags(item) & (0xFE));
                }
                if (title_guest_ui_object_id(item) == 0x417297B0u && title_guest_ui_image_asset(item))
                {
                    if (s_cursor_item != item)
                    {
                        s_cursor_item = item;
                        s_cursor_scene = scene;
                        s_cursor_visible = title_guest_ui_item_flags(item) & 1;
                        s_cursor_y = title_guest_ui_asset_y(title_guest_ui_image_asset(item));
                    }
                    title_guest_ui_asset_y_set(
                        title_guest_ui_image_asset(item),
                        2.0f + (s_open == 2 ? 110.0f + 40.0f * s_selection : 110.0f + 40.0f * s_selection));
                }
            }
        }
    }
    InterlockedExchange(&s_options, mode == 5 || (main_visible && s_recomp) ||
                                        (mode == 7 && title_state_map_options_open()));
    if (mode == 2 && main_visible && s_open)
    {
        s_hidden_scene = scene;
        for (unsigned i = 0; i < 4; i++)
        {
            uint32_t item = title_guest_ui_find_font(scene, s_option_ids[i]);
            if (item && (title_guest_ui_item_flags(item) & 1))
            {
                s_hidden_rows |= 1u << i;
                title_guest_ui_item_flags_set(item, title_guest_ui_item_flags(item) & (0xFE));
            }
        }
    }
    install_quit(scene, mode);
    native_menu_frame(scene, mode);
}

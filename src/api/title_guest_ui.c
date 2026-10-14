/* Native menu layout adapter: discover interactive targets, follow the
 * active navigation graph, and preserve save-dialogue row boundaries.
 * The renderer consumes the copied hit map rather than traversing guest UI. */
#define RECOMP_GENERATED_CODE
#include "gen/recomp_types.h"
#include "title_guest_ui.h"
#include "title_guest_menu.h"
#include <math.h>
/* Collection is serialized on the title thread. These special entries were
 * injected by the display host and have a native Select action. */
static uint32_t hit_quit_entry, hit_native_row;

static unsigned item_event(uint32_t item)
{
    if (item == hit_quit_entry && item)
    {
        return 63;
    }
    if (item == hit_native_row)
    {
        return 63;
    }
    uint32_t links = MEM32(item + 8);
    unsigned count = MEM8(item + 5), back = 0;
    if (!links)
    {
        return 0;
    }
    for (unsigned i = 0; i < count; i++)
    {
        unsigned event = MEM16(links + i * 32);
        if (event == 63)
        {
            return 63;
        }
        if (event == 66)
        {
            back = 66;
        }
    }
    return back;
}

int title_guest_ui_has_navigation(uint32_t item)
{
    uint32_t links = MEM32(item + 8);
    unsigned count = MEM8(item + 5);
    for (unsigned i = 0; links && i < count; i++)
    {
        unsigned event = MEM16(links + i * 32);
        if (event >= 73 && event <= 76)
        {
            return 1;
        }
    }
    return 0;
}

static uint32_t image_item(uint32_t scene, uint32_t id)
{
    uint32_t n = MEM32(scene + 0xFC), items = MEM32(scene + 0x1A8);
    if (!items || n > 4096)
    {
        return 0;
    }
    for (unsigned i = 0; i < n; i++)
    {
        if (MEM32(items + i * 0xF8) == id)
        {
            return items + i * 0xF8;
        }
    }
    return 0;
}

static int map_page_visible(uint32_t scene, uint32_t mode)
{
    return title_menu_map_page_visible(scene, mode);
}

static int map_marker_visible(uint32_t scene, uint32_t item)
{
    uint32_t asset = MEM32(item + 0xEC);
    if (!asset)
    {
        return 0;
    }
    unsigned n = MEM32(scene + 0xFC);
    uint32_t items = MEM32(scene + 0x1A8);
    for (unsigned i = 0; items && n <= 4096 && i < n; i++)
    {
        uint32_t marker = items + i * 0xF8, a = MEM32(marker + 0xEC);
        if (!a || !(MEM8(marker + 0x14) & 1) || title_guest_ui_has_navigation(marker) || item_event(marker))
        {
            continue;
        }
        if (fabsf(MEMF(a + 0x1C) - MEMF(asset + 0x1C)) < 1 &&
            fabsf(MEMF(a + 0x20) - MEMF(asset + 0x20)) < 1 && MEM16(a + 0x54) == MEM16(asset + 0x54) &&
            MEM16(a + 0x56) == MEM16(asset + 0x56))
        {
            return 1;
        }
    }
    return 0;
}

static void add_hit(MenuHit *hits, unsigned *count, uint32_t item, int image, uint32_t mode)
{
    if (*count >= 256)
    {
        return;
    }
    unsigned event = item_event(item);
    if (!event && !title_guest_ui_has_navigation(item))
    {
        return;
    }
    uint32_t asset = MEM32(item + (image ? 0xEC : 0xF8));
    if (!asset)
    {
        return;
    }
    MenuHit h = {item,
                 MEM32(item),
                 event,
                 MEMF(asset + 0x1C),
                 MEMF(asset + 0x20),
                 (float)MEM16(asset + 0x54),
                 (float)MEM16(asset + 0x56),
                 image};
    if (!isfinite(h.x) || !isfinite(h.y) || h.w <= 0 || h.h <= 0)
    {
        return;
    }
    /* Font boxes include unused vertical space. Their visible line uses the
     * native glyph height; adjacent Options rows otherwise overlap. */
    if (!image && mode != 4 && mode != 6)
    {
        float line = (float)MEM16(asset + 0x96);
        if (line > 0 && line + 6 < h.h)
        {
            h.h = line + 6;
        }
    }
    hits[(*count)++] = h;
}

unsigned title_guest_ui_collect_hits(uint32_t scene, uint32_t mode, MenuHit *hits, uint32_t quit_entry,
                                     uint32_t native_row)
{
    hit_quit_entry = quit_entry;
    hit_native_row = native_row;
    unsigned count = 0;
    if (!scene || (mode != 2 && mode != 4 && mode != 6 && mode != 7 && mode != 5))
    {
        return 0;
    }
    int map = map_page_visible(scene, mode);
    uint32_t n = MEM32(scene + 0x100), items = MEM32(scene + 0x1AC);
    if (items && n <= 4096)
    {
        for (unsigned i = 0; i < n; i++)
        {
            uint32_t item = items + i * 0x100;
            if ((MEM8(item + 0x14) & 1) && (MEM32(item + 0xF0) & 8))
            {
                add_hit(hits, &count, item, 0, mode);
            }
        }
    }
    n = MEM32(scene + 0xFC);
    items = MEM32(scene + 0x1A8);
    if (items && n <= 4096)
    {
        for (unsigned i = 0; i < n; i++)
        {
            uint32_t item = items + i * 0xF8;
            if (map && title_guest_ui_has_navigation(item))
            {
                if (map_marker_visible(scene, item))
                {
                    add_hit(hits, &count, item, 1, mode);
                }
            }
            else if ((MEM8(item + 0x14) & 1) && (MEM32(item + 0xF0) & 8))
            {
                add_hit(hits, &count, item, 1, mode);
            }
        }
    }
    /* Warp/map icons hide their selection images until selected. Follow the
     * current page's native navigation graph, never another nested page. */
    for (unsigned i = 0; !map && i < count; i++)
    {
        if (hits[i].image)
        {
            uint32_t item = hits[i].item, links = MEM32(item + 8);
            unsigned num = MEM8(item + 5);
            for (unsigned j = 0; links && j < num; j++)
            {
                uint32_t link = links + j * 32;
                unsigned event = MEM16(link);
                if (event < 73 || event > 76 || MEM16(link + 2) != 94)
                {
                    continue;
                }
                uint32_t target = image_item(scene, MEM32(link + 4));
                if (!target)
                {
                    continue;
                }
                unsigned k;
                for (k = 0; k < count; k++)
                {
                    if (hits[k].item == target)
                    {
                        break;
                    }
                }
                if (k == count)
                {
                    add_hit(hits, &count, target, 1, mode);
                }
            }
        }
    }
    /* Save slots contain multiple text lines. Keep their whole box clickable,
     * but stop at the next row rather than overlap its hit area. */
    if (mode == 4 || mode == 6)
    {
        for (unsigned i = 0; i < count; i++)
        {
            if (!hits[i].image)
            {
                for (unsigned j = 0; j < count; j++)
                {
                    if (!hits[j].image && hits[j].y > hits[i].y && hits[j].x < hits[i].x + hits[i].w &&
                        hits[j].x + hits[j].w > hits[i].x && hits[j].y < hits[i].y + hits[i].h)
                    {
                        hits[i].h = hits[j].y - hits[i].y;
                    }
                }
            }
        }
    }
    return count;
}

uint32_t title_guest_ui_find_font(uint32_t scene, uint32_t id)
{
    if (!scene)
    {
        return 0;
    }
    uint32_t count = MEM32(scene + 0x100), items = MEM32(scene + 0x1AC);
    if (!items || count > 4096)
    {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        if (MEM32(items + i * 0x100) == id)
        {
            return items + i * 0x100;
        }
    }
    return 0;
}

uint32_t title_guest_ui_item_state(uint32_t address)
{
    return MEM32(address + 0xF0);
}

void title_guest_ui_item_state_set(uint32_t address, uint32_t value)
{
    MEM32(address + 0xF0) = value;
}

uint8_t title_guest_ui_item_flags(uint32_t address)
{
    return MEM8(address + 0x14);
}

void title_guest_ui_item_flags_set(uint32_t address, uint8_t value)
{
    MEM8(address + 0x14) = value;
}

uint32_t title_guest_ui_image_asset(uint32_t address)
{
    return MEM32(address + 0xEC);
}

void title_guest_ui_image_asset_set(uint32_t address, uint32_t value)
{
    MEM32(address + 0xEC) = value;
}

uint32_t title_guest_ui_font_asset(uint32_t address)
{
    return MEM32(address + 0xF8);
}

void title_guest_ui_font_asset_set(uint32_t address, uint32_t value)
{
    MEM32(address + 0xF8) = value;
}

uint16_t title_guest_ui_font_style(uint32_t address)
{
    return MEM16(address + 0xFC);
}

void title_guest_ui_font_style_set(uint32_t address, uint16_t value)
{
    MEM16(address + 0xFC) = value;
}

uint8_t title_guest_ui_link_count(uint32_t address)
{
    return MEM8(address + 5);
}

void title_guest_ui_link_count_set(uint32_t address, uint8_t value)
{
    MEM8(address + 5) = value;
}

uint32_t title_guest_ui_links(uint32_t address)
{
    return MEM32(address + 8);
}

void title_guest_ui_links_set(uint32_t address, uint32_t value)
{
    MEM32(address + 8) = value;
}

uint32_t title_guest_ui_scene_font_count(uint32_t address)
{
    return MEM32(address + 0x100);
}

void title_guest_ui_scene_font_count_set(uint32_t address, uint32_t value)
{
    MEM32(address + 0x100) = value;
}

uint32_t title_guest_ui_scene_fonts(uint32_t address)
{
    return MEM32(address + 0x1AC);
}

void title_guest_ui_scene_fonts_set(uint32_t address, uint32_t value)
{
    MEM32(address + 0x1AC) = value;
}

uint32_t title_guest_ui_scene_image_count(uint32_t address)
{
    return MEM32(address + 0xFC);
}

void title_guest_ui_scene_image_count_set(uint32_t address, uint32_t value)
{
    MEM32(address + 0xFC) = value;
}

uint32_t title_guest_ui_scene_images(uint32_t address)
{
    return MEM32(address + 0x1A8);
}

void title_guest_ui_scene_images_set(uint32_t address, uint32_t value)
{
    MEM32(address + 0x1A8) = value;
}

float title_guest_ui_asset_y(uint32_t address)
{
    return MEMF(address + 0x20);
}

void title_guest_ui_asset_y_set(uint32_t address, float value)
{
    MEMF(address + 0x20) = value;
}

uint32_t title_guest_ui_asset_text_id(uint32_t address)
{
    return MEM32(address + 0x80);
}

void title_guest_ui_asset_text_id_set(uint32_t address, uint32_t value)
{
    MEM32(address + 0x80) = value;
}

void title_guest_ui_write_link(uint32_t link, unsigned from, unsigned to, uint32_t target)
{
    memset((void *)XBOX_PTR(link), 0, 32);
    MEM16(link) = from;
    MEM16(link + 2) = to;
    MEM32(link + 4) = target;
}

uint32_t title_guest_ui_object_id(uint32_t item)
{
    return MEM32(item);
}

uint32_t title_guest_ui_text_length(uint32_t text)
{
    return MEM32(text);
}

unsigned title_guest_ui_link_source_event(uint32_t links, unsigned index)
{
    return MEM16(links + index * 32);
}

void title_guest_ui_set_text(uint32_t text, const char *label, unsigned length)
{
    memcpy((void *)XBOX_PTR(text + 4), label, length + 1);
    MEM32(text) = length;
}

void title_guest_ui_copy_bytes(uint32_t destination, uint32_t source, unsigned count)
{
    memcpy((void *)XBOX_PTR(destination), (void *)XBOX_PTR(source), count);
}

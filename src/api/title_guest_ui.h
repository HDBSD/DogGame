#ifndef TITLE_GUEST_UI_H
#define TITLE_GUEST_UI_H
#include <stdint.h>

/* Copied click targets retain guest identity for revalidation on the title
 * thread. Geometry is owned data; item addresses are never dereferenced by UI callbacks. */
typedef struct MenuHit
{
    uint32_t item, id, event;
    float x, y, w, h;
    int image;
} MenuHit;

unsigned title_guest_ui_collect_hits(uint32_t scene, uint32_t mode, MenuHit *hits, uint32_t quit_entry,
                                     uint32_t native_row);
int title_guest_ui_has_navigation(uint32_t item);
uint32_t title_guest_ui_find_font(uint32_t scene, uint32_t id);
/* These accessors preserve the original field widths and require a valid
 * current-scene address. The caller controls when UI changes are applied. */
uint32_t title_guest_ui_item_state(uint32_t address);
void title_guest_ui_item_state_set(uint32_t address, uint32_t value);
uint8_t title_guest_ui_item_flags(uint32_t address);
void title_guest_ui_item_flags_set(uint32_t address, uint8_t value);
uint32_t title_guest_ui_image_asset(uint32_t address);
void title_guest_ui_image_asset_set(uint32_t address, uint32_t value);
uint32_t title_guest_ui_font_asset(uint32_t address);
void title_guest_ui_font_asset_set(uint32_t address, uint32_t value);
uint16_t title_guest_ui_font_style(uint32_t address);
void title_guest_ui_font_style_set(uint32_t address, uint16_t value);
uint8_t title_guest_ui_link_count(uint32_t address);
void title_guest_ui_link_count_set(uint32_t address, uint8_t value);
uint32_t title_guest_ui_links(uint32_t address);
void title_guest_ui_links_set(uint32_t address, uint32_t value);
uint32_t title_guest_ui_scene_font_count(uint32_t address);
void title_guest_ui_scene_font_count_set(uint32_t address, uint32_t value);
uint32_t title_guest_ui_scene_fonts(uint32_t address);
void title_guest_ui_scene_fonts_set(uint32_t address, uint32_t value);
uint32_t title_guest_ui_scene_image_count(uint32_t address);
void title_guest_ui_scene_image_count_set(uint32_t address, uint32_t value);
uint32_t title_guest_ui_scene_images(uint32_t address);
void title_guest_ui_scene_images_set(uint32_t address, uint32_t value);
float title_guest_ui_asset_y(uint32_t address);
void title_guest_ui_asset_y_set(uint32_t address, float value);
uint32_t title_guest_ui_asset_text_id(uint32_t address);
void title_guest_ui_asset_text_id_set(uint32_t address, uint32_t value);
void title_guest_ui_write_link(uint32_t link, unsigned from, unsigned to, uint32_t target);
uint32_t title_guest_ui_object_id(uint32_t item);
uint32_t title_guest_ui_text_length(uint32_t text);
unsigned title_guest_ui_link_source_event(uint32_t links, unsigned index);
void title_guest_ui_set_text(uint32_t text, const char *label, unsigned length);
void title_guest_ui_copy_bytes(uint32_t destination, uint32_t source, unsigned count);
#endif

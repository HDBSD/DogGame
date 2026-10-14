/* Replace controller glyphs for this draw, without editing original assets
 * or retained dialogue strings. Native text wrapping/colour/rendering remain. */
#include "api/title_guest_text.h"
#include <xbox/input_method.h>
#include <xbox/display_options.h>
#include <string.h>
#include "api/prompt_text.h"

static uint32_t s_prompt_buffers[4];
static uint16_t s_prompt_offsets[4][4096];
static unsigned s_prompt_depth;

static void draw_prompt_with_current_input(void)
{
    uint32_t box = title_guest_text_current_box();
    uint32_t source = box ? title_guest_text_source(box) : 0;
    if (!source || !xbox_DisplayDynamicPrompts() || !xbox_InputUsesKeyboard() || s_prompt_depth >= 4)
    {
        title_prompt_draw_original();
        return;
    }
    const char *text = title_guest_text_string(source);
    size_t length = 0;
    while (length < 2048 && text[length])
    {
        length++;
    }
    if (length == 2048 || !strstr(text, "$A"))
    {
        title_prompt_draw_original();
        return;
    }
    unsigned slot = s_prompt_depth;
    if (!s_prompt_buffers[slot])
    {
        s_prompt_buffers[slot] = title_guest_text_allocate_buffer(4096);
    }
    uint32_t buffer = s_prompt_buffers[slot];
    size_t rewritten = buffer ? prompt_text_convert(text, length, title_guest_text_string(buffer), 4096,
                                                    s_prompt_offsets[slot])
                              : 0;
    if (!rewritten)
    {
        title_prompt_draw_original();
        return;
    }
    title_guest_text_set_source(box, buffer);
    s_prompt_depth++;
    title_prompt_draw_original();
    s_prompt_depth--;
    title_guest_text_set_source(box, source);
    /* Text rendering returns a continuation pointer for paged dialogue.
     * Map it back into the original string, including the terminator. */
    uint32_t continuation = title_guest_text_continuation();
    if (continuation >= buffer && continuation <= buffer + rewritten)
    {
        title_guest_text_set_continuation(source + s_prompt_offsets[slot][continuation - buffer]);
    }
}

/* ABI entry point retained for direct and indirect guest dispatch. */
void sub_0008CE00(void)
{
    draw_prompt_with_current_input();
}

/* Native text renderer retained in the guest text adapter. */

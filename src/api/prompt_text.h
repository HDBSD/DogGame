#ifndef TITLE_PROMPT_TEXT_H
#define TITLE_PROMPT_TEXT_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Convert only the game's explicitly coloured controller glyph tokens.
 * Other rich-text commands and literal punctuation stay intact. */
/* offsets maps each output byte back to the source for paged dialogue.
 * A zero result means unchanged text or insufficient output space. */
static size_t prompt_text_convert(const char *source, size_t length, char *destination, size_t capacity,
                                  uint16_t *offsets)
{
    size_t source_offset = 0, output_length = 0;
    int changed = 0;
    while (source_offset < length)
    {
        const char *key_label = NULL;
        size_t consumed = 1;
        if (source_offset + 14 <= length && source[source_offset] == '$' &&
            source[source_offset + 1] == 'A' && source[source_offset + 12] == '$' &&
            source[source_offset + 13] == 'C')
        {
            int valid_colour = 1;
            for (size_t j = source_offset + 2; j < source_offset + 11; j++)
            {
                if (source[j] < '0' || source[j] > '9')
                {
                    valid_colour = 0;
                }
            }
            if (valid_colour)
            {
                switch (source[source_offset + 11])
                {
                    case '#':
                        key_label = "[SPACE]";
                        break;
                    case '~':
                        key_label = "[L]";
                        break;
                    case '{':
                        key_label = "[J]";
                        break;
                    /* Menu Back uses a second legacy B glyph, not Y. */
                    case '}':
                        key_label = "[L]";
                        break;
                    case '|':
                        key_label = "[ESC]";
                        break;
                }
            }
            if (key_label)
            {
                consumed = 14;
            }
        }
        size_t label_length = key_label ? strlen(key_label) : 1;
        if (output_length + label_length >= capacity)
        {
            return 0; /* fail without truncating a dialogue */
        }
        for (size_t j = 0; j < label_length; j++)
        {
            destination[output_length] = key_label ? key_label[j] : source[source_offset];
            offsets[output_length++] = (uint16_t)source_offset;
        }
        if (key_label)
        {
            changed = 1;
        }
        source_offset += consumed;
    }
    destination[output_length] = 0;
    offsets[output_length] = (uint16_t)length;
    return changed ? output_length : 0;
}
#endif

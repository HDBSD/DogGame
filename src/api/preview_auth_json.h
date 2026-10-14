#ifndef PREVIEW_AUTH_JSON_H
#define PREVIEW_AUTH_JSON_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Tokens refer to spans in the original JSON. next skips the entire subtree,
 * so claim lookup can inspect direct members without matching nested claims. */
enum
{
    JSON_OBJECT = 1,
    JSON_ARRAY,
    JSON_STRING,
    JSON_PRIMITIVE
};

typedef struct
{
    int type, start, end, next;
} JsonToken;

typedef struct
{
    const char *text;
    size_t length, position;
    int token_count;
    JsonToken tokens[2048];
} JsonParser;

static int json_hex_digit(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return -1;
}

static void json_skip_whitespace(JsonParser *parser)
{
    while (parser->position < parser->length && strchr(" \t\r\n", parser->text[parser->position]))
    {
        parser->position++;
    }
}

static int json_parse_value(JsonParser *parser, int depth)
{
    json_skip_whitespace(parser);
    if (depth > 32 || parser->token_count >= 2048 || parser->position >= parser->length)
    {
        return -1;
    }
    int index = parser->token_count++;
    JsonToken *token = &parser->tokens[index];
    token->start = (int)parser->position;
    char c = parser->text[parser->position++];
    if (c == '"')
    {
        token->type = JSON_STRING;
        token->start++;
        while (parser->position < parser->length)
        {
            c = parser->text[parser->position++];
            if (c == '"')
            {
                token->end = (int)parser->position - 1;
                goto done;
            }
            if ((unsigned char)c < 32)
            {
                return -1;
            }
            if (c == '\\')
            {
                if (parser->position == parser->length)
                {
                    return -1;
                }
                c = parser->text[parser->position++];
                if (c == 'u')
                {
                    for (int k = 0; k < 4; k++)
                    {
                        if (parser->position == parser->length ||
                            json_hex_digit(parser->text[parser->position++]) < 0)
                        {
                            return -1;
                        }
                    }
                }
                else if (!strchr("\"\\/bfnrt", c))
                {
                    return -1;
                }
            }
        }
        return -1;
    }
    if (c == '{' || c == '[')
    {
        char close = c == '{' ? '}' : ']';
        token->type = c == '{' ? JSON_OBJECT : JSON_ARRAY;
        json_skip_whitespace(parser);
        if (parser->position < parser->length && parser->text[parser->position] == close)
        {
            parser->position++;
            token->end = (int)parser->position;
            goto done;
        }
        for (;;)
        {
            if (token->type == JSON_OBJECT)
            {
                int key = json_parse_value(parser, depth + 1);
                if (key < 0 || parser->tokens[key].type != JSON_STRING)
                {
                    return -1;
                }
                json_skip_whitespace(parser);
                if (parser->position == parser->length || parser->text[parser->position++] != ':')
                {
                    return -1;
                }
            }
            if (json_parse_value(parser, depth + 1) < 0)
            {
                return -1;
            }
            json_skip_whitespace(parser);
            if (parser->position == parser->length)
            {
                return -1;
            }
            c = parser->text[parser->position++];
            if (c == close)
            {
                token->end = (int)parser->position;
                goto done;
            }
            if (c != ',')
            {
                return -1;
            }
        }
    }
    token->type = JSON_PRIMITIVE;
    while (parser->position < parser->length && !strchr(",]} \t\r\n", parser->text[parser->position]))
    {
        parser->position++;
    }
    token->end = (int)parser->position;
    size_t n = (size_t)(token->end - token->start);
    const char *text = parser->text + token->start;
    if ((n == 4 && !memcmp(text, "true", 4)) || (n == 5 && !memcmp(text, "false", 5)) ||
        (n == 4 && !memcmp(text, "null", 4)))
    {
        goto done;
    }
    /* Validate JSON number grammar; claim timestamps below require integers. */
    size_t k = 0;
    if (k < n && text[k] == '-')
    {
        k++;
    }
    if (k == n)
    {
        return -1;
    }
    if (text[k] == '0')
    {
        k++;
    }
    else
    {
        if (text[k] < '1' || text[k] > '9')
        {
            return -1;
        }
        while (k < n && isdigit((unsigned char)text[k]))
        {
            k++;
        }
    }
    if (k < n && text[k] == '.')
    {
        k++;
        size_t first = k;
        while (k < n && isdigit((unsigned char)text[k]))
        {
            k++;
        }
        if (k == first)
        {
            return -1;
        }
    }
    if (k < n && (text[k] == 'e' || text[k] == 'E'))
    {
        k++;
        if (k < n && (text[k] == '+' || text[k] == '-'))
        {
            k++;
        }
        size_t first = k;
        while (k < n && isdigit((unsigned char)text[k]))
        {
            k++;
        }
        if (k == first)
        {
            return -1;
        }
    }
    if (k != n)
    {
        return -1;
    }
done:
    token->next = parser->token_count;
    return index;
}

static int json_parse(JsonParser *parser, const char *text)
{
    memset(parser, 0, sizeof *parser);
    parser->text = text;
    parser->length = strlen(text);
    if (json_parse_value(parser, 0) != 0)
    {
        return 0;
    }
    json_skip_whitespace(parser);
    return parser->position == parser->length;
}

/* Relevant claim names/values are ASCII; decode their JSON escapes rather
 * than matching substrings or accepting nested/differently named claims. */
static int json_read_string(const JsonParser *parser, int index, char *destination, size_t capacity)
{
    if (index < 0 || index >= parser->token_count || parser->tokens[index].type != JSON_STRING)
    {
        return 0;
    }
    JsonToken token = parser->tokens[index];
    size_t n = 0;
    for (int i = token.start; i < token.end; i++)
    {
        unsigned c = (unsigned char)parser->text[i];
        if (c == '\\')
        {
            c = (unsigned char)parser->text[++i];
            if (c == 'u')
            {
                c = 0;
                for (int k = 0; k < 4; k++)
                {
                    c = c * 16 + json_hex_digit(parser->text[++i]);
                }
                if (c > 127 || !c)
                {
                    return 0;
                }
            }
            else if (c == 'b')
            {
                c = '\b';
            }
            else if (c == 'f')
            {
                c = '\f';
            }
            else if (c == 'n')
            {
                c = '\n';
            }
            else if (c == 'r')
            {
                c = '\r';
            }
            else if (c == 't')
            {
                c = '\t';
            }
        }
        if (n + 1 >= capacity)
        {
            return 0;
        }
        destination[n++] = (char)c;
    }
    destination[n] = 0;
    return 1;
}

static int json_string_equals(const JsonParser *parser, int index, const char *text)
{
    char value[512];
    return json_read_string(parser, index, value, sizeof value) && !strcmp(value, text);
}

/* Duplicate matching members are ambiguous and must fail claim validation. */
static int json_find_member(const JsonParser *parser, int object, const char *name)
{
    if (object < 0 || parser->tokens[object].type != JSON_OBJECT)
    {
        return -1;
    }
    int result = -1;
    for (int i = object + 1; i < parser->tokens[object].next;)
    {
        int value = i + 1;
        if (json_string_equals(parser, i, name))
        {
            if (result >= 0)
            {
                return -1;
            }
            result = value;
        }
        i = parser->tokens[value].next;
    }
    return result;
}

static int json_array_contains(const JsonParser *parser, int index, const char *value)
{
    if (index < 0 || parser->tokens[index].type != JSON_ARRAY)
    {
        return 0;
    }
    for (int i = index + 1; i < parser->tokens[index].next; i = parser->tokens[i].next)
    {
        if (json_string_equals(parser, i, value))
        {
            return 1;
        }
    }
    return 0;
}

static int json_read_integer(const JsonParser *parser, int index, int64_t *value)
{
    if (index < 0 || parser->tokens[index].type != JSON_PRIMITIVE)
    {
        return 0;
    }
    JsonToken token = parser->tokens[index];
    if (token.end - token.start <= 0 || token.end - token.start > 16)
    {
        return 0;
    }
    int64_t n = 0;
    for (int i = token.start; i < token.end; i++)
    {
        char c = parser->text[i];
        if (c < '0' || c > '9')
        {
            return 0;
        }
        n = n * 10 + c - '0';
    }
    *value = n;
    return 1;
}

/* JWT and PKCE use the URL-safe alphabet without padding. */
static size_t base64url_encode(const unsigned char *source, size_t length, char *destination, size_t capacity)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    size_t n = 0;
    unsigned bit_count = 0, accumulator = 0;
    for (size_t i = 0; i < length; i++)
    {
        accumulator = (accumulator << 8) | source[i];
        bit_count += 8;
        while (bit_count >= 6)
        {
            bit_count -= 6;
            if (n + 1 >= capacity)
            {
                return 0;
            }
            destination[n++] = alphabet[(accumulator >> bit_count) & 63];
        }
    }
    if (bit_count)
    {
        if (n + 1 >= capacity)
        {
            return 0;
        }
        destination[n++] = alphabet[(accumulator << (6 - bit_count)) & 63];
    }
    destination[n] = 0;
    return n;
}

static size_t base64url_decode(const char *source, size_t length, unsigned char *destination, size_t capacity)
{
    const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    if (length % 4 == 1)
    {
        return 0;
    }
    unsigned bit_count = 0, accumulator = 0;
    size_t n = 0;
    for (size_t i = 0; i < length; i++)
    {
        const char *p = source[i] ? strchr(alphabet, source[i]) : NULL;
        if (!p)
        {
            return 0;
        }
        accumulator = (accumulator << 6) | (unsigned)(p - alphabet);
        bit_count += 6;
        if (bit_count >= 8)
        {
            bit_count -= 8;
            if (n >= capacity)
            {
                return 0;
            }
            destination[n++] = (unsigned char)(accumulator >> bit_count);
        }
    }
    if (bit_count && (accumulator & ((1u << bit_count) - 1)))
    {
        return 0;
    }
    return n;
}
#endif

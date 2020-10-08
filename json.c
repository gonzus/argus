#include <stdlib.h>
#include <string.h>
#include <log.h>
#include <ctype.h>
#include "json.h"

/*
 * STATES:
 * ------
 * start
 * array_elem
 * array_comma
 * hash_key
 * hash_colon
 * hash_value
 * hash_comma
 *
 * STACK:
 * -----
 * array
 * hash
 */
enum Memory {
    MEMORY_ARRAY,
    MEMORY_HASH,
};

enum State {
    STATE_START,
    STATE_ARRAY_ELEM,
    STATE_ARRAY_COMMA,
    STATE_HASH_KEY,
    STATE_HASH_COLON,
    STATE_HASH_VALUE,
    STATE_HASH_COMMA,
    STATE_END,
};

JSON* json_create(void) {
    JSON* json = (JSON*) malloc(sizeof(JSON));
    memset(json, 0, sizeof(JSON));
    json->stack = stack_create();
    json->string = buffer_build();
    return json;
}

void json_destroy(JSON* json) {
    buffer_destroy(json->string);
    json->string = 0;
    stack_destroy(json->stack);
    json->stack = 0;
    free((void*) json);
    json = 0;
}

void json_clear(JSON* json) {
    stack_clear(json->stack);
}

int json_validate(JSON* json, const char* ptr, int len) {
    int valid = 1;
    int popped = 0;
    int number = 0;
    int state = STATE_START;
    int pos = 0;
    json_clear(json);
    for (pos = 0; valid && state != STATE_END && pos < len; ) {
        unsigned char c = ptr[pos++];

        if (c == '#') {
            // comment
            // skip to EOL
            while (pos < len) {
                c = ptr[pos++];
                if (c == '\n') {
                    break;
                }
            }
            continue;
        }

        if (c == '\'' || c == '\"') {
            // single or double quoted string
            // read until closing quote
            unsigned char b = c;
            buffer_clear(json->string);
            int ok = 0;
            while (pos < len) {
                c = ptr[pos++];
                if (c == b) {
                    LOG_INFO("EOS [%d:%.*s]", json->string->len, json->string->len, json->string->ptr);
                    ok = 1;
                    break;
                }
                if (c == '\\') {
                    c = ptr[pos++];
                }
                buffer_append_byte(json->string, c);
            }
            if (!ok) {
                LOG_INFO("INVALID STRING");
                valid = 0;
                break;
            }
            switch (state) {
                case STATE_START:
                    state = STATE_END;
                    break;
                case STATE_ARRAY_ELEM:
                    state = STATE_ARRAY_COMMA;
                    break;
                case STATE_HASH_KEY:
                    state = STATE_HASH_COLON;
                    break;
                case STATE_HASH_VALUE:
                    state = STATE_HASH_COMMA;
                    break;
                default:
                    LOG_INFO("INVALID STRING");
                    valid = 0;
                    break;
            }
            continue;
        }

        if (isdigit(c)) {
            // number
            // TODO: handle signs
            number = c - '0';
            while (pos < len) {
                c = ptr[pos++];
                if (isdigit(c)) {
                    number = number * 10 + c - '0';
                    continue;
                }
                --pos;
                LOG_INFO("EON [%d]", number);
                break;
            }
            switch (state) {
                case STATE_START:
                    state = STATE_END;
                    break;
                case STATE_ARRAY_ELEM:
                    state = STATE_ARRAY_COMMA;
                    break;
                case STATE_HASH_KEY:
                    state = STATE_HASH_COLON;
                    break;
                case STATE_HASH_VALUE:
                    state = STATE_HASH_COMMA;
                    break;
                default:
                    LOG_INFO("INVALID NUMBER");
                    valid = 0;
                    break;
            }
            continue;
        }

        if (isspace(c)) {
            // whitespace
            // skip
            continue;
        }

        switch (c) {
            case '[':
                LOG_INFO("BOA");
                switch (state) {
                    case STATE_START:
                    case STATE_ARRAY_ELEM:
                    case STATE_HASH_VALUE:
                        state = STATE_ARRAY_ELEM;
                        break;
                    default:
                        LOG_INFO("INVALID '['");
                        valid = 0;
                        break;
                }
                if (stack_push(json->stack, MEMORY_ARRAY)) {
                    LOG_INFO("OVERFLOW");
                    valid = 0;
                }
                break;

            case ']':
                LOG_INFO("EOA");
                switch (state) {
                    case STATE_ARRAY_ELEM:
                    case STATE_ARRAY_COMMA:
                        state = STATE_ARRAY_ELEM;
                        break;
                    default:
                        LOG_INFO("INVALID ']'");
                        valid = 0;
                        break;
                }
                if (stack_pop(json->stack, &popped)) {
                    LOG_INFO("UNDERFLOW");
                    valid = 0;
                    break;
                }
                if (popped != MEMORY_ARRAY) {
                    LOG_INFO("INVALID ARRAY");
                    valid = 0;
                    break;
                }
                if (stack_empty(json->stack)) {
                    state = STATE_END;
                    break;
                }
                break;

            case '{':
                LOG_INFO("BOH");
                switch (state) {
                    case STATE_START:
                    case STATE_ARRAY_ELEM:
                    case STATE_HASH_VALUE:
                        state = STATE_HASH_KEY;
                        break;
                    default:
                        LOG_INFO("INVALID '{'");
                        valid = 0;
                        break;
                }
                if (stack_push(json->stack, MEMORY_HASH)) {
                    LOG_INFO("OVERFLOW");
                    valid = 0;
                }
                break;

            case '}':
                LOG_INFO("EOH");
                switch (state) {
                    case STATE_HASH_KEY:
                    case STATE_HASH_COMMA:
                        state = STATE_HASH_KEY;
                        break;
                    default:
                        LOG_INFO("INVALID '}'");
                        valid = 0;
                        break;
                }
                if (stack_pop(json->stack, &popped)) {
                    LOG_INFO("UNDERFLOW");
                    valid = 0;
                    break;
                }
                if (popped != MEMORY_HASH) {
                    LOG_INFO("INVALID HASH");
                    valid = 0;
                    break;
                }
                if (stack_empty(json->stack)) {
                    state = STATE_END;
                    break;
                }
                break;

            case ',':
                LOG_INFO("COMMA");
                switch (state) {
                    case STATE_ARRAY_COMMA:
                        state = STATE_ARRAY_ELEM;
                        break;
                    case STATE_HASH_COMMA:
                        state = STATE_HASH_KEY;
                        break;
                    default:
                        LOG_INFO("INVALID ','");
                        valid = 0;
                        break;
                }
                break;

            case ':':
                LOG_INFO("COLON");
                switch (state) {
                    case STATE_HASH_COLON:
                        state = STATE_HASH_VALUE;
                        break;
                    default:
                        LOG_INFO("INVALID ':'");
                        valid = 0;
                        break;
                }
                break;

            default:
                LOG_INFO("HUH?");
                break;
        }
    }

    return (valid &&
            pos == len &&
            (state == STATE_END || state == STATE_START) &&
            stack_empty(json->stack));
}

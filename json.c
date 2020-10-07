#include <stdlib.h>
#include <string.h>
#include <log.h>
#include <ctype.h>
#include "json.h"

enum State {
    STATE_SCALAR,
    STATE_ARRAY_ELEMENT,
    STATE_HASH_KEY,
    STATE_HASH_VALUE,
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
    for (int p = 0; valid && p < len; ) {
        unsigned char c = ptr[p++];
        if (c == '#') {
            while (p < len) {
                c = ptr[p++];
                if (c == '\n') {
                    break;
                }
            }
            continue;
        }
        if (c == '\'' || c == '\"') {
            unsigned char b = c;
            buffer_clear(json->string);
            while (p < len) {
                c = ptr[p++];
                if (c == b) {
                    LOG_DEBUG("EOS [%d:%.*s]", json->string->pos, json->string->pos, json->string->ptr);
                    break;
                }
                if (c == '\\') {
                    c = ptr[p++];
                }
                buffer_append_byte(json->string, c);
            }
            continue;
        }

        // TODO: handle signs
        if (isdigit(c)) {
            number = c - '0';
            while (p < len) {
                c = ptr[p++];
                if (isdigit(c)) {
                    number = number * 10 + c - '0';
                    continue;
                }
                --p;
                LOG_DEBUG("EON [%d]", number);
                break;
            }
            continue;
        }

        if (isspace(c)) {
            continue;
        }

        switch (c) {
            case '[':
                LOG_DEBUG("BOA");
                if (stack_push(json->stack, STATE_ARRAY_ELEMENT)) {
                    LOG_WARNING("OVERFLOW");
                    valid = 0;
                }
                break;
            case ']':
                LOG_DEBUG("EOA");
                if (stack_pop(json->stack, &popped)) {
                    LOG_WARNING("UNDERFLOW");
                    valid = 0;
                }
                break;
            case '{':
                LOG_DEBUG("BOH");
                if (stack_push(json->stack, STATE_HASH_KEY)) {
                    LOG_WARNING("OVERFLOW");
                    valid = 0;
                }
                break;
            case '}':
                LOG_DEBUG("EOH");
                if (stack_pop(json->stack, &popped)) {
                    LOG_WARNING("UNDERFLOW");
                    valid = 0;
                }
                break;
            case ',':
                if (stack_top(json->stack, &popped)) {
                    LOG_WARNING("UNDERFLOW");
                    valid = 0;
                    break;
                }
                switch (popped) {
                    case STATE_ARRAY_ELEMENT:
                        LOG_DEBUG("AE");
                        break;
                    case STATE_HASH_VALUE:
                        LOG_DEBUG("HK");
                        if (stack_set(json->stack, STATE_HASH_KEY)) {
                            LOG_WARNING("UNDERFLOW");
                            valid = 0;
                        }
                        break;
                    default:
                        /* ERROR */
                        valid = 0;
                        break;
                }
                break;
            case ':':
                if (stack_top(json->stack, &popped)) {
                    LOG_WARNING("UNDERFLOW");
                    valid = 0;
                    break;
                }
                switch (popped) {
                    case STATE_HASH_KEY:
                        LOG_DEBUG("HV");
                        if (stack_set(json->stack, STATE_HASH_VALUE)) {
                            LOG_WARNING("UNDERFLOW");
                            valid = 0;
                        }
                        break;
                    default:
                        /* ERROR */
                        valid = 0;
                        break;
                }
                break;
            default:
                LOG_DEBUG("HUH?");
                break;
        }
    }
    return valid && stack_empty(json->stack);
}

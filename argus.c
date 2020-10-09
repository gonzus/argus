#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <log.h>
#include "argus.h"

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

Argus* argus_create(void) {
    Argus* argus = (Argus*) malloc(sizeof(Argus));
    memset(argus, 0, sizeof(Argus));
    argus->stack = stack_create();
    argus->string = buffer_build();
    return argus;
}

void argus_destroy(Argus* argus) {
    buffer_destroy(argus->string);
    argus->string = 0;
    stack_destroy(argus->stack);
    argus->stack = 0;
    free((void*) argus);
    argus = 0;
}

void argus_clear(Argus* argus) {
    stack_clear(argus->stack);
}

int argus_parse_buffer(Argus* argus, const char* ptr, int len) {
    int valid = 1;
    int popped = 0;
    int state = STATE_START;
    int pos = 0;
    argus_clear(argus);
    while (pos < len) {
        if (!valid) {
            break;
        }
#if 0
        if (state == STATE_END) {
            break;
        }
#endif
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
            buffer_clear(argus->string);
            int ok = 0;
            while (pos < len) {
                c = ptr[pos++];
                if (c == b) {
                    LOG_INFO("EOS [%d:%.*s]", argus->string->len, argus->string->len, argus->string->ptr);
                    ok = 1;
                    break;
                }
                if (c == '\\') {
                    c = ptr[pos++];
                }
                buffer_append_byte(argus->string, c);
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

        if (c == '-' || c == '+' || c == '.' || isdigit(c)) {
            // number
            int sign = 1;
            int cnt[2] = {0,0};
            int num[2] = {0,0};
            int dot = 0;
            switch (c) {
                case '-':
                    sign = -1;
                    break;
                case '+':
                    sign = +1;
                    break;
                case '.':
                    ++dot;
                    break;
                default:
                    num[dot] = num[dot] * 10 + c - '0';
                    ++cnt[dot];
                    break;
            }
            while (pos < len) {
                c = ptr[pos++];
                if (c == '.') {
                    if (dot > 0) {
                        break;
                    }
                    ++dot;
                    continue;
                }
                if (!isdigit(c)) {
                    --pos;
                    break;
                }
                num[dot] = num[dot] * 10 + c - '0';
                ++cnt[dot];
            }
            if (!cnt[0] && !cnt[1]) {
                LOG_INFO("INVALID NUMBER");
                valid = 0;
                break;
            }
            LOG_INFO("EON [%d:%d:%d]", sign, num[0], num[1]);
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

        if (c == 't') {
            if ((len - pos) >= 3 &&
                ptr[pos+0] == 'r' &&
                ptr[pos+1] == 'u' &&
                ptr[pos+2] == 'e') {
                LOG_INFO("true");
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
                        LOG_INFO("INVALID true");
                        valid = 0;
                        break;
                }
                continue;
            }
        }

        if (c == 'f') {
            if ((len - pos) >= 4 &&
                ptr[pos+0] == 'a' &&
                ptr[pos+1] == 'l' &&
                ptr[pos+2] == 's' &&
                ptr[pos+3] == 'e') {
                LOG_INFO("false");
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
                        LOG_INFO("INVALID false");
                        valid = 0;
                        break;
                }
                continue;
            }
        }

        if (c == 'n') {
            if ((len - pos) >= 3 &&
                ptr[pos+0] == 'u' &&
                ptr[pos+1] == 'l' &&
                ptr[pos+2] == 'l') {
                LOG_INFO("null");
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
                        LOG_INFO("INVALID null");
                        valid = 0;
                        break;
                }
                continue;
            }
        }

        if (isspace(c)) {
            // whitespace
            // skip
            continue;
        }

        switch (c) {
            case '[':
                LOG_INFO("BOA %d", stack_size(argus->stack) + 1);
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
                if (stack_push(argus->stack, MEMORY_ARRAY)) {
                    LOG_INFO("OVERFLOW");
                    valid = 0;
                }
                break;

            case ']':
                LOG_INFO("EOA %d", stack_size(argus->stack));
                switch (state) {
                    case STATE_ARRAY_ELEM:
                    case STATE_ARRAY_COMMA:
                        state = STATE_ARRAY_ELEM;
                        break;
                    default:
                        LOG_INFO("INVALID ']', state %d", state);
                        valid = 0;
                        break;
                }
                if (stack_pop(argus->stack, &popped)) {
                    LOG_INFO("UNDERFLOW");
                    valid = 0;
                    break;
                }
                if (popped != MEMORY_ARRAY) {
                    LOG_INFO("INVALID ARRAY");
                    valid = 0;
                    break;
                }
                if (stack_top(argus->stack, &popped)) {
                    state = STATE_END;
                    break;
                }
                switch (popped) {
                    case MEMORY_ARRAY:
                        state = STATE_ARRAY_COMMA;
                        break;
                    case MEMORY_HASH:
                        state = STATE_HASH_COMMA;
                        break;
                }
                break;

            case '{':
                LOG_INFO("BOH %d", stack_size(argus->stack) + 1);
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
                if (stack_push(argus->stack, MEMORY_HASH)) {
                    LOG_INFO("OVERFLOW");
                    valid = 0;
                }
                break;

            case '}':
                LOG_INFO("EOH %d", stack_size(argus->stack));
                switch (state) {
                    case STATE_HASH_KEY:
                    case STATE_HASH_COMMA:
                        state = STATE_HASH_KEY;
                        break;
                    default:
                        LOG_INFO("INVALID '}', state %d", state);
                        valid = 0;
                        break;
                }
                if (stack_pop(argus->stack, &popped)) {
                    LOG_INFO("UNDERFLOW");
                    valid = 0;
                    break;
                }
                if (popped != MEMORY_HASH) {
                    LOG_INFO("INVALID HASH");
                    valid = 0;
                    break;
                }
                if (stack_top(argus->stack, &popped)) {
                    state = STATE_END;
                    break;
                }
                switch (popped) {
                    case MEMORY_ARRAY:
                        state = STATE_ARRAY_COMMA;
                        break;
                    case MEMORY_HASH:
                        state = STATE_HASH_COMMA;
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
            stack_empty(argus->stack));
}

int argus_parse_file(Argus* argus, const char* name) {
    int valid = 0;
    int fd = -1;
    char* data = 0;
    int size = 0;

    do {
        fd = open(name, O_RDONLY);
        if (fd < 0) {
            LOG_ERROR("Cannot open [%s]", name);
            break;
        }
        LOG_DEBUG("Opened file [%s] as descriptor %d", name, fd);

        struct stat st;
        int status = fstat(fd, &st);
        if (status < 0) {
            LOG_ERROR("Cannot stat [%s]", name);
            break;
        }
        size = st.st_size;

        data = mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            LOG_ERROR("Cannot mmap [%s]", name);
            data = 0;
            break;
        }

        LOG_INFO("Mapped file [%s] with %u bytes at %p", name, size, data);
        valid = argus_parse_buffer(argus, data, size);
    } while (0);

    if (data) {
        int rc = munmap(data, size);
        if (rc < 0) {
            LOG_ERROR("Cannot unmap [%s] from %p", name, data);
        }
        LOG_DEBUG("Unmapped file [%s] from %p", name, data);
        data = 0;
    }
    if (fd >= 0) {
        int rc = close(fd);
        if (rc < 0) {
            LOG_ERROR("Cannot close [%s]", name);
        }
        LOG_DEBUG("Closed file [%s] as descriptor %d", name, fd);
        fd = -1;
    }

    return valid;
}

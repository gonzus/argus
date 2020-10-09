#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <log.h>
#include "argus.h"

#define ADD_DIGIT(num, cnt, pos, dig) \
    do { \
        num[pos] = num[pos] * 10 + dig - '0'; \
        ++cnt[pos]; \
    } while (0)

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
    STATE_INVALID,
};

static int parse_string(Argus* argus, char quote, const char* ptr, int pos, int len);
static int parse_number(Argus* argus, char c, const char* ptr, int pos, int len);
static int parse_fixed(const char* fixed, int state, const char* ptr, int pos, int len);
static int state_after_scalar(int state);

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
        unsigned char c = ptr[pos++];

        if (c == '#') {
            // comment -- skip to EOL
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
            int n = parse_string(argus, c, ptr, pos, len);
            if (!n) {
                LOG_INFO("INVALID STRING");
                valid = 0;
                break;
            }
            pos = n;
            state = state_after_scalar(state);
            if (state == STATE_INVALID) {
                LOG_INFO("INVALID STRING");
                valid = 0;
                break;
            }
            continue;
        }

        if (c == '-' || c == '+' || c == '.' || isdigit(c)) {
            // number
            int n = parse_number(argus, c, ptr, pos, len);
            if (!n) {
                LOG_INFO("INVALID NUMBER");
                valid = 0;
                break;
            }
            pos = n;
            state = state_after_scalar(state);
            if (state == STATE_INVALID) {
                LOG_INFO("INVALID NUMBER");
                valid = 0;
                break;
            }
            continue;
        }

        if (c == 't') {
            state = parse_fixed("true", state, ptr, pos, len);
            if (state == STATE_INVALID) {
                LOG_INFO("INVALID true");
                valid = 0;
                break;
            }
            pos += 3;
            continue;
        }

        if (c == 'f') {
            state = parse_fixed("false", state, ptr, pos, len);
            if (state == STATE_INVALID) {
                LOG_INFO("INVALID false");
                valid = 0;
                break;
            }
            pos += 4;
            continue;
        }

        if (c == 'n') {
            state = parse_fixed("null", state, ptr, pos, len);
            if (state == STATE_INVALID) {
                LOG_INFO("INVALID null");
                valid = 0;
                break;
            }
            pos += 3;
            continue;
        }

        if (isspace(c)) {
            // whitespace -- skip
            continue;
        }

        // here we parse arrays and hashes
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
                valid = 0;
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

static int parse_string(Argus* argus, char quote, const char* ptr, int pos, int len) {
    buffer_clear(argus->string);
    int ok = 0;
    // read until closing quote
    while (pos < len) {
        unsigned char c = ptr[pos++];
        if (c == quote) {
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
        return 0;
    }
    return pos;
}

static int parse_number(Argus* argus, char c, const char* ptr, int pos, int len) {
    (void) argus;
    int sign = 1;
    int num[2] = {0,0}; // number so far, before / after decimal point
    int cnt[2] = {0,0}; // count of digits so far, before / after decimal point
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
            ADD_DIGIT(num, cnt, dot, c);
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
        ADD_DIGIT(num, cnt, dot, c);
    }
    if (!cnt[0] && !cnt[1]) {
        return 0;
    }
    LOG_INFO("EON [%d:%d:%d]", sign, num[0], num[1]);
    return pos;
}

static int parse_fixed(const char* fixed, int state, const char* ptr, int pos, int len) {
    do {
        int flen = strlen(fixed) - 1;
        if ((len - pos) < flen) {
            state = STATE_INVALID;
            break;
        }
        if (memcmp(ptr + pos, fixed + 1, flen) != 0) {
            state = STATE_INVALID;
            break;
        }
        state = state_after_scalar(state);
    } while (0);

    if (state != STATE_INVALID) {
        LOG_INFO("%s", fixed);
    }
    return state;
}

static int state_after_scalar(int state) {
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
            state = STATE_INVALID;
            break;
    }
    return state;
}

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define STACK_MAX_DEPTH 1024

#define DEBUG 0

#if DEBUG
#define LOG(x) do { printf x; } while (0)
#else
#define LOG(x) do {} while (0)
#endif

#define STACK_PUSH(sa, sp, s) \
    ( \
        (++sp >= STACK_MAX_DEPTH) ? 0 : (sa[sp] = s, max_depth < sp ? max_depth = sp : 1, 1) \
    )
#define STACK_SET(sa, sp, s) \
    ( \
        sa[sp] = s, 1 \
    )
#define STACK_POP(sa, sp) \
    ( \
        (--sp < 0) ? 0 : 1 \
    )
#define STACK_TOP(sa, sp) \
    sa[sp]

enum State {
    STATE_SCALAR,
    STATE_ARRAY_ELEM,
    STATE_HASH_KEY,
    STATE_HASH_VALUE,
};

static int max_depth = 0;
static int max_len = 0;

int validate_string(const unsigned char* data, int len) {
    int valid = 1;
    int number = 0;
    char string[1024*1024];
    int slen = 0;
    int sa[STACK_MAX_DEPTH];
    int sp = 0;
    max_depth = 0;
    max_len = 0;
    STACK_SET(sa, sp, STATE_SCALAR);
    for (int p = 0; valid && p < len; ) {
        int c = data[p++];
        if (c == '#') {
            while (p < len) {
                c = data[p++];
                if (c == '\n') {
                    break;
                }
            }
            continue;
        }
        if (c == '\'' || c == '\"') {
            int b = c;
            slen = 0;
            while (p < len) {
                c = data[p++];
                if (c == b) {
                    string[slen] = '\0';
                    LOG(("EOS [%d:%s]\n", slen, string));
                    if (max_len < slen) {
                        max_len = slen;
                    }
                    break;
                }
                if (c == '\\') {
                    c = data[p++];
                }
                string[slen++] = c;
            }
            continue;
        }

        // TODO: handle signs
        if (isdigit(c)) {
            number = c - '0';
            while (p < len) {
                c = data[p++];
                if (isdigit(c)) {
                    number = number * 10 + c - '0';
                    continue;
                }
                --p;
                LOG(("EON [%d]\n", number));
                break;
            }
            continue;
        }

        if (isspace(c)) {
            continue;
        }

        switch (c) {
            case '[':
                LOG(("BOA\n"));
                if (!STACK_PUSH(sa, sp, STATE_ARRAY_ELEM)) {
                    LOG(("OVERFLOW\n"));
                    valid = 0;
                }
                break;
            case ']':
                LOG(("EOA\n"));
                if (!STACK_POP(sa, sp)) {
                    LOG(("UNDERFLOW\n"));
                    valid = 0;
                }
                break;
            case '{':
                LOG(("BOH\n"));
                if (!STACK_PUSH(sa, sp, STATE_HASH_KEY)) {
                    LOG(("OVERFLOW\n"));
                    valid = 0;
                }
                break;
            case '}':
                LOG(("EOH\n"));
                if (!STACK_POP(sa, sp)) {
                    LOG(("UNDERFLOW\n"));
                    valid = 0;
                }
                break;
            case ',':
                switch (STACK_TOP(sa, sp)) {
                    case STATE_ARRAY_ELEM:
                        LOG(("AE\n"));
                        break;
                    case STATE_HASH_VALUE:
                        LOG(("HK\n"));
                        STACK_SET(sa, sp, STATE_HASH_KEY);
                        break;
                    default:
                        /* ERROR */
                        valid = 0;
                        break;
                }
                break;
            case ':':
                switch (STACK_TOP(sa, sp)) {
                    case STATE_HASH_KEY:
                        LOG(("HV\n"));
                        STACK_SET(sa, sp, STATE_HASH_VALUE);
                        break;
                    default:
                        /* ERROR */
                        valid = 0;
                        break;
                }
                break;
            default:
                LOG(("HUH?\n"));
                break;
        }
    }
    printf("max_depth: %d -- max_len: %d\n", max_depth, max_len);
    return valid && sp == 0 && STACK_TOP(sa, sp) == STATE_SCALAR;
}

int validate_file(const char* name) {
    int valid = 0;
    int fd = -1;
    unsigned char* data = 0;
    int size = 0;

    do {
        fd = open(name, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "Cannot open [%s]\n", name);
            break;
        }
        LOG(("Opened file [%s] as descriptor %d\n", name, fd));

        struct stat st;
        int status = fstat(fd, &st);
        if (status < 0) {
            fprintf(stderr, "Cannot stat [%s]\n", name);
            break;
        }
        size = st.st_size;

        data = (unsigned char*) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            fprintf(stderr, "Cannot mmap [%s]\n", name);
            data = 0;
            break;
        }

        LOG(("Mapped file [%s] at %p\n", name, data));
        valid = validate_string(data, size);
    } while (0);

    if (data) {
        int rc = munmap(data, size);
        if (rc < 0) {
            fprintf(stderr, "Cannot unmap [%s] from %p\n", name, data);
        }
        LOG(("Unmapped file [%s] from %p\n", name, data));
        data = 0;
    }
    if (fd >= 0) {
        int rc = close(fd);
        if (rc < 0) {
            fprintf(stderr, "Cannot close [%s]\n", name);
        }
        LOG(("Closed file [%s] as descriptor %d\n", name, fd));
        fd = -1;
    }

    return valid;
}

int main(int argc, char* argv[]) {
    for (int j = 1; j < argc; ++j) {
        const char* name = argv[j];
        int valid = validate_file(name);
        printf("%-3.3s %s\n", valid ? "OK" : "BAD", name);
    }
    return 0;
}

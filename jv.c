#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define DEBUG 0

#if DEBUG
#define LOG(x) do { printf x; } while (0)
#else
#define LOG(x) do {} while (0)
#endif

#define STACK_SET_INC(sa, sp, s) \
    ( \
        (++sp >= STACK_MAX_DEPTH) ? 0 : (sa[sp] = s, max_depth < sp ? max_depth = sp : 1, 1) \
    )
#define STACK_SET(sa, sp, s) \
    ( \
        sa[sp] = s, 1 \
    )
#define STACK_DEC(sa, sp) \
    ( \
        (--sp < 0) ? 0 : 1 \
    )
#define STACK_GET(sa, sp) \
    sa[sp]

enum Stack {
    STACK_MAX_DEPTH = 1024,
};

enum State {
    STATE_SCALAR,
    STATE_ARRAY_ELEM,
    STATE_HASH_KEY,
    STATE_HASH_VALUE,
};

static int max_depth = 0;
static int max_len = 0;

static int valid(const unsigned char* data, int len) {
    int ok = 1;
    int done = 0;
    int number = 0;
    char string[1024*1024];
    int slen = 0;
    int sa[STACK_MAX_DEPTH];
    int sp = 0;
    max_depth = 0;
    max_len = 0;
    STACK_SET(sa, sp, STATE_SCALAR);
    for (int p = 0; !done && p < len; ) {
        int c = data[p++];
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
                continue;
            }
            continue;
        }

        if (isdigit(c)) {
            number = c - '0';
            while (p < len) {
                c = data[p++];
                if (isdigit(c)) {
                    number = number * 10 + c - '0';
                    continue;
                } else {
                    --p;
                    LOG(("EON [%d]\n", number));
                }
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
                if (!STACK_SET_INC(sa, sp, STATE_ARRAY_ELEM)) {
                    LOG(("OVERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
                break;
            case ']':
                LOG(("EOA\n"));
                if (!STACK_DEC(sa, sp)) {
                    LOG(("UNDERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
                break;
            case '{':
                LOG(("BOH\n"));
                if (!STACK_SET_INC(sa, sp, STATE_HASH_KEY)) {
                    LOG(("OVERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
                break;
            case '}':
                LOG(("EOH\n"));
                if (!STACK_DEC(sa, sp)) {
                    LOG(("UNDERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
                break;
            case ',':
                switch (STACK_GET(sa, sp)) {
                    case STATE_ARRAY_ELEM:
                        LOG(("AE\n"));
                        break;
                    case STATE_HASH_VALUE:
                        LOG(("HK\n"));
                        STACK_SET(sa, sp, STATE_HASH_KEY);
                        break;
                    default:
                        /* ERROR */
                        ok = 0;
                        done = 1;
                        break;
                }
                break;
            case ':':
                switch (STACK_GET(sa, sp)) {
                    case STATE_HASH_KEY:
                        LOG(("HV\n"));
                        STACK_SET(sa, sp, STATE_HASH_VALUE);
                        break;
                    default:
                        /* ERROR */
                        ok = 0;
                        done = 1;
                        break;
                }
                break;
            default:
                break;
        }
    }
    printf("max_depth: %d -- max_len: %d\n", max_depth, max_len);
    return ok && sp == 0 && STACK_GET(sa, sp) == STATE_SCALAR;
}

static void process(const char* name) {
    int fd = -1;
    do {
        fd = open(name, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "Cannot open [%s]\n", name);
            break;
        }

        struct stat st;
        int status = fstat(fd, &st);
        if (status < 0) {
            fprintf(stderr, "Cannot stat [%s]\n", name);
            break;
        }
        int size = st.st_size;

        unsigned char* data = (unsigned char*) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            fprintf(stderr, "Cannot mmap [%s]\n", name);
            break;
        }

        int v = valid(data, size);
        printf("%-3.3s %s\n", v ? "OK" : "BAD", name);
    } while (0);
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

int main(int argc, char* argv[]) {
    for (int j = 1; j < argc; ++j) {
        process(argv[j]);
    }
    return 0;
}

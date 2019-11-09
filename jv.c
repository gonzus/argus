#include <ctype.h>
#include <stdio.h>

#define DEBUG 1

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
    STATE_SPACE,
    STATE_NUMBER,
    STATE_STRING_SINGLE,
    STATE_STRING_DOUBLE,
    STATE_BACKSLASH,
    STATE_ARRAY_ELEM,
    STATE_HASH_KEY,
    STATE_HASH_VALUE,
};

static int max_depth = 0;

int valid(FILE* fp) {
    int ok = 1;
    int done = 0;
    int number = 0;
    int sa[STACK_MAX_DEPTH];
    int sp = 0;
    max_depth = 0;
    STACK_SET(sa, sp, STATE_SPACE);
    while (!done) {
        int c = getc(fp);
        if (c == EOF) {
            LOG(("EOF\n"));
            done = 1;
            continue;
        }
        if (STACK_GET(sa, sp) == STATE_BACKSLASH) {
            LOG(("EOBS\n"));
            if (!STACK_DEC(sa, sp)) {
                LOG(("UNDERFLOW\n"));
                ok = 0;
                done = 1;
            }
            continue;
        }
        if (STACK_GET(sa, sp) == STATE_STRING_DOUBLE ||
            STACK_GET(sa, sp) == STATE_STRING_SINGLE) {
            if (c == '\\') {
                LOG(("BOBS\n"));
                if (!STACK_SET_INC(sa, sp, STATE_BACKSLASH)) {
                    LOG(("OVERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
            }
            if (c == '\'' && STACK_GET(sa, sp) == STATE_STRING_SINGLE) {
                LOG(("EOSQ\n"));
                if (!STACK_DEC(sa, sp)) {
                    LOG(("UNDERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
            }
            if (c == '\"' && STACK_GET(sa, sp) == STATE_STRING_DOUBLE) {
                LOG(("EODQ\n"));
                if (!STACK_DEC(sa, sp)) {
                    LOG(("UNDERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
            }
            continue;
        }
        if (STACK_GET(sa, sp) == STATE_NUMBER) {
            if (c >= '0' && c <= '9') {
                number = number * 10 + c - '0';
                continue;
            } else {
                LOG(("EON %d\n", number));
                if (!STACK_DEC(sa, sp)) {
                    LOG(("UNDERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
                // we don't continue in this case
                // need to process the character that terminated the number
            }
        }
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
            case '\f':
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                LOG(("BON\n"));
                if (!STACK_SET_INC(sa, sp, STATE_NUMBER)) {
                    LOG(("OVERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
                number = c - '0';
                break;
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
            case '"':
                LOG(("BODQ\n"));
                if (!STACK_SET_INC(sa, sp, STATE_STRING_DOUBLE)) {
                    LOG(("OVERFLOW\n"));
                    ok = 0;
                    done = 1;
                }
                break;
            case '\'':
                LOG(("BOSQ\n"));
                if (!STACK_SET_INC(sa, sp, STATE_STRING_SINGLE)) {
                    LOG(("OVERFLOW\n"));
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
    printf("max_depth: %d\n", max_depth);
    return ok && sp == 0 && STACK_GET(sa, sp) == STATE_SPACE;
}

void process(const char* name, FILE* fp) {
    int v = valid(fp);
    printf("%-3.3s %s\n", v ? "OK" : "BAD", name);
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        process("<STDIN>", stdin);
    } else {
        for (int j = 1; j < argc; ++j) {
            FILE* fp = fopen(argv[j], "r");
            if (!fp) {
                fprintf(stderr, "Could not open file '%s'\n", argv[j]);
                continue;
            }
            process(argv[j], fp);
            fclose(fp);
        }
    }
    return 0;
}

#include <ctype.h>
#include <stdio.h>

#define LOG(x) printf x

enum State {
    STATE_INIT,
    STATE_STRING,
    STATE_ARRAY_ELEM,
    STATE_HASH_KEY,
    STATE_HASH_VALUE,
};

int valid(FILE* fp) {
    int ok = 1;
    int done = 0;
    int sa[1024];
    int sp = 0;
    sa[sp] = STATE_INIT;
    while (!done) {
        int c = getc(fp);
        if (c == EOF) {
            LOG(("EOF\n"));
            done = 1;
            continue;
        }
        if (sa[sp] == STATE_STRING) {
            if (c == '"') {
                LOG(("EOS\n"));
                --sp;
            }
            continue;
        }
        if (isspace(c)) {
            continue;
        }
        switch (c) {
            case '[':
                LOG(("BOA\n"));
                sa[++sp] = STATE_ARRAY_ELEM;
                break;
            case ']':
                LOG(("EOA\n"));
                --sp;
                break;
            case '{':
                LOG(("BOH\n"));
                sa[++sp] = STATE_HASH_KEY;
                break;
            case '}':
                LOG(("EOH\n"));
                --sp;
                break;
            case '"':
                LOG(("BOS\n"));
                sa[++sp] = STATE_STRING;
                break;
            case ',':
                switch (sa[sp]) {
                    case STATE_ARRAY_ELEM:
                        LOG(("AE\n"));
                        break;
                    case STATE_HASH_VALUE:
                        LOG(("HK\n"));
                        sa[sp] = STATE_HASH_KEY;
                        break;
                    default:
                        /* ERROR */
                        ok = 0;
                        done = 1;
                        break;
                }
                break;
            case ':':
                switch (sa[sp]) {
                    case STATE_HASH_KEY:
                        LOG(("HV\n"));
                        sa[sp] = STATE_HASH_VALUE;
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
    return ok && sp == 0 && sa[sp] == STATE_INIT;
}

void process(const char* name, FILE* fp) {
    int v = valid(fp);
    printf("File '%s' %s valid\n", name, v ? "IS" : "IS NOT");
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

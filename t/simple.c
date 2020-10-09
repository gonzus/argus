#include <string.h>
#include <tap.h>
#include "argus.h"

#define ALEN(a) (int) (sizeof(a) / sizeof(a[0]))

static void test_scalar(void) {
    static struct {
        const char* str;
        int valid;
    } data[] = {
        { ""            , 1 },
        { "1"           , 1 },
        { "+1"          , 1 },
        { "-1"          , 1 },
        { ".14"         , 1 },
        { "+.14"        , 1 },
        { "-.14"        , 1 },
        { "3.14"        , 1 },
        { "+3.14"       , 1 },
        { "-3.14"       , 1 },
        { "'a'"         , 1 },
        { "\"b\""       , 1 },
        { "true"        , 1 },
        { "false"       , 1 },
        { "null"        , 1 },

        { ","           , 0 },
        { "++1"         , 0 },
        { "+-1"         , 0 },
        { "-+1"         , 0 },
        { "--1"         , 0 },
        { ".+1"         , 0 },
        { ".-1"         , 0 },
        { "..1"         , 0 },
        { "1,"          , 0 },
        { "1,2"         , 0 },
        { "'"           , 0 },
        { "\""          , 0 },
    };
    Argus* argus = argus_create();
    for (int j = 0; j < ALEN(data); ++j) {
        const char* str = data[j].str;
        int expected = data[j].valid;
        int len = strlen(str);
        argus_clear(argus);
        int valid = argus_parse_buffer(argus, str, len);
        cmp_ok(valid, "==", expected, "%s JSON scalar: <%.*s>",
               expected ? "valid" : "invalid", len, str);
    }
    argus_destroy(argus);
}

static void test_array(void) {
    static struct {
        const char* str;
        int valid;
    } data[] = {
        { "[]"          , 1 },
        { "[1]"         , 1 },
        { "[1,]"        , 1 },
        { "[1,2]"       , 1 },
        { "[1,2,]"      , 1 },
        { "[1,2,3]"     , 1 },
        { "[1,2,3,]"    , 1 },
        { "['a']"       , 1 },
        { "['a',]"      , 1 },
        { "['a',2]"     , 1 },
        { "['a',2,]"    , 1 },
        { "[\"b\"]"     , 1 },
        { "[\"b\",]"    , 1 },
        { "[\"b\",2]"   , 1 },
        { "[\"b\",2,]"  , 1 },

        { "[:]"         , 0 },
        { "[,,]"        , 0 },
        { "[],1"        , 0 },
        { "[],'a'"      , 0 },
        { "[],[]"       , 0 },
        { "[],{}"       , 0 },
    };
    Argus* argus = argus_create();
    for (int j = 0; j < ALEN(data); ++j) {
        const char* str = data[j].str;
        int expected = data[j].valid;
        int len = strlen(str);
        argus_clear(argus);
        int valid = argus_parse_buffer(argus, str, len);
        cmp_ok(valid, "==", expected, "%s JSON array: <%.*s>",
               expected ? "valid" : "invalid", len, str);
    }
    argus_destroy(argus);
}

static void test_hash(void) {
    static struct {
        const char* str;
        int valid;
    } data[] = {
        { "{}"          , 1 },
        { "{1:2}"       , 1 },
        { "{1:2,}"      , 1 },
        { "{1:'x'}"     , 1 },
        { "{1:'x',}"    , 1 },
        { "{1:\"y\"}"   , 1 },
        { "{1:\"y\",}"  , 1 },
        { "{'a':2}"     , 1 },
        { "{'a':2,}"    , 1 },
        { "{\"b\":2}"   , 1 },
        { "{\"b\":2,}"  , 1 },
        { "{1:2, 3:4}"  , 1 },
        { "{1:2, 3:4,}" , 1 },

        { "{:}"         , 0 },
        { "{: 44}"      , 0 },
        { "{,}"         , 0 },
        { "{},1"        , 0 },
        { "{},'a'"      , 0 },
        { "{},[]"       , 0 },
        { "{},{}"       , 0 },
    };
    Argus* argus = argus_create();
    for (int j = 0; j < ALEN(data); ++j) {
        const char* str = data[j].str;
        int expected = data[j].valid;
        int len = strlen(str);
        int valid = argus_parse_buffer(argus, str, len);
        cmp_ok(valid, "==", expected, "%s JSON hash: <%.*s>",
               expected ? "valid" : "invalid", len, str);
    }
    argus_destroy(argus);
}

int main (int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    test_scalar();
    test_array();
    test_hash();

    done_testing();
}

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <tap.h>
#include <log.h>
#include "json.h"

#define ALEN(a) (int) (sizeof(a) / sizeof(a[0]))

static void test_files(const char* dir, int expected) {
    char dir_name[1024];
    sprintf(dir_name, "t/data/%s", dir);
    JSON* json = 0;
    DIR* dirp = 0;

    do {
        json = json_create();
        if (!json) {
            LOG_WARNING("Could not create JSON parser");
            break;
        }

        dirp = opendir(dir_name);
        if (!dirp) {
            LOG_WARNING("Could not open directory [%s]", dir_name);
            break;
        }

        int done = 0;
        while (!done) {
            errno = 0;
            struct dirent* entry = readdir(dirp);
            if (!entry) {
                if (errno) {
                    LOG_WARNING("Could not read directory [%s]", dir_name);
                }
                break;
            }
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            int l = strlen(entry->d_name);
            if (l < 5 || memcmp(entry->d_name + l - 5, ".json", 5) != 0) {
                LOG_INFO("Skipping file [%s], wrong extension", entry->d_name);
                continue;
            }

            LOG_INFO("File [%s]", entry->d_name);
            char file_name[2048];
            sprintf(file_name, "%s/%s", dir_name, entry->d_name);
            int valid = json_validate_file(json, file_name);
            cmp_ok(valid, "==", expected, "%s JSON file <%s>",
                   expected ? "valid" : "invalid", file_name);
        }
    } while (0);

    if (dirp) {
        int ret = closedir(dirp);
        if (ret) {
            LOG_WARNING("Could not close directory [%s]", dir_name);
        }
        dirp = 0;
    }

    if (json) {
        json_destroy(json);
        json = 0;
    }
}

static void test_good(void) {
    test_files("good", 1);
}

static void test_bad(void) {
    test_files("bad", 0);
}

int main (int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    test_good();
    test_bad();

    done_testing();
}

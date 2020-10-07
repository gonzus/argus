#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <log.h>
#include "json.h"

static int validate_buffer(JSON* json, const char* ptr, int len) {
    json_clear(json);
    return json_validate(json, ptr, len);
}

static int validate_file(JSON* json, const char* name) {
    int valid = 0;
    int fd = -1;
    unsigned char* data = 0;
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

        data = (unsigned char*) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            LOG_ERROR("Cannot mmap [%s]", name);
            data = 0;
            break;
        }

        LOG_INFO("Mapped file [%s] with %u bytes at %p", name, size, data);
        valid = validate_buffer(json, data, size);
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

int main(int argc, char* argv[]) {
    JSON* json = json_create();
    for (int j = 1; j < argc; ++j) {
        const char* name = argv[j];
        int valid = validate_file(json, name);
        printf("%-3.3s %s\n", valid ? "OK" : "BAD", name);
    }
    json_destroy(json);
    return 0;
}

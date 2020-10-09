#ifndef ARGUS_H_
#define ARGUS_H_

#include <buffer.h>
#include "stack.h"

typedef struct Argus {
    Stack* stack;
    Buffer* string;
} Argus;

Argus* argus_create(void);
void argus_destroy(Argus* argus);
void argus_clear(Argus* argus);

int argus_parse_buffer(Argus* argus, const char* ptr, int len);
int argus_parse_file(Argus* argus, const char* name);

#endif

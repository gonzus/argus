#ifndef JSON_H_
#define JSON_H_

#include <buffer.h>
#include "stack.h"

typedef struct JSON {
    Stack* stack;
    Buffer* string;
} JSON;

JSON* json_create(void);
void json_destroy(JSON* json);
void json_clear(JSON* json);

int json_validate_buffer(JSON* json, const char* ptr, int len);
int json_validate_file(JSON* json, const char* name);

#endif

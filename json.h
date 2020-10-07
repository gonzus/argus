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

int json_validate(JSON* json, const char* ptr, int len);

#endif

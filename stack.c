#include <stdlib.h>
#include <string.h>
#include "stack.h"

// TODO: change this back to 1024; we use 2 to test the automatic stack growth
#define STACK_DEFAULT_CAPACITY 2
// #define STACK_DEFAULT_CAPACITY 1024

Stack* stack_create(void) {
    return stack_create_capacity(0);
}

Stack* stack_create_capacity(int capacity) {
    Stack* s = (Stack*) malloc(sizeof(Stack));
    memset(s, 0, sizeof(Stack));
    s->cap = capacity ? capacity : STACK_DEFAULT_CAPACITY;
    s->dat = (char*) calloc(s->cap, 1);
    return s;
}

void stack_destroy(Stack* s) {
    free((void*) s->dat);
    s->dat = 0;
    free((void*) s);
    s = 0;
}

void stack_clear(Stack* s) {
    s->pos = 0;
}

int stack_size(Stack* s) {
    return s->pos;
}

int stack_empty(Stack* s) {
    return s->pos == 0;
}

int stack_top(Stack* s, char* v) {
    if (s->pos == 0) {
        *v = 0;
        return STACK_EMPTY;
    }
    *v = s->dat[s->pos - 1];
    return STACK_OK;
}

int stack_pop(Stack* s, char* v) {
    if (s->pos == 0) {
        *v = 0;
        return STACK_EMPTY;
    }
    *v = s->dat[--s->pos];
    return STACK_OK;
}

int stack_push(Stack* s, char v) {
    if (s->pos >= s->cap) {
        int cap = s->cap ? 2*s->cap : STACK_DEFAULT_CAPACITY;
        char* dat = realloc(s->dat, cap);
        s->dat = dat;
        s->cap = cap;
    }
    s->dat[s->pos++] = v;
    return STACK_OK;
}

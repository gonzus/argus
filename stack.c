#include <stdlib.h>
#include <string.h>
#include <log.h>
#include "stack.h"

// #define STACK_DEFAULT_CAPACITY 1024
#define STACK_DEFAULT_CAPACITY 2

Stack* stack_create(void) {
    return stack_create_capacity(0);
}

Stack* stack_create_capacity(int capacity) {
    Stack* s = (Stack*) malloc(sizeof(Stack));
    memset(s, 0, sizeof(Stack));
    s->cap = capacity ? capacity : STACK_DEFAULT_CAPACITY;
    s->dat = (int*) calloc(s->cap, sizeof(int));
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

int stack_top(Stack* s, int* v) {
    if (s->pos == 0) {
        LOG_WARNING("stack %p is empty, cannot get top", s);
        return STACK_EMPTY;
    }
    *v = s->dat[s->pos - 1];
    return STACK_OK;
}

int stack_pop(Stack* s, int* v) {
    if (s->pos == 0) {
        LOG_WARNING("stack %p is empty, cannot pop", s);
        return STACK_EMPTY;
    }
    *v = s->dat[--s->pos];
    return STACK_OK;
}

int stack_push(Stack* s, int v) {
    if (s->pos >= s->cap) {
        int cap = s->cap ? 2*s->cap : STACK_DEFAULT_CAPACITY;
        int* dat = realloc(s->dat, cap * sizeof(int));
        LOG_DEBUG("STACK %p %d -> %p  %d", s->dat, s->cap, dat, cap);
        s->dat = dat;
        s->cap = cap;
    }
    s->dat[s->pos++] = v;
    return STACK_OK;
}

int stack_set(Stack* s, int v) {
    if (s->pos == 0) {
        LOG_WARNING("stack %p is empty, cannot set", s);
        return STACK_EMPTY;
    }
    s->dat[s->pos -1] = v;
    return STACK_OK;
}

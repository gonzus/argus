#ifndef STACK_H_
#define STACK_H_

enum Stack_Error {
    STACK_OK,
    STACK_EMPTY,
};

typedef struct Stack {
    char* dat;
    int cap;
    int pos;
} Stack;

Stack* stack_create(void);
Stack* stack_create_capacity(int capacity);
void stack_destroy(Stack* s);
void stack_clear(Stack* s);

int stack_size(Stack* s);
int stack_empty(Stack* s);

int stack_top(Stack* s, char* v);
int stack_pop(Stack* s, char* v);
int stack_push(Stack* s, char v);

#endif

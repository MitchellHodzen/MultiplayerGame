#ifndef STACK_DEF
#define STACK_DEF
#include <stdlib.h>

struct Stack
{
    unsigned int size;
    unsigned int max_size;
    size_t element_size;
};

size_t Stack_Calculate_Required_Memory(size_t element_size, unsigned int max_stack_size);
void Stack_Init(struct Stack* stack, size_t element_size, unsigned int max_stack_size);
void Stack_Clear(struct Stack* stack);
void* Stack_Push(struct Stack* stack);
void* Stack_Pop(struct Stack* stack);
void* Stack_Peek(const struct Stack* stack);

#endif /* STACK_DEF */
#ifndef RING_STACK_DEF
#define RING_STACK_DEF
#include <stdlib.h>

struct Ring_Stack
{
    unsigned int buffer_size;
    unsigned int buffer_max_size;
    size_t element_size;
    unsigned int _write_index;
};

size_t Ring_Stack_Calculate_Required_Memory(size_t element_size, unsigned int max_buffer_size);
void Ring_Stack_Init(struct Ring_Stack* stack, size_t element_size, unsigned int max_buffer_size);
void Ring_Stack_Clear(struct Ring_Stack* stack);
void* Ring_Stack_Push(struct Ring_Stack* stack);
void* Ring_Stack_Pop(struct Ring_Stack* stack);
void* Ring_Stack_Peek(const struct Ring_Stack* stack);
void* Ring_Stack_Peek_Back(const struct Ring_Stack* stack);

#endif /* RING_STACK_DEF */
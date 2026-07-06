#include "ring_stack.h"

size_t Ring_Stack_Calculate_Required_Memory(size_t element_size, unsigned int max_buffer_size)
{
    // Total stack size will be the size of the ring stack header + the max amount of elements in the stack
    return sizeof(struct Ring_Stack) + (element_size * max_buffer_size);
}

void Ring_Stack_Init(struct Ring_Stack* stack, size_t element_size, unsigned int max_stack_size)
{    
    stack->buffer_max_size = max_stack_size;
    stack->buffer_size = 0;
    stack->_write_index = 0;
    stack->element_size = element_size;
}

void* Ring_Stack_Get_Buffer_Start(struct Ring_Stack* stack)
{
    // The stack starts at the end of the header
    return (char*)stack + sizeof(struct Ring_Stack);
}

void* Ring_Stack_Get_Pointer_At(struct Ring_Stack* stack, unsigned int index)
{
    return ((char*)Ring_Stack_Get_Buffer_Start(stack)) + (index * stack->element_size);
}

void* Ring_Stack_Push(struct Ring_Stack* stack)
{
    // Get a pointer to the next writable space
    void* retval = Ring_Stack_Get_Pointer_At(stack, stack->_write_index);
    
    // Increment the write index
    stack->_write_index++;
    // If the write index overflows, reset it to 0
    stack->_write_index = stack->_write_index * (stack->_write_index != stack->buffer_max_size);
    
    // Increment the buffer count if it isnt full
    stack->buffer_size = stack->buffer_size + (1 * (stack->buffer_size != stack->buffer_max_size));

    return retval;
}

void* Ring_Stack_Peek_Unsafe(const struct Ring_Stack* stack)
{
    // write index is always one ahead of the last value written, so the last value written is at write index - 1. Offset by max buffer size so won't go negative
    unsigned int index = stack->buffer_max_size - 1 + stack->_write_index;

    // Don't let index overflow
    index -= stack->buffer_max_size * (index >= stack->buffer_max_size);
    return Ring_Stack_Get_Pointer_At(stack, index);
}

void* Ring_Stack_Peek(const struct Ring_Stack* stack)
{
    if (stack->buffer_size == 0)
    {
        return NULL;
    }
    
    return Ring_Stack_Peek_Unsafe(stack);
}

void* Ring_Stack_Pop(struct Ring_Stack* stack)
{
    if (stack->buffer_size == 0)
    {
        return NULL;
    }

    void* retval = Ring_Stack_Peek_Unsafe(stack);

    // popping, so decrement the count. If here we know it is greater than 0
    stack->buffer_size--;

    // decrement the write index and overflow
    unsigned int _write_index = stack->buffer_max_size - 1 + stack->_write_index;
    _write_index -= stack->buffer_max_size * (_write_index >= stack->buffer_max_size);

    return retval;
}
#include "stack.h"

size_t Stack_Calculate_Required_Memory(size_t element_size, unsigned int max_stack_size)
{
    // Total stack size will be the size of the stack header + the max amount of elements in the stack
    return sizeof(struct Stack) + (element_size * max_stack_size);
}

void Stack_Init(struct Stack* stack, size_t element_size, unsigned int max_stack_size)
{    
    stack->max_size = max_stack_size;
    stack->element_size = element_size;
    Stack_Clear(stack);
}

void Stack_Clear(struct Stack* stack)
{
    stack->size = 0;
}

static void* Get_Pointer_At(const struct Stack* stack, unsigned int index)
{
    return (char*)stack + sizeof(struct Stack) + (index * stack->element_size);
}

void* Stack_Push(struct Stack* stack)
{
    if (stack->size >= stack->max_size)
    {
        return NULL;
    }

    return Get_Pointer_At(stack, stack->size++);
}

void* Stack_Peek(const struct Stack* stack)
{
    if (stack->size == 0)
    {
        return NULL;
    }
    
    return Get_Pointer_At(stack, stack->size - 1);
}

void* Stack_Pop(struct Stack* stack)
{
    if (stack->size == 0)
    {
        return NULL;
    }

    return Get_Pointer_At(stack, --stack->size);
}
#include "intstack.h"
#include <stdlib.h> 

size_t IntStack_Calculate_Required_Memory(unsigned int size)
{
    return sizeof(struct IntStack) + (size * sizeof(int));
}

void IntStack_Init(struct IntStack* intStack, unsigned int size)
{
    intStack->maxSize = size;
    intStack->length = 0;
}

int* IntStack_Data(const struct IntStack* intStack)
{
    // The stack starts at the end of the header
    return (int*)((char*)intStack + sizeof(struct IntStack));
}

bool IntStack_Push(struct IntStack* intStack, int value)
{
    if (intStack->length >= intStack->maxSize)
    {
        // Stack is full, fail
        return false;
    }

    // Here we know that the current length is strictly less than max size
    IntStack_Data(intStack)[intStack->length] = value; // current pointer is at length - 1, so the next value pointer is length
    intStack->length++;
    return true;
}

bool IntStack_Pop(struct IntStack* intStack, int* value)
{
    if (intStack->length == 0)
    {
        // If there are no elements, fail
        return false;
    }

    intStack->length--; // Length is pointer + 1, so length -1 will be the current pointer. Since we're popping, we are getting the current pointer and decrementing at once
    *value = IntStack_Data(intStack)[intStack->length];
    return true;
}

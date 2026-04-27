#include "intstack.h"
#include <stdlib.h> 

bool IntStack_Init(IntStack* intStack, unsigned int size)
{
    intStack->maxSize = size;
    intStack->_data = (int*) malloc(size * sizeof(int));
    if (intStack->_data == NULL)
    {
        return false;
    }

    intStack->length = 0;
    return true;
}

bool IntStack_Push(IntStack* intStack, int value)
{
    if (intStack->length >= intStack->maxSize)
    {
        // Stack is full, fail
        return false;
    }

    // Here we know that the current length is strictly less than max size
    intStack->_data[intStack->length] = value; // current pointer is at length - 1, so the next value pointer is length
    intStack->length++;
    return true;
}

bool IntStack_Pop(IntStack* intStack, int* value)
{
    if (intStack->length == 0)
    {
        // If there are no elements, fail
        return false;
    }

    intStack->length--; // Length is pointer + 1, so length -1 will be the current pointer. Since we're popping, we are getting the current pointer and decrementing at once
    *value = intStack->_data[intStack->length];
    return true;
}

void IntStack_Free(IntStack* intStack)
{
    free(intStack->_data);
    free(intStack);
    intStack = NULL;
}

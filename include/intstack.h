#ifndef INTSTACK
#define INTSTACK
#include <stdbool.h> 

typedef struct IntStack
{
    unsigned int length;
    unsigned int maxSize;
    int* _data;
} IntStack;

bool IntStack_Init(IntStack* intStack, unsigned int size);
bool IntStack_Push(IntStack* intStack, int value);
bool IntStack_Pop(IntStack* intStack, int* value);
void IntStack_Free(IntStack* intStack);
#endif /* INTSTACK */
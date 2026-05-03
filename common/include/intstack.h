#ifndef INTSTACK
#define INTSTACK
#include <stdbool.h> 

struct IntStack
{
    unsigned int length;
    unsigned int maxSize;
    int* _data;
};

bool IntStack_Init(struct IntStack* intStack, unsigned int size);
bool IntStack_Push(struct IntStack* intStack, int value);
bool IntStack_Pop(struct IntStack* intStack, int* value);
void IntStack_Free(struct IntStack* intStack);
#endif /* INTSTACK */
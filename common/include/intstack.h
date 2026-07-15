#ifndef INTSTACK
#define INTSTACK
#include <stdbool.h> 
#include <stddef.h> 

struct IntStack
{
    unsigned int length;
    unsigned int maxSize;
};

size_t IntStack_Calculate_Required_Memory(unsigned int size);
int* IntStack_Data(const struct IntStack* intStack);
void IntStack_Init(struct IntStack* intStack, unsigned int size);
bool IntStack_Push(struct IntStack* intStack, int value);
bool IntStack_Pop(struct IntStack* intStack, int* value);
#endif /* INTSTACK */
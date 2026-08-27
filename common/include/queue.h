#ifndef QUEUE_DEF
#define QUEUE_DEF
#include <stdlib.h>

struct Queue
{
    unsigned int size;
    unsigned int max_size;
    size_t element_size;
};

size_t Queue_Calculate_Required_Memory(size_t element_size, unsigned int max_queue_size);
void Queue_Init(struct Queue* queue, size_t element_size, unsigned int max_queue_size);
void Queue_Clear(struct Queue* queue);
void* Queue_Push(struct Queue* queue);
void* Queue_Pop(struct Queue* queue);
void* Queue_Peek(const struct Queue* queue);

#endif /* QUEUE_DEF */
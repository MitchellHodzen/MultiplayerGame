#include "queue.h"

size_t Queue_Calculate_Required_Memory(size_t element_size, unsigned int max_queue_size)
{
    // Total buffer size will be the size of the queue header + the max amount of elements in the buffer
    return sizeof(struct Queue) + (element_size * max_queue_size);
}

void Queue_Init(struct Queue* queue, size_t element_size, unsigned int max_queue_size)
{    
    queue->max_size = max_queue_size;
    queue->element_size = element_size;
    Queue_Clear(queue);
}

void Queue_Clear(struct Queue* queue)
{
    queue->size = 0;
}

static void* Get_Pointer_At(const struct Queue* queue, unsigned int index)
{
    return (char*)queue + sizeof(struct Queue) + (index * queue->element_size);
}

void* Queue_Push(struct Queue* queue)
{
    if (queue->size >= queue->max_size)
    {
        return NULL;
    }

    return Get_Pointer_At(queue, queue->size++);
}

void* Queue_Peek(const struct Queue* queue)
{
    if (queue->size == 0)
    {
        return NULL;
    }
    
    return Get_Pointer_At(queue, queue->size - 1);
}

void* Queue_Pop(struct Queue* queue)
{
    if (queue->size == 0)
    {
        return NULL;
    }

    return Get_Pointer_At(queue, --queue->size);
}
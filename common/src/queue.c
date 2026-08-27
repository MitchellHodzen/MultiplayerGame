#include "queue.h"

size_t Queue_Calculate_Required_Memory(size_t element_size, unsigned int max_queue_size)
{
    return Ring_Buffer_Calculate_Required_Memory(element_size, max_queue_size);
}

void Queue_Init(struct Queue* queue, size_t element_size, unsigned int max_queue_size)
{
    Ring_Buffer_Init(&queue->_buffer, element_size, max_queue_size);
}

void* Queue_Push(struct Queue* queue)
{
    // Don't overwrite values in the queue
    if (queue->_buffer.buffer_size >= queue->_buffer.buffer_max_size)
    {
        return NULL;
    }

    return Ring_Buffer_Get_Next(&queue->_buffer);
}

void* Queue_Peek(const struct Queue* queue)
{
    return Ring_Buffer_Get_At(&queue->_buffer, 0);
}

void* Queue_Pop(struct Queue* queue)
{
    return Ring_Buffer_Dequeue(&queue->_buffer);
}
#ifndef RING_BUFFER_DEF
#define RING_BUFFER_DEF
#include <stddef.h>

struct Ring_Buffer
{
    unsigned int buffer_size;
    unsigned int buffer_max_size;
    size_t element_size;
    unsigned int _write_index;
};

size_t Ring_Buffer_Calculate_Required_Memory(size_t element_size, unsigned int max_buffer_size);
void Ring_Buffer_Init(struct Ring_Buffer* buffer, size_t element_size, unsigned int max_buffer_size);
void* Ring_Buffer_Get_Next(struct Ring_Buffer* buffer);
void* Ring_Buffer_Get_At(const struct Ring_Buffer* buffer, unsigned int index);
void* Ring_Buffer_Pop(struct Ring_Buffer* buffer);
void* Ring_Buffer_Dequeue(struct Ring_Buffer* buffer);
#endif /* RING_BUFFER_DEF */
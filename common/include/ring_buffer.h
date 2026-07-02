#ifndef RING_BUFFER_DEF
#define RING_BUFFER_DEF
#include <stdlib.h>
#include <stdbool.h>

struct Ring_Buffer
{
    unsigned int buffer_size;
    unsigned int buffer_max_size;
    size_t element_size;
    unsigned int _write_index;
};

bool Ring_Buffer_Init(struct Ring_Buffer** buffer, size_t element_size, unsigned int max_buffer_size);
void* Ring_Buffer_Get_Next(struct Ring_Buffer* buffer);
void* Ring_Buffer_Get_At(const struct Ring_Buffer* buffer, unsigned int index);

#endif /* RING_BUFFER_DEF */
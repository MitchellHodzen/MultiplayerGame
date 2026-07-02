#include "ring_buffer.h"

bool Ring_Buffer_Init(struct Ring_Buffer** buffer, size_t element_size, unsigned int max_buffer_size)
{
    // Total buffer size will be the size of the ring buffer header + the max amount of elements in the buffer
    unsigned int buffer_total_size = sizeof(struct Ring_Buffer) + (element_size * max_buffer_size);
    *buffer = (struct Ring_Buffer*) calloc(1, buffer_total_size);
    if (*buffer == NULL)
    {
        // Couldnt instantiate ring buffer
        return false;
    }
    
    (*buffer)->buffer_max_size = max_buffer_size;
    (*buffer)->buffer_size = 0;
    (*buffer)->_write_index = 0;
    (*buffer)->element_size = element_size;
    
    return true;
}

inline void* Get_Buffer_Start(struct Ring_Buffer* buffer)
{
    // The buffer starts at the end of the header
    return (char*)buffer + sizeof(struct Ring_Buffer);
}

void* Get_Pointer_At(struct Ring_Buffer* buffer, unsigned int index)
{
    return ((char*)Get_Buffer_Start(buffer)) + (index * buffer->element_size);
}

void* Ring_Buffer_Get_Next(struct Ring_Buffer* buffer)
{
    // Get a pointer to the next writable space
    void* retval = Get_Pointer_At(buffer, buffer->_write_index);
    
    // Increment the write index
    buffer->_write_index++;
    // If the write index overflows, reset it to 0
    buffer->_write_index = buffer->_write_index * (buffer->_write_index != buffer->buffer_max_size);
    
    // Increment the buffer count if it isnt full
    buffer->buffer_size = buffer->buffer_size + (1 * (buffer->buffer_size != buffer->buffer_max_size));

    return retval;
}

void* Ring_Buffer_Get_At(const struct Ring_Buffer* buffer, unsigned int index)
{
    // write index is always one ahead of start, so start is write index - 1. Offset by max buffer size so won't go negative in next step
    unsigned int start_index = buffer->buffer_max_size - 1 + buffer->_write_index;

    // Offset the index based on the start index
    index = start_index - index; 

    // Don't let index overflow
    index -= buffer->buffer_max_size * (index >= buffer->buffer_max_size);
    
    return Get_Pointer_At(buffer, index);
}
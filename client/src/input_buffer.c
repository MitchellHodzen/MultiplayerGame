#include "input_buffer.h"
#include <stdlib.h>
#include <string.h>


void Input_Buffer_Init(struct Input_Snapshot_Buffer* buffer)
{
    buffer->buffer_size = 0;
    buffer->_write_index = 0;
    memset(buffer->_buffer, 0, INPUT_SNAPSHOT_BUF_SIZE * sizeof(struct Input_Snapshot));
}

struct Input_Snapshot Input_Buffer_Get_At(struct Input_Snapshot_Buffer* buffer, unsigned int index)
{
    // write index is always one ahead of start, so start is write index - 1. Offset by max buffer size so won't go negative in next step
    unsigned int start_index = INPUT_SNAPSHOT_BUF_SIZE - 1 + buffer->_write_index;

    // Offset the index based on the start index
    index = start_index - index; 

    // Don't let index overflow
    index -= INPUT_SNAPSHOT_BUF_SIZE * (index >= INPUT_SNAPSHOT_BUF_SIZE);
    
    return buffer->_buffer[index];
}

void Input_Buffer_Put(struct Input_Snapshot_Buffer* buffer, struct Input_Snapshot snapshot)
{
    // write to the current write index
    buffer->_buffer[buffer->_write_index] = snapshot;
    
    // always increase write index
    buffer->_write_index++;
    // If the write index overflows, reset it to 0
    buffer->_write_index = buffer->_write_index * (buffer->_write_index != INPUT_SNAPSHOT_BUF_SIZE);
    
    // Increment the buffer count if it isnt full
    buffer->buffer_size = buffer->buffer_size + (1 * (buffer->buffer_size != INPUT_SNAPSHOT_BUF_SIZE));
}
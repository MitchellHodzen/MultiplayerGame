#include "chat_buffers.h"
#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h>

void Chat_Reset_Input_Buffer(struct Chat_Buffers* chat_buffers)
{
    chat_buffers->_input_cursor = 0;
    chat_buffers->chat_input_buffer[0] = '\0';
}

bool Chat_Initialize(struct Chat_Buffers** chat_buffers, unsigned int max_chat_size, unsigned int max_history_size)
{
    *chat_buffers = (struct Chat_Buffers*) malloc(sizeof(struct Chat_Buffers));
    if (*chat_buffers == NULL)
    {
        // Couldnt instantiate chat buffers
        return false;
    }

    (*chat_buffers)->max_chat_size = max_chat_size;
    (*chat_buffers)->chat_input_buffer = calloc(max_chat_size + 1, sizeof(char));
    if ((*chat_buffers)->chat_input_buffer == NULL)
    {
        // Couldnt instantiate input buffer
        Chat_Free(chat_buffers);
        return false;
    }

    Chat_Reset_Input_Buffer(*chat_buffers);

    // Set up history buffer
    if (!Ring_Buffer_Init(&((*chat_buffers)->chat_history_buffer), (max_chat_size + 1) * sizeof(char), max_history_size))
    {
        Chat_Free(chat_buffers);
        return false;
    }

    return true;
}

char* Chat_Get_Message_At(struct Chat_Buffers* chat_buffers, unsigned int chat_history_index)
{
    // Chat messages are inverted - the oldest message is at position 0
    chat_history_index =  (chat_buffers->chat_history_buffer->buffer_size - 1) - chat_history_index;
    return (char*)Ring_Buffer_Get_At(chat_buffers->chat_history_buffer, chat_history_index);
}

void Chat_History_Write(struct Chat_Buffers* chat_buffers, char* string, unsigned int strlen)
{
    char* next = Ring_Buffer_Get_Next(chat_buffers->chat_history_buffer);
    // TODO: Use the length to not rely on null termination
    strcpy(next, string);
}

bool Chat_Try_Write_To_Input(struct Chat_Buffers* chat_buffers, char input)
{
    if (chat_buffers->_input_cursor < chat_buffers->max_chat_size)
    {
        // buffer is chat max size + 1, so we can safely operate < chat max size
        chat_buffers->chat_input_buffer[chat_buffers->_input_cursor] = input;
        // always put the null term char after the cursor
        chat_buffers->chat_input_buffer[chat_buffers->_input_cursor + 1] =  '\0';
        chat_buffers->_input_cursor++;

        return true;
    }

    return false;
}

void Chat_Free(struct Chat_Buffers** chat_buffers)
{
    free((*chat_buffers)->chat_input_buffer);
    free((*chat_buffers)->chat_history_buffer);
    free(*chat_buffers);
    *chat_buffers = NULL;
}
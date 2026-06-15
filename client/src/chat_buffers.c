#include "chat_buffers.h"
#include <stdlib.h>

void Chat_Reset_Input_Buffer(struct Chat_Buffers* chatBuffers)
{
    chatBuffers->_input_cursor = 0;
    chatBuffers->chat_input_buffer[0] = '\0';
}

bool Chat_Initialize(struct Chat_Buffers** chatBuffers, unsigned int max_chat_size, unsigned int max_history_size)
{
    *chatBuffers = (struct Chat_Buffers*) malloc(sizeof(struct Chat_Buffers));
    if (*chatBuffers == NULL)
    {
        // Couldnt instantiate chat buffers
        return false;
    }

    (*chatBuffers)->max_chat_size = max_chat_size;
    (*chatBuffers)->chat_input_buffer = calloc(max_chat_size + 1, sizeof(char));
    if ((*chatBuffers)->chat_input_buffer == NULL)
    {
        // Couldnt instantiate input buffer
        Chat_Free(chatBuffers);
        return false;
    }

    Chat_Reset_Input_Buffer(*chatBuffers);

    (*chatBuffers)->max_history_size = max_history_size;
    (*chatBuffers)->_chat_history_buffer = calloc(max_history_size, (max_chat_size + 1) * sizeof(char));
    if ((*chatBuffers)->_chat_history_buffer == NULL)
    {
        // Couldnt instantiate history buffer
        Chat_Free(chatBuffers);
        return false;
    }

    (*chatBuffers)->chat_history_count = 0;

    return true;
}

char* Get_Message_At(struct Chat_Buffers* chatBuffers, unsigned int chat_history_index)
{
    // Every message has a max_chat_size + 1 large buffer
    unsigned int index = chat_history_index * ((chatBuffers->max_chat_size + 1)  * sizeof(char));
    return &(chatBuffers->_chat_history_buffer[index]);
}

bool Chat_Try_Write_To_Input(struct Chat_Buffers* chatBuffers, char input)
{
    if (chatBuffers->_input_cursor < chatBuffers->max_chat_size)
    {
        // buffer is chat max size + 1, so we can safely operate < chat max size
        chatBuffers->chat_input_buffer[chatBuffers->_input_cursor] = input;
        // always put the null term char after the cursor
        chatBuffers->chat_input_buffer[chatBuffers->_input_cursor + 1] =  '\0';
        chatBuffers->_input_cursor++;

        return true;
    }

    return false;
}

void Chat_Free(struct Chat_Buffers** chatBuffers)
{
    free((*chatBuffers)->chat_input_buffer);
    free((*chatBuffers)->_chat_history_buffer);
    free(*chatBuffers);
    *chatBuffers = NULL;
}
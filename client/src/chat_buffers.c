#include "chat_buffers.h"
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

    (*chat_buffers)->max_history_size = max_history_size;
    (*chat_buffers)->_chat_history_buffer = calloc(max_history_size, (max_chat_size + 1) * sizeof(char));
    if ((*chat_buffers)->_chat_history_buffer == NULL)
    {
        // Couldnt instantiate history buffer
        Chat_Free(chat_buffers);
        return false;
    }

    (*chat_buffers)->chat_history_count = 0;
    (*chat_buffers)->_history_start_index = 0;
    (*chat_buffers)->_history_write_index = 0;

    return true;
}

char* Chat_Get_Direct(struct Chat_Buffers* chat_buffers, unsigned int index)
{
    // Every message has a max_chat_size + 1 large buffer
    unsigned int corrected_index = index * ((chat_buffers->max_chat_size + 1)  * sizeof(char));
    return &(chat_buffers->_chat_history_buffer[corrected_index]);
}

char* Chat_Get_Message_At(struct Chat_Buffers* chat_buffers, unsigned int chat_history_index)
{
    // Offset the index by the start index
    chat_history_index = chat_history_index + chat_buffers->_history_start_index;
    // If the index overflows, wrap it
    chat_history_index -= chat_buffers->max_history_size * (chat_history_index >= chat_buffers->max_history_size);

    return Chat_Get_Direct(chat_buffers, chat_history_index);
}

void Chat_History_Write(struct Chat_Buffers* chat_buffers, char* string, unsigned int strlen)
{
    // write to the current write index
    char* history_buffer = Chat_Get_Direct(chat_buffers, chat_buffers->_history_write_index);
    // TODO: Use the length to not rely on null termination
    strcpy(history_buffer, string);

    // If the buffer is full, start incrementing the start index
    chat_buffers->_history_start_index += 1 * (chat_buffers->chat_history_count == chat_buffers->max_history_size);
    // If the start index overflows, reset it to 0
    chat_buffers->_history_start_index = chat_buffers->_history_start_index * (chat_buffers->_history_start_index != chat_buffers->max_history_size);
    
    // always increase write index
    chat_buffers->_history_write_index++;
    // If the write index overflows, reset it to 0
    chat_buffers->_history_write_index = chat_buffers->_history_write_index * (chat_buffers->_history_write_index != chat_buffers->max_history_size);
    
    // Increment the chat history if it isnt full
    chat_buffers->chat_history_count = chat_buffers->chat_history_count + (1 * (chat_buffers->chat_history_count != chat_buffers->max_history_size));
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
    free((*chat_buffers)->_chat_history_buffer);
    free(*chat_buffers);
    *chat_buffers = NULL;
}
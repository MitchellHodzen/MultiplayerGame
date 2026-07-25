#include "chat_buffers.h"
#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h>
#include "utf8_helper.h"

void Chat_Reset_Input_Buffer(struct Chat_Buffers* chat_buffers)
{
    chat_buffers->_input_cursor = 0;
    chat_buffers->chat_input_buffer[0] = '\0';
    chat_buffers->chat_size = 0;
}

bool Chat_Initialize(struct Chat_Buffers** chat_buffers, unsigned int max_chat_size, unsigned int max_history_size)
{
    *chat_buffers = (struct Chat_Buffers*) malloc(sizeof(struct Chat_Buffers));
    if (*chat_buffers == NULL)
    {
        // Couldnt instantiate chat buffers
        return false;
    }

    // Buffer size is the size needed to hold max_chat_size amount of the largest utf8 character + the null terminator
    (*chat_buffers)->_buffer_size = Calculate_UTF8_Max_Size(max_chat_size) + 1;
    (*chat_buffers)->max_chat_size = max_chat_size;
    (*chat_buffers)->chat_input_buffer = calloc((*chat_buffers)->_buffer_size, sizeof(char));
    if ((*chat_buffers)->chat_input_buffer == NULL)
    {
        // Couldnt instantiate input buffer
        Chat_Free(chat_buffers);
        return false;
    }

    Chat_Reset_Input_Buffer(*chat_buffers);

    // Set up history buffer
    (*chat_buffers)->chat_history_buffer = calloc(1, Ring_Buffer_Calculate_Required_Memory((*chat_buffers)->_buffer_size * sizeof(char), max_history_size));
    if ((*chat_buffers)->chat_history_buffer == NULL)
    {
        Chat_Free(chat_buffers);
        return false;
    }

    Ring_Buffer_Init((*chat_buffers)->chat_history_buffer, (*chat_buffers)->_buffer_size * sizeof(char), max_history_size);

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

bool Chat_Try_Write_To_Input(struct Chat_Buffers* chat_buffers, char* text, size_t text_length)
{
    size_t space_left = chat_buffers->_buffer_size - chat_buffers->_input_cursor;
    if (chat_buffers->chat_size < chat_buffers->max_chat_size && space_left >= text_length)
    {
        // buffer is chat max size + 1, so we can safely operate < chat max size
        memcpy(chat_buffers->chat_input_buffer + chat_buffers->_input_cursor, text, text_length);
        chat_buffers->_input_cursor += text_length;
        chat_buffers->chat_size++;
        
        // always put the null term char after the new input
        chat_buffers->chat_input_buffer[chat_buffers->_input_cursor] =  '\0';
        return true;
    }

    return false;
}

void Chat_Remove_Last_Input(struct Chat_Buffers* chat_buffers)
{    
    while(chat_buffers->_input_cursor > 0)
    {
        chat_buffers->_input_cursor--;
        if (UTF8_Is_Start_Char(chat_buffers->chat_input_buffer[chat_buffers->_input_cursor]))
        {
            // If its ASCII (first bit 0) or the first byte of a UTF-8 character (second bit 1), we're done
            chat_buffers->chat_input_buffer[chat_buffers->_input_cursor] =  '\0';
            chat_buffers->chat_size--;
            return;
        }
    }

    Chat_Reset_Input_Buffer(chat_buffers);
}

void Chat_Free(struct Chat_Buffers** chat_buffers)
{
    free((*chat_buffers)->chat_input_buffer);
    free((*chat_buffers)->chat_history_buffer);
    free(*chat_buffers);
    *chat_buffers = NULL;
}
#ifndef CHAT_BUFFERS_DEF
#define CHAT_BUFFERS_DEF
#include <stdbool.h>

struct Ring_Buffer;

struct Chat_Buffers
{
    unsigned int max_chat_size;
    char* chat_input_buffer;
    unsigned int _input_cursor;
    struct Ring_Buffer* chat_history_buffer;
};

bool Chat_Initialize(struct Chat_Buffers** chat_buffers, unsigned int max_chat_size, unsigned int max_history_size);
void Chat_Reset_Input_Buffer(struct Chat_Buffers* chat_buffers);
bool Chat_Try_Write_To_Input(struct Chat_Buffers* chat_buffers, char input);
void Chat_History_Write(struct Chat_Buffers* chat_buffers, char* string, unsigned int strlen);
char* Chat_Get_Message_At(struct Chat_Buffers* chat_buffers, unsigned int chat_history_index);
void Chat_Free(struct Chat_Buffers** chat_buffers);

#endif /* CHAT_BUFFERS_DEF */

#include "input_buffer.h"
#include <stdlib.h>
#include <string.h>


bool Input_Buffer_Init(Input_Snapshot_Buffer** buffer, unsigned int buffer_max_size)
{
    *buffer = calloc(1, Ring_Buffer_Calculate_Required_Memory(sizeof(struct Input_Snapshot), buffer_max_size));
    if ((*buffer) == NULL)
    {
        return false;
    }

    Ring_Buffer_Init(*buffer, sizeof(struct Input_Snapshot), buffer_max_size);

    return true;
}

struct Input_Snapshot Input_Buffer_Get_At(Input_Snapshot_Buffer* buffer, unsigned int index)
{
    return *((struct Input_Snapshot*)Ring_Buffer_Get_At(buffer, index));
}

void Input_Buffer_Put(Input_Snapshot_Buffer* buffer, struct Input_Snapshot snapshot)
{
    struct Input_Snapshot* next = Ring_Buffer_Get_Next(buffer);
    //SDL_Log("Input snapshot saved with chat buffer size of %i", snapshot.chat_messages_cached);
    *next = snapshot;
}

void Input_Snapshot_Init(struct Input_Snapshot* input_snapshot)
{
    memset(input_snapshot, 0, sizeof(struct Input_Snapshot));
}

bool Input_Snapshot_Push_Command(struct Input_Snapshot* snapshot, struct Command_Entry command)
{
    if (snapshot->command_queue.command_cnt >= EVENT_BUF_SIZE)
    {
        return false;
    }

    snapshot->command_queue.command_queue[snapshot->command_queue.command_cnt++] = command;
    return true;
}

unsigned int Input_Snapshot_Save_Keyboard_State(struct Input_Snapshot* snapshot, bool* keyboard_state, size_t keyboard_state_len)
{
    memcpy(snapshot->keyboard_state, keyboard_state, keyboard_state_len);
    // Any keyboard state over and above the max keyboard size would get thrown out
    return keyboard_state_len - MAX_KEYBOARD_SIZE;
}
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
#include "input_buffer.h"
#include <stdlib.h>
#include <string.h>


bool Input_Buffer_Init(Input_Snapshot_Buffer** buffer)
{
    return Ring_Buffer_Init(buffer, sizeof(struct Input_Snapshot), INPUT_SNAPSHOT_BUF_SIZE);
}

struct Input_Snapshot Input_Buffer_Get_At(Input_Snapshot_Buffer* buffer, unsigned int index)
{
    return *((struct Input_Snapshot*)Ring_Buffer_Get_At(buffer, index));
}

void Input_Buffer_Put(Input_Snapshot_Buffer* buffer, struct Input_Snapshot snapshot)
{
    struct Input_Snapshot* next = Ring_Buffer_Get_Next(buffer);
    *next = snapshot;
}
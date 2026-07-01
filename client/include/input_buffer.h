#ifndef INPUT_SNAPSHOT_DEF
#define INPUT_SNAPSHOT_DEF
#include "Vector2.h"
#include "stdbool.h"
#include "ring_buffer.h"

struct Input_Snapshot
{
    long client_time;
    struct Vector2 direction;
};

#define INPUT_SNAPSHOT_BUF_SIZE 10
typedef struct Ring_Buffer Input_Snapshot_Buffer;

bool Input_Buffer_Init(Input_Snapshot_Buffer** buffer);
struct Input_Snapshot Input_Buffer_Get_At(Input_Snapshot_Buffer* buffer, unsigned int index);
void Input_Buffer_Put(Input_Snapshot_Buffer* buffer, struct Input_Snapshot snapshot);

#endif /* INPUT_SNAPSHOT_DEF */
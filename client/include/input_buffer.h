#ifndef INPUT_SNAPSHOT_DEF
#define INPUT_SNAPSHOT_DEF
#include "Vector2.h"

struct Input_Snapshot
{
    long client_time;
    struct Vector2 direction;
};

#define INPUT_SNAPSHOT_BUF_SIZE 10
struct Input_Snapshot_Buffer
{
    unsigned int buffer_size;
    unsigned int _write_index;
    struct Input_Snapshot _buffer[INPUT_SNAPSHOT_BUF_SIZE];
};

struct Input_Snapshot Input_Buffer_Get_At(struct Input_Snapshot_Buffer* buffer, unsigned int index);
void Input_Buffer_Put(struct Input_Snapshot_Buffer* buffer, struct Input_Snapshot snapshot);

#endif /* INPUT_SNAPSHOT_DEF */
#ifndef COMPONENT_TRANSFORM
#define COMPONENT_TRANSFORM
#include "vector2.h"

struct C_Transform
{
    unsigned int parent_id;
    struct Vector2 position;
};

struct N_C_Transform_Snapshot
{
    long server_time;
    struct C_Transform transform;
};

#define NET_TRANS_BUF_SIZE 5
struct N_C_Transform_Interpolation_Buffer
{
    struct N_C_Transform_Snapshot _buffer[NET_TRANS_BUF_SIZE];
};

void Interp_Buf_Add(struct N_C_Transform_Interpolation_Buffer* interp_buf, struct N_C_Transform_Snapshot snapshot);

#endif /* COMPONENT_TRANSFORM */
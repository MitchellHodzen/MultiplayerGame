#ifndef PACKETS
#define PACKETS
#include "vector2.h"

enum Packet_Type
{
    REQUEST_JOIN,
    ADD_SQUARE,
    UNUSED,
};

struct P_Add_Square
{
    enum Packet_Type type;
    unsigned int networkId;
    struct Vector2 position;
};


#endif /* PACKETS */
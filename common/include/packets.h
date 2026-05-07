#ifndef PACKETS
#define PACKETS
#include "vector2.h"

enum Packet_Type
{
    ADD_SQUARE,
    UNUSED,
};

struct P_Add_Square
{
    enum Packet_Type type;
    struct Vector2 position;
};


#endif /* PACKETS */
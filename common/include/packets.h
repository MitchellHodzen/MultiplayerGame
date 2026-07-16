#ifndef PACKETS
#define PACKETS
#include "vector2.h"
#include "component_player_state.h"

enum Packet_Type
{
    SERVER_TIME,
    REQUEST_JOIN,
    JOIN_SERVER,
    ADD_SQUARE,
    INPUT_DIRECTION,
    UPDATE,
    UNUSED,
};

struct P_JOIN_SERVER
{
    enum Packet_Type type;
    unsigned int max_entities;
    unsigned int max_chat_length;
    unsigned int network_id;
    unsigned int ticks_per_s;
    unsigned long server_time_ms;
    unsigned int mocked_latency_ms;
    struct Vector2 position;
};

struct P_Input_Direction
{
    enum Packet_Type type;
    unsigned int networkId;
    struct Vector2 direction;
};

struct P_Update_Header
{
    enum Packet_Type type;
    unsigned int removals_count;
    unsigned int updates_count;
    unsigned long server_time_ms;
};

struct P_Update_Entity_Data
{
    unsigned int networkId;
    struct Vector2 position;
    struct Vector2 velocity;
    enum Player_State state;
};

struct P_Server_Time
{
    enum Packet_Type type;
    unsigned long server_time_ms;
    unsigned int mocked_latency_ms;
};

enum Message_Importance
{
    MESSAGE_IMPORTANCE_STANDARD,
    MESSAGE_IMPORTANCE_ALERT,
    MESSAGE_IMPORTANCE_LOW
};

struct P_Chat_Header
{
    enum Message_Importance messageImportance;
    bool isServerMessage;
    unsigned int networkId;
};


#endif /* PACKETS */
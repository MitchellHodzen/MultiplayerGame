#ifndef INPUT_SNAPSHOT_DEF
#define INPUT_SNAPSHOT_DEF
#include "Vector2.h"
#include "stdbool.h"
#include "ring_buffer.h"

struct Chat_Snapshot_Info
{
    unsigned int entity_id;
    char message[40]; // Hardcoded max chat length. make it variable length and get from server
};

// TODO: replace with event system
struct Input_Snapshot
{
    long client_time;
    struct Vector2 direction;
    bool prediction_toggled_on;
    bool prediction_toggled_off;
    bool interpolation_toggled_on;
    bool interpolation_toggled_off;
    unsigned int chat_messages_cached;
    struct Chat_Snapshot_Info chat_cache[10]; // unlikely to receive multiple chat messages in a single frame, dont cache many
};

typedef struct Ring_Buffer Input_Snapshot_Buffer;

bool Input_Buffer_Init(Input_Snapshot_Buffer** buffer, unsigned int buffer_max_size);
struct Input_Snapshot Input_Buffer_Get_At(Input_Snapshot_Buffer* buffer, unsigned int index);
void Input_Buffer_Put(Input_Snapshot_Buffer* buffer, struct Input_Snapshot snapshot);
void Input_Snapshot_Init(struct Input_Snapshot* input_snapshot);

#endif /* INPUT_SNAPSHOT_DEF */
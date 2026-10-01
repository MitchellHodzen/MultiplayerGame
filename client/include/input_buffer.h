#ifndef INPUT_SNAPSHOT_DEF
#define INPUT_SNAPSHOT_DEF
#include "Vector2.h"
#include "stdbool.h"
#include "ring_buffer.h"
#include "input_command_buffer.h"

struct Chat_Snapshot_Info
{
    unsigned int entity_id;
    char message[40]; // Hardcoded max chat length. make it variable length and get from server
};

struct Input_Snapshot
{
    long client_time;
    unsigned int sim_tick;
    unsigned int chat_messages_cached;
    struct Chat_Snapshot_Info chat_cache[10]; // unlikely to receive multiple chat messages in a single frame, dont cache many
    struct Command_Buffer command_queue;
    bool cmnd_states[CMND_MAX_CNT];
};

typedef struct Ring_Buffer Input_Snapshot_Buffer;

bool Input_Buffer_Init(Input_Snapshot_Buffer** buffer, unsigned int buffer_max_size);
struct Input_Snapshot Input_Buffer_Get_At(Input_Snapshot_Buffer* buffer, unsigned int index);
void Input_Buffer_Put(Input_Snapshot_Buffer* buffer, struct Input_Snapshot snapshot);
void Input_Snapshot_Init(struct Input_Snapshot* input_snapshot);
bool Input_Snapshot_Push_Command(struct Input_Snapshot* snapshot, struct Command_Entry command);
unsigned int Input_Snapshot_Save_Command_State(struct Input_Snapshot* snapshot, bool* cmnd_states, size_t cmnd_state_len);

#endif /* INPUT_SNAPSHOT_DEF */
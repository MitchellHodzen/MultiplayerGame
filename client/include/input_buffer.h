#ifndef INPUT_SNAPSHOT_DEF
#define INPUT_SNAPSHOT_DEF
#include "Vector2.h"
#include "stdbool.h"
#include "ring_buffer.h"

#define EVENT_BUF_SIZE 100
#define MAX_KEYBOARD_SIZE 200

enum COMMAND
{
    UNDETERMINED,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    ATTACK,
    TOGGLE_INTERPOLATION,
    TOGGLE_PREDICTION,
    START_CHAT
};

struct Command_Entry
{
    enum COMMAND command;
    bool pressed; // if not pressed then released
};

struct Chat_Snapshot_Info
{
    unsigned int entity_id;
    char message[40]; // Hardcoded max chat length. make it variable length and get from server
};

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
    struct Command_Entry command_queue[EVENT_BUF_SIZE];
    unsigned int command_cnt;
    bool* keyboard_state[MAX_KEYBOARD_SIZE];
};

typedef struct Ring_Buffer Input_Snapshot_Buffer;

bool Input_Buffer_Init(Input_Snapshot_Buffer** buffer, unsigned int buffer_max_size);
struct Input_Snapshot Input_Buffer_Get_At(Input_Snapshot_Buffer* buffer, unsigned int index);
void Input_Buffer_Put(Input_Snapshot_Buffer* buffer, struct Input_Snapshot snapshot);
void Input_Snapshot_Init(struct Input_Snapshot* input_snapshot);
bool Input_Snapshot_Push_Command(struct Input_Snapshot* snapshot, struct Command_Entry command);
unsigned int Input_Snapshot_Save_Keyboard_State(struct Input_Snapshot* snapshot, bool* keyboard_state, size_t keyboard_state_len);

#endif /* INPUT_SNAPSHOT_DEF */
#include <stdio.h>
#include <stdlib.h>
#include <enet/enet.h>
#include <SDL3/SDL.H>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "ecdb.h"
#include "vector2.h"
#include "system_render.h"
#include "system_write_input.h"
#include "intstack.h"
#include "component_input.h"
#include "packets.h"
#include "net_manager.h"
#define CLAY_IMPLEMENTATION
#include <clay.h>
#include <clay_renderer_SDL3.c>
#include "clay_ui.c"
#include "game_state.h"
#include "component_handles.h"
#include "component_transform.h"
#include "component_lifetime.h"
#include "system_lifetime.h"
#include "window_state.h"
#include "entity_builders.h"
#include "chat_buffers.h"
#include "component_physics_2d.h"
#include "system_physics.h"
#include "system_interpolation.h"
#include "input_buffer.h"
#include "ring_stack.h"
#include "system_player_state_machine.h"
#include "component_animation.h"
#include "camera.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define CHAT_HISTORY_SIZE 50
#define INTERP_DELAY_MS 300
#define LEVEL_WIDTH 1000
#define LEVEL_HEIGHT 1000

enum Command_Contex
{
    COMMAND_STANDARD,
    COMMAND_CHAT,
};

struct Vector2 Get_Direction_From_Input_State()
{
    const bool* keyboardStates = SDL_GetKeyboardState(NULL);
    return (struct Vector2) {.x = -(keyboardStates[SDL_SCANCODE_A]) + keyboardStates[SDL_SCANCODE_D], .y = -(keyboardStates[SDL_SCANCODE_W]) + keyboardStates[SDL_SCANCODE_S]};
}

enum Command_Contex Handle_Standard_Input_Event(SDL_Event* event)
{
    if( event->type == SDL_EVENT_KEY_DOWN && event->key.repeat == 0)
    {
        // If enter clicked, change context to text context
        if (event->key.key == SDLK_RETURN)
        {
            return COMMAND_CHAT;
        }
    }

    // if here, no change in context
    return COMMAND_STANDARD;
}

enum Command_Contex Handle_Chat_Input_Event(SDL_Event* event, struct Chat_Buffers* chat_buffers, bool* charWritten)
{
    if( event->type == SDL_EVENT_KEY_DOWN)
    {
        if (event->key.key == SDLK_RETURN)
        {
            // If enter clicked, change context to standard context
            return COMMAND_STANDARD;
        }
        if (event->key.key == SDLK_BACKSPACE)
        {
            Chat_Remove_Last_Input(chat_buffers);
        }
    }
    else if (event->type == SDL_EVENT_TEXT_INPUT)
    {
        *charWritten = Chat_Try_Write_To_Input(chat_buffers, event->text.text, strlen(event->text.text));
    }

    // if here, no change in context
    return COMMAND_CHAT;
}

void Add_Networked_Entity(struct Game_Data* gameData, struct ECDB* ecdb, int network_id_handle, unsigned int entityId, unsigned int networkId)
{
    unsigned int* network_id_comp = ECDB_EnableEntityComponent(ecdb, entityId, network_id_handle);
    *network_id_comp = networkId;
    gameData->networkIdEntityMap[networkId] = entityId;
}

void Remove_Networked_Entity(struct Game_Data* gameData, struct ECDB* ecdb, int network_id_handle, unsigned int networkId)
{
    unsigned int local_entity_id = gameData->networkIdEntityMap[networkId];
    if (local_entity_id == ecdb->invalidEntityId)
    {
        SDL_Log("Networked entity ID %i not found, can't remove", networkId);
    }
    gameData->networkIdEntityMap[networkId] = ecdb->invalidEntityId;
    ECDB_DestroyEntity(ecdb, local_entity_id);

    // Destroy any children. TODO: Some kind of heirarchy to do this automatically, or a cleanup job
    struct C_Transform* transforms = (struct C_Transform*) ecdb->componentArrays[gameData->componentHandles.transforms_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ecdb, i, gameData->componentHandles.transforms_handle) && transforms[i].parent_id == local_entity_id)
        {
            ECDB_DestroyEntity(ecdb, i);
        }
    }
}

struct Game_State_Snapshot
{
    uint64_t client_time_ms;
};

void Save_State_History(struct ECDB* ecdb, struct Ring_Stack* game_state_history_stack, uint64_t client_time_ms)
{
    struct Game_State_Snapshot* snapshot = Ring_Stack_Push(game_state_history_stack);
    snapshot->client_time_ms = client_time_ms;

    // Snapshot data is after the struct header
    void* ecdb_state_snapshot = (char*)snapshot + sizeof(struct Game_State_Snapshot);
    ECDB_Generate_Snapshot(ecdb, ecdb_state_snapshot);
}

void Run_Sim(struct ECDB* ecdb, struct Net_Manager* net_manager, struct Component_Handles* component_handles, struct Input_Snapshot* input, struct Animation* animations, unsigned int player_id, float delta_time_s)
{
    // If any texts were received this frame, add them back
    for(unsigned int i = 0; i < input->chat_messages_cached; ++i)
    {
        if (ecdb->validEntities[input->chat_cache[i].entity_id])
        {
            unsigned int textMessageId;
            AddParentedTextWithLifetime(ecdb, component_handles, input->chat_cache[i].entity_id, (struct Vector2){ 0, -60}, input->chat_cache[i].message, 2, &textMessageId);
        }
    }

    // if prediction was toggled, add or remove physics
    if (input->prediction_toggled_off == true)
    {
        // if prediction was disabled, disable by removing the physics component from the player
        ECDB_DisableEntityComponent(ecdb, player_id, component_handles->player_physics_2d_handle);
    }
    else if (input->prediction_toggled_on == true)
    {
        // If prediction was enabled, enable by adding a default physics component to the player
        ECDB_EnableEntityComponent(ecdb, player_id, component_handles->player_physics_2d_handle);
    }

    if (input->interpolation_toggled_off == true)
    {
        // Disable all interpolation buffers; if the entity doesn't have one, nothing happens
        for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
        {
            ECDB_DisableEntityComponent(ecdb, i, component_handles->transforms_interpolation_buffer_handle);
        }
    }
    else if (input->interpolation_toggled_on == true)
    {
        // Enable interpolation by adding an interpolation buffer to every networked transform (ignore the player as a special case)
        for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
        {
            if (i != player_id && ECDB_EntityHasComponent(ecdb, i, component_handles->network_id_handle) && ECDB_EntityHasComponent(ecdb, i, component_handles->transforms_handle))
            {
                ECDB_EnableEntityComponent(ecdb, i, component_handles->transforms_interpolation_buffer_handle);
            }
        }
    }

    s_interpolate_position(ecdb, component_handles->transforms_handle, component_handles->transforms_interpolation_buffer_handle, Net_Estimate_Server_Time(net_manager, input->client_time), net_manager->update_packets_per_s, 2, Net_Get_Round_Trip_Time_Ms(net_manager));
    s_lifetime_iterate(ecdb, component_handles->lifetimes_handle, delta_time_s);
    s_lifetime_remove(ecdb, component_handles->lifetimes_handle);
    s_write_input(ecdb, component_handles->inputs_handle, input->direction);
    s_player_state_machine(ecdb, component_handles->inputs_handle, component_handles->player_states_handle, component_handles->player_physics_2d_handle, component_handles->animation_instance_handle, delta_time_s);
    s_update_physics(ecdb, component_handles->physics_2d_handle, component_handles->inputs_handle, delta_time_s);
    s_apply_physics(ecdb, component_handles->physics_2d_handle, component_handles->transforms_handle, delta_time_s);
    s_apply_physics(ecdb, component_handles->player_physics_2d_handle, component_handles->transforms_handle, delta_time_s);
    s_animation_iterate(ecdb, component_handles->animation_instance_handle, delta_time_s * 1000, animations); // TODO: Move out of sim so animations can be more fluid
}

int main(int argc, char* args[])
{
    char* address_string;
    if (argc > 1)
    {
        // first arg will be the IP address to connect to
        address_string = args[1];
    }
    else
    {
        // if no first arg given, connect to localhost
        address_string = "localhost";
    }

    SDL_Log("Address Name: %s", address_string);

    bool quit = false;

    // Init window
    struct Window_State* window_state = NULL;
    if(Window_State_Init(&window_state, SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        SDL_Log("Window initialization Successful");
    }
    else
    {
        SDL_Log("Window initialization Failed");
        return 1;
    }

    // Initialize networking
    struct Net_Manager* netManager;
    if (Net_Initialize(&netManager))
    {
        SDL_Log("NetManager Initialized");
    }
    else
    {
        SDL_Log("NetManager Initialization Failed");
        return 1;
    }

    ENetAddress address;
    enet_address_set_host (&address, address_string);
    address.port = 1234;

    // Join the server
    struct P_JOIN_SERVER joinGamePacket;
    if (Net_Join_Server(netManager, &address, &joinGamePacket))
    {
        SDL_Log("Connected to server Successfully");
    }
    else
    {
        SDL_Log("Connection to server Failed");
        return 1;
    }

    // Set initial server time offset
    // Todo: initial calculation seems off by some ms
    Net_Calculate_Server_Time_Offset(netManager, SDL_GetTicks(), joinGamePacket.server_time_ms, joinGamePacket.mocked_latency_ms);

    // Init game state based on server info
    struct Game_Data* gameData = NULL;
    if(Game_Data_Init(&gameData, joinGamePacket.max_entities, joinGamePacket.max_chat_length, CHAT_HISTORY_SIZE))
    {
        SDL_Log("Game Data Initialization Successful");
    }
    else
    {
        SDL_Log("Game Data Initialization Failed");
        return 1;
    }

    unsigned int networked_player;
    SDL_Log("Adding square at position %f, %f", joinGamePacket.position.x, joinGamePacket.position.y);
    if (!AddSquare(gameData->ec, &gameData->componentHandles, joinGamePacket.position, (SDL_FColor){1.0f, 1.0f, 1.0f, SDL_ALPHA_OPAQUE_FLOAT}, &networked_player, "You"))
    {
        SDL_Log("Failed to create player, disconnecting");
        goto disconnect;
    }

    struct C_Input* entityInput = ECDB_EnableEntityComponent(gameData->ec, networked_player, gameData->componentHandles.inputs_handle);
    entityInput->speed=300;

    ECDB_EnableEntityComponent(gameData->ec, networked_player, gameData->componentHandles.player_physics_2d_handle);
    ECDB_EnableEntityComponent(gameData->ec, networked_player, gameData->componentHandles.animation_instance_handle);
    ECDB_EnableEntityComponent(gameData->ec, networked_player, gameData->componentHandles.last_server_position_handle);

    int* player_z_order = ECDB_EnableEntityComponent(gameData->ec, networked_player, gameData->componentHandles.y_order_handle);

    //ECDB_DisableEntityComponent(gameData->ec, networked_player, gameData->componentHandles.colors_handle);

    /*unsigned int local_player;
    if (AddSquare(gameData->ec, &gameData->componentHandles, joinGamePacket.position, (SDL_FColor){1.0f, 1.0f, 1.0f, SDL_ALPHA_OPAQUE_FLOAT}, &local_player, "local"))
    {
        struct C_Input* input = ECDB_EnableEntityComponent(gameData->ec, local_player, gameData->componentHandles.inputs_handle);
        input->speed=300;

        ECDB_EnableEntityComponent(gameData->ec, local_player, gameData->componentHandles.player_physics_2d_handle);
    }
    ECDB_DisableEntityComponent(gameData->ec, local_player, gameData->componentHandles.colors_handle);*/


    // create camera
    unsigned int camera_id;
    if (ECDB_CreateEntity(gameData->ec, &camera_id) == false)
    {
        SDL_Log("Couldn't create camera");
        goto disconnect;
    }
    ECDB_EnableEntityComponent(gameData->ec, camera_id, gameData->componentHandles.transforms_handle);
    struct C_Camera* camera_component = ECDB_EnableEntityComponent(gameData->ec, camera_id, gameData->componentHandles.camera_component_handle);
    camera_component->width = SCREEN_WIDTH;
    camera_component->height = SCREEN_HEIGHT;
    camera_component->target_id = networked_player;

    Add_Networked_Entity(gameData, gameData->ec, gameData->componentHandles.network_id_handle, networked_player, joinGamePacket.network_id);
    SDL_Log("Successfully joined at position %f,%f with network ID of %i", joinGamePacket.position.x,  joinGamePacket.position.y, joinGamePacket.network_id);

    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {0.0f, 0.0f});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {LEVEL_WIDTH, LEVEL_HEIGHT});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {0, LEVEL_HEIGHT});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {LEVEL_WIDTH, 0});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {189.0f, 223.0f});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {250.0f, 595.0f});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {574.0f, 200.0f});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {720.0f, 500.0f});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {814.0f, 25.0f});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {450.0f, 632.0f});
    Add_Flower(gameData->ec, &gameData->componentHandles, (struct Vector2) {350.0f, 900.0f});

    struct Vector2 direction = {.x = 0, .y = 0};
    SDL_Event e;
    Uint64 currentFrameTimeMs = SDL_GetTicks();
    Uint64 previousFrameTimeMs = currentFrameTimeMs;
    ENetEvent event;
    enum Command_Contex command_context = COMMAND_STANDARD;

    // TODO: Move prediction logic elsewhere, only here temporarily
    unsigned int history_frames_to_save = 2000;
    Input_Snapshot_Buffer* input_queue;
    Input_Buffer_Init(&input_queue, history_frames_to_save);

    // Game state snapshot is the snapshot struct + the actual game state
    size_t ecdb_snapshot_size = ECDB_Snapshot_Size(gameData->ec);
    size_t game_state_ring_stack_size = Ring_Stack_Calculate_Required_Memory(sizeof(struct Game_State_Snapshot) + ecdb_snapshot_size, history_frames_to_save);
    struct Ring_Stack* game_state_history_stack = calloc(1, game_state_ring_stack_size);
    if (game_state_history_stack == NULL)
    {
        // couldn't instantiate game state history
        SDL_Log("cant alloc state history buffer");
        return 1;
    }

    Ring_Stack_Init(game_state_history_stack, sizeof(struct Game_State_Snapshot) + ecdb_snapshot_size, history_frames_to_save);

    bool client_side_prediction_enabled = true;
    bool client_side_interpolation_enabled = true;

    float targetSecPerFrame = (1.0f / (float)60 );
    float sim_accumulator_s = 0;

    unsigned int sim_frames = 0;
    unsigned int sim_frames_at_last_update = 0;
    unsigned int successful_frames = 0;

    struct Input_Snapshot input_snapshot;
    Input_Snapshot_Init(&input_snapshot);

    unsigned int last_checked = 0;

    SDL_Log("Starting game loop");
    while(quit == false)
    {
        previousFrameTimeMs = currentFrameTimeMs;
        currentFrameTimeMs = SDL_GetTicks();
        // TODO: change all delta time usage from seconds to miliseconds to avoid floating points?
        uint64_t deltaTimeMs = currentFrameTimeMs - previousFrameTimeMs;
        float deltaTimeS = (float)deltaTimeMs / 1000;

        // The input snapshot for this frame
        //struct Input_Snapshot input_snapshot = {.client_time = currentFrameTimeMs, .chat_messages_cached = 0 };

        //SDL_Log("Corrected client time: %lu", Net_Estimate_Server_Time(netManager, currentFrameTimeMs));

        // TODO: move to netmanager? decouple?
        while (enet_host_service(netManager->client, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
            {
                switch(event.channelID)
                {
                    case 0: // General packets
                    {
                        enum Packet_Type type = (enum Packet_Type) *(event.packet->data);
                        switch(type)
                        {
                        case UPDATE:
                        {
                            // Update packets start with a header followed by an array of updates
                            struct P_Update_Header* header = (struct P_Update_Header*)event.packet->data;

                            // When we receive an update header, revert to the state that was right before that in server time
                            // TODO: End of frame is actually equivalent to current ECDB data since the sim hasn't been re-run yet. make more clear?
                            // TODO: Calculate how many frames we should replay based on latency and see if it matches up
                            int going_back_frames = 0;
                            //SDL_Log("Server time: %lu", header->server_time_ms);
                            while(client_side_prediction_enabled == true && game_state_history_stack->buffer_size > 0)
                            {
                                // TODO: becuase the first saved snapshot is always the current state, it doesn't make sense to test it or roll back to it 
                                struct Game_State_Snapshot* state = (struct Game_State_Snapshot*)Ring_Stack_Pop(game_state_history_stack);
                                going_back_frames++;
                                // If the state is less than or matches server time, or if we are at the oldest state we have on record, use it. look at the previous frame to make sure we are looking at the old
                                uint64_t snapshot_estimated_server_time = Net_Estimate_Server_Time(netManager, state->client_time_ms);
                                //SDL_Log("\tClient Estimated Server Time: %lu. Diff: %li", snapshot_estimated_server_time, (long)((snapshot_estimated_server_time + 1000) - header->server_time_ms) - 1000);
                                if (game_state_history_stack->buffer_size == 0 || snapshot_estimated_server_time <= header->server_time_ms)
                                {
                                    void* ecdb_state_snapshot = (char*)state + sizeof(struct Game_State_Snapshot);
                                    ECDB_Apply_Snapshot(gameData->ec, ecdb_state_snapshot);
                                    // We have a new starting point, so clear state history
                                    Ring_Stack_Clear(game_state_history_stack);
                                    break;
                                }
                            }

                            // Calculate how many frames we expect to go back
                            /*unsigned int sim_frames_since_last_update = sim_frames - sim_frames_at_last_update;
                            if (sim_frames_since_last_update != going_back_frames)
                            {
                                SDL_Log("Going back frames diverged: Sim frames since last update: %u. Frames going back: %u. Successes: %u", sim_frames_since_last_update, going_back_frames, successful_frames);
                                successful_frames = 0;
                            }
                            else
                            {
                                successful_frames++;
                            }
                            sim_frames_at_last_update = sim_frames;*/

                            // Record removals
                            unsigned int* removal_buffer_ptr = (unsigned int*)(((char*)event.packet->data) + sizeof(struct P_Update_Header));
                            for(unsigned int i = 0; i < header->removals_count; ++i)
                            {
                                unsigned int network_entity_to_remove = removal_buffer_ptr[i];
                                Remove_Networked_Entity(gameData, gameData->ec, gameData->componentHandles.network_id_handle, network_entity_to_remove);
                            }

                            // Record updates
                            struct P_Update_Entity_Data* update_buffer_ptr = (struct P_Update_Entity_Data*)(((char*)event.packet->data) + sizeof(struct P_Update_Header) + (header->removals_count * sizeof(unsigned int)));

                            for(unsigned int i = 0; i < header->updates_count; ++i)
                            {
                                struct P_Update_Entity_Data update = update_buffer_ptr[i];
                                if (gameData->networkIdEntityMap[update.networkId] == gameData->ec->invalidEntityId)
                                {
                                    // if we don't know about the entity, add it
                                    unsigned int entityId;
                                    char playerNameBuffer[10];
                                    int strlen = sprintf(playerNameBuffer, "Player %i", update.networkId);
                                    if (AddSquare(gameData->ec, &gameData->componentHandles, update.position, (SDL_FColor){0.5f, 0.5f, 0.5f, SDL_ALPHA_OPAQUE_FLOAT}, &entityId, playerNameBuffer))
                                    {
                                        // all incoming players will have transform interpolation
                                        ECDB_EnableEntityComponent(gameData->ec, entityId, gameData->componentHandles.transforms_interpolation_buffer_handle);
                                        ECDB_EnableEntityComponent(gameData->ec, entityId, gameData->componentHandles.last_server_position_handle);
                                        ECDB_EnableEntityComponent(gameData->ec, entityId, gameData->componentHandles.animation_instance_handle);
                                        int* z_order = ECDB_EnableEntityComponent(gameData->ec, entityId, gameData->componentHandles.y_order_handle);

                                        Add_Networked_Entity(gameData, gameData->ec, gameData->componentHandles.network_id_handle, entityId, update.networkId);
                                        SDL_Log("Player joined at position %f,%f with network ID of %i. Assigned to entity ID %i", update.position.x,  update.position.y, update.networkId, entityId);
                                    }
                                    else
                                    {
                                        SDL_Log("Too many entities received from server. Disconnecting.");
                                        goto disconnect;
                                    }
                                }

                                int localEntityId = gameData->networkIdEntityMap[update.networkId];
                                if(ECDB_EntityHasComponent(gameData->ec, localEntityId, gameData->componentHandles.transforms_handle))
                                {
                                    if (ECDB_EntityHasComponent(gameData->ec, localEntityId, gameData->componentHandles.transforms_interpolation_buffer_handle))
                                    {
                                        // Unreliable packets are still sequenced, so we know this is the latest transform message
                                        struct N_C_Transform_Interpolation_Buffer* trans_buf = (struct N_C_Transform_Interpolation_Buffer*)ECDB_GetEntityComponent(gameData->ec, localEntityId, gameData->componentHandles.transforms_interpolation_buffer_handle);
                                        struct C_Transform* transform = (struct C_Transform*)ECDB_GetEntityComponent(gameData->ec, localEntityId, gameData->componentHandles.transforms_handle);
                                        struct N_C_Transform_Snapshot trans_snap = {.server_time = header->server_time_ms, .transform = *transform};
                                        trans_snap.transform.position = update.position;
                                        Interp_Buf_Add(trans_buf, trans_snap);
                                    }
                                    else
                                    {
                                        // If we aren't interpolating, write actor position directly
                                        struct C_Transform* actorPosition = (struct C_Transform*)ECDB_GetEntityComponent(gameData->ec, localEntityId, gameData->componentHandles.transforms_handle);
                                        actorPosition->position = update.position;
                                    }

                                    // record last position for debugging
                                    if (ECDB_EntityHasComponent(gameData->ec, localEntityId, gameData->componentHandles.last_server_position_handle))
                                    {
                                        struct Vector2* lastposition = (struct Vector2*)ECDB_GetEntityComponent(gameData->ec, localEntityId, gameData->componentHandles.last_server_position_handle);
                                        *lastposition = update.position;
                                    }
                                }

                                if (ECDB_EntityHasComponent(gameData->ec, localEntityId, gameData->componentHandles.player_physics_2d_handle))
                                {
                                    struct C_Physics_2d* physics = (struct C_Physics_2d*)ECDB_GetEntityComponent(gameData->ec, localEntityId, gameData->componentHandles.player_physics_2d_handle);
                                    physics->velocity = update.velocity;
                                }

                                if (ECDB_EntityHasComponent(gameData->ec, localEntityId, gameData->componentHandles.player_states_handle))
                                {
                                    struct C_Player_State* state = (struct C_Player_State*)ECDB_GetEntityComponent(gameData->ec, localEntityId, gameData->componentHandles.player_states_handle);
                                    *state = update.state;
                                }
                            }

                            // replay the same amount of input as missed frames. the input frame has already been played on the corresponding state frame
                            // as they are both written to at the same time, buffer size will always be at least as big as going back frames.
                            for(unsigned int i = input_queue->buffer_size - going_back_frames + 1; i < input_queue->buffer_size; ++i)
                            {
                                // Re-play any input captured after the last frame.
                                struct Input_Snapshot input = Input_Buffer_Get_At(input_queue, i);

                                // calculate delta time based on previous input time. If no previous value, use the current frame's as an approximation
                                float replay_delta_time_s = deltaTimeS;
                                if (i != 0)
                                {
                                    // calculate delta time based on previous input time
                                    float previous_frame_time_ms = Input_Buffer_Get_At(input_queue, i - 1).client_time;
                                    replay_delta_time_s = (float)(input.client_time - previous_frame_time_ms) / 1000;
                                }

                                // TODO - instead of calling run_sim in two places in the main loop, put all input together and then play it out later?
                                Run_Sim(gameData->ec, netManager, &(gameData->componentHandles), &input, gameData->animations, networked_player, replay_delta_time_s);
                            }

                            break;
                        }
                        case SERVER_TIME:
                        {
                            struct P_Server_Time* packetData = (struct P_Server_Time*) event.packet->data;
                            unsigned long estimated_server_time_ms = Net_Estimate_Server_Time(netManager, currentFrameTimeMs) - ((netManager->serverPeer->roundTripTime + packetData->mocked_latency_ms) / 2);
                            // TODO: packet loss is a fixed point number, convert to decimal notation before displaying
                            SDL_Log("Server time: %lu. Estimated server time when message sent: %lu. Server time diff: %li. Round trip time: %i. Mocked round trip time: %i. Packet Loss: %i", packetData->server_time_ms, estimated_server_time_ms, (long)((estimated_server_time_ms + 1000) -  packetData->server_time_ms) - 1000, event.peer->roundTripTime, event.peer->roundTripTime + packetData->mocked_latency_ms, event.peer->packetLoss);
                            Net_Calculate_Server_Time_Offset(netManager, currentFrameTimeMs, packetData->server_time_ms, packetData->mocked_latency_ms);
                            break;
                        }
                        default:
                            printf ("Some weird packet of type %i\n", type);
                            break;
                        }

                        break;
                    }
                    case 1: // Chat packets
                    {
                        // Chat packets start with a header followed by the chat string
                        struct P_Chat_Header* header = (struct P_Chat_Header*)event.packet->data;
                        char* text_pointer = ((char*)event.packet->data) + sizeof(struct P_Chat_Header);
                        unsigned int chat_length = strlen(text_pointer);
                        unsigned int max_prefix_length = 10;
                        char* prefixed_message_buffer = _malloca(chat_length + max_prefix_length);
                        unsigned int prefixed_message_length;

                        if (header->isServerMessage)
                        {
                            prefixed_message_length = sprintf(prefixed_message_buffer, "Server: %s", text_pointer);
                        }
                        else
                        {
                            unsigned int localEntityId = gameData->networkIdEntityMap[header->networkId];
                            if (localEntityId == networked_player)
                            {
                                prefixed_message_length = sprintf(prefixed_message_buffer, "You: %s", text_pointer);
                            }
                            else
                            {
                                prefixed_message_length = sprintf(prefixed_message_buffer, "Player %i: %s", header->networkId, text_pointer);
                            }

                            if (input_snapshot.chat_messages_cached < 10)
                            {
                                struct Chat_Snapshot_Info* chat_snapshot = &(input_snapshot.chat_cache[input_snapshot.chat_messages_cached]);
                                chat_snapshot->entity_id = localEntityId;
                                strcpy(chat_snapshot->message, text_pointer);
                                input_snapshot.chat_messages_cached++;
                            }
                        }

                        // Write the prefixed message to the chat history
                        Chat_History_Write(gameData->chat_buffers, prefixed_message_buffer, prefixed_message_length);

                        break;
                    }
                    default:
                    {
                        printf("Message received from server on unexpected channel %i\n", event.channelID);
                        break;
                    }
                }

                enet_packet_destroy (event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                SDL_Log("Disconnected from the server.");
                netManager->connected = false;
                break;
            }
            }
        }

        sim_accumulator_s += deltaTimeS;
        while (sim_accumulator_s > targetSecPerFrame)
        {
            bool directionChanged = false;

            //  Handle keyboard events
            while( SDL_PollEvent( &e ) == true )
            {
                if( e.type == SDL_EVENT_QUIT )
                {
                    quit = true;
                }
                else if (e.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    Clay_SetLayoutDimensions((Clay_Dimensions) { (float) e.window.data1, (float) e.window.data2 });

                    // resize camera
                    struct C_Camera* camera_position = (struct C_Camera*)ECDB_GetEntityComponent(gameData->ec, camera_id, gameData->componentHandles.camera_component_handle);
                    camera_position->width = (float)e.window.data1;
                    camera_position->height = (float)e.window.data2;
                }
                else if (e.type == SDL_EVENT_MOUSE_WHEEL)
                {
                    Clay_UpdateScrollContainers(true, (Clay_Vector2) { e.wheel.x, e.wheel.y }, targetSecPerFrame);
                }
                else if (command_context == COMMAND_STANDARD)
                {
                    if( e.type == SDL_EVENT_KEY_DOWN && e.key.repeat == 0)
                    {
                        // If 1 clicked, turn client side prediction off
                        if (e.key.key == SDLK_1)
                        {
                            if (client_side_prediction_enabled == true)
                            {
                                input_snapshot.prediction_toggled_off = true;
                            }
                            else
                            {
                                input_snapshot.prediction_toggled_on = true;
                            }

                            client_side_prediction_enabled = !client_side_prediction_enabled;
                        }

                        // If 2 clicked, turn off client side interpolation
                        if (e.key.key == SDLK_2)
                        {
                            if (client_side_interpolation_enabled == true)
                            {
                                input_snapshot.interpolation_toggled_off = true;
                            }
                            else
                            {
                                input_snapshot.interpolation_toggled_on = true;
                            }

                            client_side_interpolation_enabled = !client_side_interpolation_enabled;
                        }
                    }
                    
                    command_context = Handle_Standard_Input_Event(&e);
                    if (command_context != COMMAND_STANDARD)
                    {
                        // If the context changed, stop movement
                        if (direction.x != 0 || direction.y != 0)
                        {
                            directionChanged = true;
                            direction.x = 0;
                            direction.y = 0;
                        }

                        if (command_context == COMMAND_CHAT)
                        {
                            SDL_StartTextInput(window_state->window);
                        }
                    }
                }
                else if (command_context == COMMAND_CHAT)
                {
                    bool charWritten = false;
                    command_context = Handle_Chat_Input_Event(&e, gameData->chat_buffers, &charWritten);
                    if (command_context != COMMAND_CHAT)
                    {
                        // If we've stopped chatting, send the chat packet
                        int messageSize = (sizeof(char) * gameData->chat_buffers->_input_cursor) + 1; // Size is number of characters + the null termination character
                        ENetPacket* chatPacket = enet_packet_create(gameData->chat_buffers->chat_input_buffer, messageSize, ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(netManager->serverPeer, 1, chatPacket); // Send on channel 1 as the chat channel

                        // reset the buffer
                        Chat_Reset_Input_Buffer(gameData->chat_buffers);

                        printf("\n");
                        SDL_StopTextInput(window_state->window);
                    }
                    else if (charWritten)
                    {
                        // If still chatting, write the recent character to the console
                        printf("%c", gameData->chat_buffers->chat_input_buffer[gameData->chat_buffers->_input_cursor - 1]); // Chat cursor is always at current char + 1
                    }
                }
            }

            // Handle keyboard state
            if (command_context == COMMAND_STANDARD)
            {
                struct Vector2 newDirection = Get_Direction_From_Input_State();

                if (direction.x != newDirection.x || direction.y != newDirection.y)
                {
                    directionChanged = true;
                    direction.x = newDirection.x;
                    direction.y = newDirection.y;
                }
            }

            // save direction in the frame snapshot
            input_snapshot.direction = direction;

            // Handle mouse movement
            struct Vector2 mousePos;
            Uint32 buttons = SDL_GetMouseState(&(mousePos.x), &(mousePos.y));
            Clay_SetPointerState(
                (Clay_Vector2){.x = mousePos.x, .y = mousePos.y},
                buttons & SDL_BUTTON_LMASK
            );
            
            if (directionChanged)
            {
                // If input has been given, send an input packet
                unsigned int* entityId = ECDB_GetEntityComponent(gameData->ec, networked_player, gameData->componentHandles.network_id_handle);
                struct P_Input_Direction inputPacket = {.type = INPUT_DIRECTION, .networkId = *entityId, .direction = direction};
                ENetPacket * packet = enet_packet_create(&inputPacket, sizeof(struct P_Input_Direction), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(netManager->serverPeer, 0, packet);
            }
            
            input_snapshot.client_time = currentFrameTimeMs;

            Run_Sim(gameData->ec, netManager, &(gameData->componentHandles), &input_snapshot, gameData->animations, networked_player, targetSecPerFrame);
            //Run_Sim(gameData->ec, netManager, &(gameData->componentHandles), &input_snapshot, networked_player, deltaTimeS);
            
            // save state
            Input_Buffer_Put(input_queue, input_snapshot);
            Save_State_History(gameData->ec, game_state_history_stack, currentFrameTimeMs);

            // reset the input snapshot
            Input_Snapshot_Init(&input_snapshot);

            sim_frames++;

            // pull back the accumulator
            sim_accumulator_s -= targetSecPerFrame;
        }

        // Clear previous render before drawing
        SDL_SetRenderDrawColor(window_state->renderer, 98, 189, 32, SDL_ALPHA_OPAQUE ); // Black
        SDL_RenderClear(window_state->renderer);
        s_camera_reposition(gameData->ec, camera_id, gameData->componentHandles.transforms_handle, gameData->componentHandles.camera_component_handle, LEVEL_WIDTH, LEVEL_HEIGHT);
        s_render_server_ghost(gameData->ec, camera_id, gameData->componentHandles.transforms_handle, gameData->componentHandles.last_server_position_handle, gameData->componentHandles.animation_instance_handle, gameData->animations, window_state->spritesheet, window_state->renderer);
        s_render(gameData->ec, camera_id, &gameData->componentHandles, gameData->animations, window_state->spritesheet, gameData->chat_buffers->_buffer_size, window_state->font, window_state->textEngine, window_state->renderer);        

        Clay_RenderCommandArray renderCommands = Build_UI(gameData, command_context == COMMAND_CHAT, client_side_prediction_enabled, client_side_interpolation_enabled, deltaTimeS);
        Clay_SDL3RendererData renderData = {.renderer = window_state->renderer, .textEngine = window_state->textEngine, .fonts = &window_state->font};
        SDL_Clay_RenderClayCommands(&renderData, &renderCommands);

        // Draw to screen
        SDL_RenderPresent(window_state->renderer);
    }

disconnect:
    Net_Disconnect(netManager);

cleanup:
    Game_Data_Free(&gameData);
    free(input_queue);
    free(game_state_history_stack);

    Net_Free(&netManager);
    netManager = NULL;
    enet_deinitialize();

    Window_State_Free(&window_state);

    return 0;
}
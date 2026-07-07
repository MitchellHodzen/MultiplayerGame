#include <stdio.h>
#include <stdlib.h>
#include <enet/enet.h>
#include <windows.h> 
#include <SDL3/SDL.H>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "ecdb.h"
#include "vector2.h"
#include "system_movement.h"
#include "system_render.h"
#include "system_write_input.h"
#include "intstack.h"
#include "component_input.h"
#include "packets.h"
#include "net_manager.h"
#define CLAY_IMPLEMENTATION
#include <clay.h>
#include <clay_renderer_SDL3.c>
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

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define CHAT_HISTORY_SIZE 50
#define INTERP_DELAY_MS 300

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
        else
        {
            // Any other keys write to the chat buffer if it isn't full. TODO: sanitize input
            *charWritten = Chat_Try_Write_To_Input(chat_buffers, event->key.key);
        }
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

void Remove_Networked_Entity(struct Game_Data* gameData, struct ECDB* ecdb, int network_id_handle, unsigned int entityId, unsigned int networkId)
{
    ECDB_DisableEntityComponent(ecdb, entityId, network_id_handle);
    gameData->networkIdEntityMap[networkId] = ecdb->invalidEntityId;
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

int main(int argc, char* args[])
{
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
    enet_address_set_host (&address, "localhost");
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
    Net_Calculate_Server_Time_Offset(netManager, SDL_GetTicks(), joinGamePacket.server_time_ms);

    // Init game state based on server info
    struct Game_Data* gameData = NULL;
    unsigned int max_chat_length = joinGamePacket.max_chat_length;
    if(Game_Data_Init(&gameData, joinGamePacket.max_entities, joinGamePacket.max_chat_length, CHAT_HISTORY_SIZE))
    {
        SDL_Log("Game Data Initialization Successful");
    }
    else
    {
        SDL_Log("Game Data Initialization Failed");
        return 1;
    }

    int networked_player;
    SDL_Log("Adding square at position %f, %f", joinGamePacket.position.x, joinGamePacket.position.y);
    if (!AddSquare(gameData->ec, &gameData->componentHandles, joinGamePacket.position, (SDL_FColor){1.0f, 1.0f, 1.0f, SDL_ALPHA_OPAQUE_FLOAT}, &networked_player, "You"))
    {
        SDL_Log("Failed to create player, disconnecting");
        goto disconnect;
    }

    struct C_Input* entityInput = ECDB_EnableEntityComponent(gameData->ec, networked_player, gameData->componentHandles.inputs_handle);
    entityInput->speed=100;

    struct C_Physics_2d* physics = ECDB_EnableEntityComponent(gameData->ec, networked_player, gameData->componentHandles.physics_2d_handle);
    physics->friction = 25;

    ECDB_EnableEntityComponent(gameData->ec, networked_player, gameData->componentHandles.last_server_position_handle);

    Add_Networked_Entity(gameData, gameData->ec, gameData->componentHandles.network_id_handle, networked_player, joinGamePacket.network_id);
    SDL_Log("Successfully joined at position %f,%f with network ID of %i", joinGamePacket.position.x,  joinGamePacket.position.y, joinGamePacket.network_id);

    // Chat UI tracking
    float previousChatBottom = 0;

    struct Vector2 direction = {.x = 0, .y = 0};
    SDL_Event e;
    Uint64 currentFrameTimeMs = SDL_GetTicks();
    Uint64 previousFrameTimeMs = currentFrameTimeMs;
    ENetEvent event;
    enum Command_Contex command_context = COMMAND_STANDARD;

    // TODO: Move prediction logic elsewhere, only here temporarily
    unsigned int history_frames_to_save = 3000;
    Input_Snapshot_Buffer* input_queue;
    Input_Buffer_Init(&input_queue, history_frames_to_save);

    // Game state snapshot is the snapshot struct + the actual game state
    size_t ecdb_snapshot_size = ECDB_Snapshot_Size(gameData->ec);
    size_t game_state_ring_stack_size = Ring_Stack_Calculate_Required_Memory(sizeof(struct Game_State_Snapshot) + ecdb_snapshot_size, history_frames_to_save);
    SDL_Log("ECDB Snapshot Size: %i. Buffer size: %i", ecdb_snapshot_size, game_state_ring_stack_size);
    struct Ring_Stack* game_state_history_stack = calloc(1, game_state_ring_stack_size);
    if (game_state_history_stack == NULL)
    {
        // couldn't instantiate game state history
        SDL_Log("cant alloc state history buffer");
        return 1;
    }

    Ring_Stack_Init(game_state_history_stack, sizeof(struct Game_State_Snapshot) + ecdb_snapshot_size, history_frames_to_save);

    SDL_Log("Starting game loop");
    while(quit == false)
    {
        previousFrameTimeMs = currentFrameTimeMs;
        currentFrameTimeMs = SDL_GetTicks();
        float deltaTimeS = (float)(currentFrameTimeMs - previousFrameTimeMs) / 1000;

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
                            uint64_t revert_frame_time_ms;
                            int testindex = 0;
                            while(game_state_history_stack->buffer_size > 0)
                            {
                                struct Game_State_Snapshot* state = (struct Game_State_Snapshot*)Ring_Stack_Pop(game_state_history_stack);
                                uint64_t state_calculated_server_time = Net_Estimate_Server_Time(netManager, state->client_time_ms);
                                if (state_calculated_server_time <= header->server_time_ms)
                                {
                                    // this state occurs before the server time, use it
                                    void* ecdb_state_snapshot = (char*)state + sizeof(struct Game_State_Snapshot);
                                    ECDB_Apply_Snapshot(gameData->ec, ecdb_state_snapshot);
                                    revert_frame_time_ms = state->client_time_ms;
                                    break;
                                }
                                testindex++;
                            }

                            // Record updates
                            struct P_Update_Entity_Data* update_buffer_ptr = ((char*)event.packet->data) + sizeof(struct P_Update_Header);

                            for(int i = 0; i < header->updates_count; ++i)
                            {
                                struct P_Update_Entity_Data update = update_buffer_ptr[i];
                                if (gameData->networkIdEntityMap[update.networkId] == gameData->ec->invalidEntityId)
                                {
                                    // if we don't know about the entity, add it
                                    int entityId;
                                    char playerNameBuffer[10];
                                    int strlen = sprintf(playerNameBuffer, "Player %i", update.networkId);
                                    if (AddSquare(gameData->ec, &gameData->componentHandles, update.position, (SDL_FColor){0.5f, 0.5f, 0.5f, SDL_ALPHA_OPAQUE_FLOAT}, &entityId, playerNameBuffer))
                                    {
                                        // all incoming players will have transform interpolation
                                        ECDB_EnableEntityComponent(gameData->ec, entityId, gameData->componentHandles.transforms_interpolation_buffer_handle);
                                        ECDB_EnableEntityComponent(gameData->ec, entityId, gameData->componentHandles.last_server_position_handle);
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
                            }

                            // Re-run simulation to bring it back up to current time
                            for(unsigned int i = 0; i < input_queue->buffer_size; ++i)
                            {
                                // Re-play any input captured after the last frame
                                struct Input_Snapshot input = Input_Buffer_Get_At(input_queue, i);
                                if (input.client_time >= revert_frame_time_ms)
                                {
                                    // calculate delta time based on previous input time. If no previous value, use the current frame's as an approximation
                                    float replay_delta_time_s = deltaTimeS;
                                    if (i != 0)
                                    {
                                        // calculate delta time based on previous input time
                                        float previous_frame_time_ms = Input_Buffer_Get_At(input_queue, i - 1).client_time;
                                        replay_delta_time_s = (float)(input.client_time - previous_frame_time_ms) / 1000;
                                    }

                                    s_interpolate_position(gameData->ec, gameData->componentHandles.transforms_handle, gameData->componentHandles.transforms_interpolation_buffer_handle, Net_Estimate_Server_Time(netManager, input.client_time), INTERP_DELAY_MS);
                                    s_lifetime_iterate(gameData->ec, gameData->componentHandles.lifetimes_handle, replay_delta_time_s);
                                    s_lifetime_remove(gameData->ec, gameData->componentHandles.lifetimes_handle);
                                    s_write_input(gameData->ec, gameData->componentHandles.inputs_handle, input.direction); // <- direction applied
                                    s_update_physics(gameData->ec, gameData->componentHandles.physics_2d_handle, gameData->componentHandles.inputs_handle, replay_delta_time_s);
                                    s_apply_physics(gameData->ec, gameData->componentHandles.physics_2d_handle, gameData->componentHandles.transforms_handle, replay_delta_time_s);
                                }
                            }

                            break;
                        }
                        case SERVER_TIME:
                        {
                            struct P_Server_Time* packetData = (struct P_Server_Time*) event.packet->data;
                            long estimated_server_time_ms = Net_Estimate_Server_Time(netManager, currentFrameTimeMs);
                            long corrected_server_time = packetData->server_time_ms - (event.peer->roundTripTime / 2);

                            SDL_Log("Server time: %i. Corrected server time: %i. Estimated server time: %i. Server time diff: %i. Corrected server time diff: %i. Round trip time: %i", packetData->server_time_ms, corrected_server_time, estimated_server_time_ms, estimated_server_time_ms - packetData->server_time_ms, estimated_server_time_ms - corrected_server_time, event.peer->roundTripTime);
                            Net_Calculate_Server_Time_Offset(netManager, currentFrameTimeMs, packetData->server_time_ms);
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
                            int localEntityId = gameData->networkIdEntityMap[header->networkId];
                            if (localEntityId == networked_player)
                            {
                                prefixed_message_length = sprintf(prefixed_message_buffer, "You: %s", text_pointer);
                            }
                            else
                            {
                                prefixed_message_length = sprintf(prefixed_message_buffer, "Player %i: %s", header->networkId, text_pointer);
                            }

                            // Display text above character
                            int textMessageId;
                            AddParentedTextWithLifetime(gameData->ec, &(gameData->componentHandles), localEntityId, (struct Vector2){ 0, -60}, text_pointer, 2, &textMessageId);
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
            }
            else if (e.type == SDL_EVENT_MOUSE_WHEEL)
            {
                Clay_UpdateScrollContainers(true, (Clay_Vector2) { e.wheel.x, e.wheel.y }, deltaTimeS);
            }
            else if (command_context == COMMAND_STANDARD)
            {
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
            ENetPacket * packet = enet_packet_create(&inputPacket, sizeof(struct P_Input_Direction), 0);
            enet_peer_send(netManager->serverPeer, 0, packet);
        }

        s_interpolate_position(gameData->ec, gameData->componentHandles.transforms_handle, gameData->componentHandles.transforms_interpolation_buffer_handle, Net_Estimate_Server_Time(netManager, currentFrameTimeMs), INTERP_DELAY_MS);
        s_lifetime_iterate(gameData->ec, gameData->componentHandles.lifetimes_handle, deltaTimeS);
        s_lifetime_remove(gameData->ec, gameData->componentHandles.lifetimes_handle);
        s_write_input(gameData->ec, gameData->componentHandles.inputs_handle, direction);
        s_update_physics(gameData->ec, gameData->componentHandles.physics_2d_handle, gameData->componentHandles.inputs_handle, deltaTimeS);
        s_apply_physics(gameData->ec, gameData->componentHandles.physics_2d_handle, gameData->componentHandles.transforms_handle, deltaTimeS);

        // Clear previous render before drawing
        SDL_SetRenderDrawColor(window_state->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE ); // Black
        SDL_RenderClear(window_state->renderer);

        s_render(gameData->ec, gameData->componentHandles.transforms_handle, gameData->componentHandles.colors_handle, gameData->componentHandles.text_handle, window_state->font, window_state->textEngine, window_state->renderer);
        s_render_server_ghost(gameData->ec, gameData->componentHandles.last_server_position_handle, window_state->renderer);

        // Chat box UI
        Clay_BeginLayout();
        CLAY(CLAY_ID("ChatParentContainer"), { .layout = { .sizing = { .width = CLAY_SIZING_PERCENT(0.5f), .height = CLAY_SIZING_GROW(0) }, .padding = {.left = 5, .right = 0, .top = 0, .bottom = 5 } , .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM}, .layoutDirection = CLAY_TOP_TO_BOTTOM}, .backgroundColor = {0,0,0,0} }) {
            CLAY(CLAY_ID("FullChatWindowContainer"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_PERCENT(0.5f) }, .childGap = 3, .layoutDirection = CLAY_TOP_TO_BOTTOM, .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM} }, .backgroundColor = { 50, 50, 50, 100 }
            }) {
                CLAY(CLAY_ID("ChatHistoryContainer"), {
                .clip = { .vertical = true, .childOffset = { Clay_GetScrollOffset().x, Clay_GetScrollOffset().y } }, .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .childGap = 0, .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM} }
                }) {
                    for (int i = 0; i < gameData->chat_buffers->chat_history_buffer->buffer_size; ++i)
                    {
                        CLAY(CLAY_IDI("Chat", i), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .padding = CLAY_PADDING_ALL(5), .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER } } }) {
                            CLAY_TEXT(((Clay_String) { .length = strlen(Chat_Get_Message_At(gameData->chat_buffers, i)), .chars = Chat_Get_Message_At(gameData->chat_buffers, i) }), { .fontSize = 24, .textColor = {255, 255, 255, 255} });
                        }
                    }

                    Clay_ScrollContainerData scrollContainerData = Clay_GetScrollContainerData(CLAY_ID("ChatHistoryContainer"));

                    // If we're at the end, lock scroll to the end
                    if (scrollContainerData.scrollPosition->y == previousChatBottom) // Could scrollposition be null here?
                    {
                        float bottomPosition = -(scrollContainerData.contentDimensions.height - scrollContainerData.scrollContainerDimensions.height);
                        scrollContainerData.scrollPosition->y = bottomPosition;
                    }

                    previousChatBottom = -(scrollContainerData.contentDimensions.height - scrollContainerData.scrollContainerDimensions.height);
                }

                float chatInputBoxAlpha = 100;
                if (command_context == COMMAND_CHAT)
                {
                    // if chatting, make the carrot more visible
                    chatInputBoxAlpha = 200;
                }

                CLAY(CLAY_ID("ChatInputBox"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .layoutDirection = CLAY_LEFT_TO_RIGHT, .padding = CLAY_PADDING_ALL(5), .childGap = 3, .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER } }, .backgroundColor = { 50, 50, 50, chatInputBoxAlpha } }) {
                    float carrotAlpha = 150;
                    if (command_context == COMMAND_CHAT)
                    {
                        // if chatting, make the carrot more visible
                        carrotAlpha = 255;
                    }
                    CLAY_TEXT(CLAY_STRING(">"), { .fontSize = 24, .textColor = {255, 255, 255, carrotAlpha} });
                    CLAY_TEXT(((Clay_String) { .length = strlen(gameData->chat_buffers->chat_input_buffer), .chars = gameData->chat_buffers->chat_input_buffer }), { .fontSize = 24, .textColor = {255, 255, 255, 255} });
                }
            }
        }

        Clay_RenderCommandArray renderCommands = Clay_EndLayout(deltaTimeS);
        Clay_SDL3RendererData renderData = {.renderer = window_state->renderer, .textEngine = window_state->textEngine, .fonts = &window_state->font};
        SDL_Clay_RenderClayCommands(&renderData, &renderCommands);

        // Draw to screen
        SDL_RenderPresent(window_state->renderer);

        // save state
        struct Input_Snapshot snapshot = {.client_time = currentFrameTimeMs, .direction = direction };
        Input_Buffer_Put(input_queue, snapshot);
        Save_State_History(gameData->ec, game_state_history_stack, currentFrameTimeMs);
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
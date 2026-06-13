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
#include "system_apply_input.h"
#include "intstack.h"
#include "component_input.h"
#include "packets.h"
#include "net_manager.h"
#define CLAY_IMPLEMENTATION
#include <clay.h>
#include <clay_renderer_SDL3.c>
#include "initialization.h"
#include "component_handles.h"
#include "component_transform.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define ENTITY_COUNT 100
#define CHAT_MAX_SIZE 100
#define CHAT_HISTORY_SIZE 50

enum Command_Contex
{
    COMMAND_STANDARD,
    COMMAND_CHAT,
};

bool AddSquare(struct ECDB* ec, struct Component_Handles* componentHandles, struct Vector2 position, SDL_FColor color, int* entityId)
{
    if (ECDB_CreateEntity(ec, entityId) == false)
    {
        SDL_Log("Couldn't create square");
        return false;
    }

    struct C_Transform* entityTransform = ECDB_EnableEntityComponent(ec, *entityId, componentHandles->transforms_handle);
    entityTransform->position = position;
    SDL_FColor* entityCol = ECDB_EnableEntityComponent(ec, *entityId, componentHandles->colors_handle);
    memcpy(entityCol, &color, sizeof(SDL_FColor));
    return true;
}

void LogClayErrors(Clay_ErrorData errorData) {
    SDL_Log("%s", errorData.errorText.chars);
}

static inline Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData)
{
    TTF_Font *font = userData; // Only one font
    int width, height;

    TTF_SetFontSize(font, config->fontSize);
    if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }
    return (Clay_Dimensions) { (float) width, (float) height };
}

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

enum Command_Contex Handle_Chat_Input_Event(SDL_Event* event, char* chatBuffer, unsigned int* chatCursor, bool* charWritten)
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
            if (*chatCursor < CHAT_MAX_SIZE)
            {
                // buffer is chat max size + 1, so we can safely operate < chat max size
                chatBuffer[*chatCursor] = event->key.key;
                // always put the string end char after the cursor
                chatBuffer[*chatCursor + 1] =  '\0';
                (*chatCursor)++;
                *charWritten = true;
            }
        }
    }

    // if here, no change in context
    return COMMAND_CHAT;
}

int main(int argc, char* args[])
{
    bool quit = false;
    struct Game_Data* gameData = NULL;
    if(Game_Data_Init(&gameData, SCREEN_WIDTH, SCREEN_HEIGHT, ENTITY_COUNT, (Clay_ErrorHandler) { LogClayErrors }, SDL_MeasureText))
    {
        SDL_Log("Initialization Successful");
    }
    else
    {
        SDL_Log("Initialization Failed");
        return 1;
    }

    ENetAddress address;
    enet_address_set_host (&address, "localhost");
    address.port = 1234;
    if (Net_Try_Connect(gameData->netManager, &address))
    {
        SDL_Log("Connected to server Successfully");
    }
    else
    {
        SDL_Log("Connection to server Failed");
        return 1;
    }

    // Request to join the game
    struct P_Add_Square joinGamePacket;
    if (Net_Join_Game(gameData->netManager, &joinGamePacket) == false)
    {
        SDL_Log("Connection to server Failed");
        return 1;
    }

    int playerId;
    if (!AddSquare(gameData->ec, &gameData->componentHandles, joinGamePacket.position, (SDL_FColor){1.0f, 1.0f, 1.0f, SDL_ALPHA_OPAQUE_FLOAT}, &playerId))
    {
        SDL_Log("Failed to create player, disconnecting");
        goto disconnect;
    }

    Net_Add_Networked_Entity(gameData->netManager, playerId, joinGamePacket.networkId);
    SDL_Log("Successfully joined at position %f,%f with network ID of %i", joinGamePacket.position.x,  joinGamePacket.position.y, joinGamePacket.networkId);

    // create a local copy of the player so we can see movement divergence
    int localPlayerCopy;
    if (AddSquare(gameData->ec, &gameData->componentHandles, joinGamePacket.position, (SDL_FColor){0.80f, 0.80f, 0.80f, SDL_ALPHA_OPAQUE_FLOAT}, &localPlayerCopy))
    {
        struct C_Input* entityInput = ECDB_EnableEntityComponent(gameData->ec, localPlayerCopy, gameData->componentHandles.inputs_handle);
        entityInput->speed=100;
    }
    else
    {
        SDL_Log("Failed to create player copy");
    }

    // create a red square above the player that is parented to show parenting works
    int childOfPlayer;
    if (ECDB_CreateEntity(gameData->ec, &childOfPlayer))
    {
        struct C_Transform* entityTransform = ECDB_EnableEntityComponent(gameData->ec, childOfPlayer, gameData->componentHandles.transforms_handle);
        entityTransform->position.x = joinGamePacket.position.x;
        entityTransform->position.y = joinGamePacket.position.y - 250;
        entityTransform->parent_id = localPlayerCopy;
        SDL_FColor* entityCol = ECDB_EnableEntityComponent(gameData->ec, childOfPlayer, gameData->componentHandles.colors_handle);
        SDL_FColor red = (SDL_FColor){0.89f, 0.494f, 0.38f, SDL_ALPHA_OPAQUE_FLOAT};
        memcpy(entityCol, &red, sizeof(SDL_FColor));
    }
    else
    {
        SDL_Log("couldnt create player child");
    }

    // Chat 
    char* chatInputMessageBuffer = NULL;
    chatInputMessageBuffer = calloc(CHAT_MAX_SIZE + 1, sizeof(char));
    unsigned int chatCursor = 0;

    char (*chatHistoryBuffer)[CHAT_MAX_SIZE] = NULL;
    chatHistoryBuffer = calloc(CHAT_HISTORY_SIZE, CHAT_MAX_SIZE * sizeof(char));
    int chatCount = 0;
    float previousChatBottom = 0;

    struct Vector2 direction = {.x = 0, .y = 0};
    SDL_Event e;
    Uint64 currentFrameTimeMs = SDL_GetTicks();
    Uint64 previousFrameTimeMs = currentFrameTimeMs;
    ENetEvent event;
    enum Command_Contex command_context = COMMAND_STANDARD;

    SDL_Log("Starting game loop");
    while(quit == false)
    {
        previousFrameTimeMs = currentFrameTimeMs;
        currentFrameTimeMs = SDL_GetTicks();
        float deltaTimeS = (float)(currentFrameTimeMs - previousFrameTimeMs) / 1000;

        // Get network events
        while (enet_host_service(gameData->netManager->client, &event, 0) > 0)
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
                            struct P_Update* packetData = (struct P_Update*) event.packet->data;
                            if (!gameData->netManager->validNetworkIds[packetData->networkId])
                            {
                                // if we don't know about the entity, add it
                                int entityId;
                                if (AddSquare(gameData->ec, &gameData->componentHandles, packetData->position, (SDL_FColor){0.5f, 0.5f, 0.5f, SDL_ALPHA_OPAQUE_FLOAT}, &entityId))
                                {
                                    Net_Add_Networked_Entity(gameData->netManager, entityId, packetData->networkId);
                                    SDL_Log("Player joined at position %f,%f with network ID of %i. Assigned to entity ID %i", packetData->position.x,  packetData->position.y, packetData->networkId, entityId);
                                }
                                else
                                {
                                    SDL_Log("Too many entities received from server. Disconnecting.");
                                    goto disconnect;
                                }
                            }

                            int localEntityId = gameData->netManager->networkIdEntityMap[packetData->networkId];
                            if(ECDB_EntityHasComponent(gameData->ec, localEntityId, gameData->componentHandles.transforms_handle))
                            {
                                struct C_Transform* actorPosition = (struct Vector2*)ECDB_GetEntityComponent(gameData->ec, localEntityId, gameData->componentHandles.transforms_handle);
                                actorPosition->position = packetData->position;
                            }

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
                        if (chatCount < CHAT_HISTORY_SIZE)
                        {
                            // Chat packets start with a header followed by the chat string
                            struct P_Chat_Header* header = (struct P_Chat_Header*)event.packet->data;
                            char* chatPointer = ((char*)event.packet->data) + sizeof(struct P_Chat_Header);
                            if (header->isServerMessage)
                            {
                                sprintf(chatHistoryBuffer[chatCount], "Server: %s", chatPointer);
                            }
                            else
                            {
                                sprintf(chatHistoryBuffer[chatCount], "Player %i: %s", header->networkId, chatPointer);
                            }
                            chatCount++; 
                        }

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
                gameData->netManager->connected = false;
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
                command_context = Handle_Chat_Input_Event(&e, chatInputMessageBuffer, &chatCursor, &charWritten);
                if (command_context != COMMAND_CHAT)
                {
                    // If we've stopped chatting, send the chat packet
                    int messageSize = (sizeof(char) * chatCursor) + 1; // Size is number of characters + the null termination character
                    ENetPacket* chatPacket = enet_packet_create(chatInputMessageBuffer, messageSize, ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(gameData->netManager->serverPeer, 1, chatPacket); // Send on channel 1 as the chat channel

                    // reset the buffer
                    chatCursor = 0;
                    chatInputMessageBuffer[0] =  '\0';

                    printf("\n");
                }
                else if (charWritten)
                {
                    // If still chatting, write the recent character to the console
                    printf("%c", chatInputMessageBuffer[chatCursor - 1]); // Chat cursor is always at current char + 1
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
            struct P_Input_Direction inputPacket = {.type = INPUT_DIRECTION, .networkId = gameData->netManager->entityNetworkIdMap[playerId], .direction = direction};
            ENetPacket * packet = enet_packet_create(&inputPacket, sizeof(struct P_Input_Direction), 0);
            enet_peer_send(gameData->netManager->serverPeer, 0, packet);
        }

        s_apply_input(gameData->ec, gameData->componentHandles.inputs_handle, direction);
        s_move(gameData->ec, gameData->componentHandles.transforms_handle, gameData->componentHandles.inputs_handle, deltaTimeS);
        s_render(gameData->ec, gameData->componentHandles.transforms_handle, gameData->componentHandles.colors_handle, gameData->renderer);

        // Chat box UI
        Clay_BeginLayout();
        CLAY(CLAY_ID("ChatParentContainer"), { .layout = { .sizing = { .width = CLAY_SIZING_PERCENT(0.5f), .height = CLAY_SIZING_GROW(0) }, .padding = {.left = 5, .right = 0, .top = 0, .bottom = 5 } , .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM}, .layoutDirection = CLAY_TOP_TO_BOTTOM}, .backgroundColor = {0,0,0,0} }) {
            CLAY(CLAY_ID("FullChatWindowContainer"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_PERCENT(0.5f) }, .childGap = 3, .layoutDirection = CLAY_TOP_TO_BOTTOM, .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM} }, .backgroundColor = { 50, 50, 50, 100 }
            }) {
                CLAY(CLAY_ID("ChatHistoryContainer"), {
                .clip = { .vertical = true, .childOffset = { Clay_GetScrollOffset().x, Clay_GetScrollOffset().y } }, .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .childGap = 0, .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM} }
                }) {
                    for (int i = 0; i < chatCount; ++i)
                    {
                        CLAY(CLAY_IDI("Chat", i), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .padding = CLAY_PADDING_ALL(5), .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER } } }) {
                            CLAY_TEXT(((Clay_String) { .length = strlen(chatHistoryBuffer[i]), .chars = chatHistoryBuffer[i] }), { .fontSize = 24, .textColor = {255, 255, 255, 255} });
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
                    CLAY_TEXT(((Clay_String) { .length = strlen(chatInputMessageBuffer), .chars = chatInputMessageBuffer }), { .fontSize = 24, .textColor = {255, 255, 255, 255} });
                }
            }
        }

        Clay_RenderCommandArray renderCommands = Clay_EndLayout(deltaTimeS);
        Clay_SDL3RendererData renderData = {.renderer = gameData->renderer, .textEngine = gameData->textEngine, .fonts = &gameData->font};
        SDL_Clay_RenderClayCommands(&renderData, &renderCommands);

        // Draw to screen
        SDL_RenderPresent(gameData->renderer);
    }

disconnect:
    Net_Disconnect(gameData->netManager);

cleanup:
    free(chatInputMessageBuffer);
    free(chatHistoryBuffer);

    Game_Data_Free(&gameData);

    return 0;
}
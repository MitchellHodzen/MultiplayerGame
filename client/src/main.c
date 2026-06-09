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

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define ENTITY_COUNT 100
#define CHAT_MAX_SIZE 100
#define CHAT_HISTORY_SIZE 50

struct Component_Handles
{
    int positions_handle;
    int colors_handle;
    int inputs_handle;
};

enum Command_Contex
{
    COMMAND_STANDARD,
    COMMAND_CHAT,
};

bool InitializeECDB(struct ECDB** ecdb, struct Component_Handles* componentHandles, unsigned int entityCount)
{
    if (!ECDB_Init(ecdb, entityCount, 3))
    {
        SDL_Log("Couldn't initialize component DB");
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct Vector2), &(componentHandles->positions_handle)))
    {
        SDL_Log("Couldn't initialize positions component");
        ECDB_Free(ecdb);
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(SDL_FColor), &(componentHandles->colors_handle)))
    {
        SDL_Log("Couldn't initialize colors component");
        ECDB_Free(ecdb);
        return false;
    }
    
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Input), &(componentHandles->inputs_handle)))
    {
        SDL_Log("Couldn't initialize input component");
        ECDB_Free(ecdb);
        return false;
    }
    return true;
}

bool InitializeSDL(SDL_Window** window, SDL_Renderer** renderer, TTF_TextEngine** textEngine, int screen_width, int screen_height)
{
    if (!SDL_SetAppMetadata("mygame", "1.0", "com.mygame"))
    {
        SDL_Log("Couldn't Set SDL Metadata: %s", SDL_GetError());
        return false;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return false;
    }

    if (!TTF_Init()) {
        SDL_Log("Couldn't initialize SDL_ttf: %s", SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer("gaem", screen_width, screen_height, SDL_WINDOW_RESIZABLE, window, renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    *textEngine = TTF_CreateRendererTextEngine(*renderer);
    if (*textEngine == NULL) {
        SDL_Log("Failed to create text engine from renderer: %s", SDL_GetError());
        // Clean up SDL
        SDL_DestroyRenderer(*renderer);
        *renderer = NULL;
        SDL_DestroyWindow(*window);
        *window = NULL;
        SDL_Quit();
        return false;
    }

    return true;
}

bool LoadFont(TTF_Font** font)
{
    *font = TTF_OpenFont("resources/fonts/PixelGamingRegular-d9w0g.ttf", 24);
    if (font == NULL) {
        SDL_Log("Failed to load font");
        return false;
    }

    return true;
}

bool AddSquare(struct ECDB* ec, struct Component_Handles* componentHandles, struct Vector2 position, SDL_FColor color, int* entityId)
{
    if (ECDB_CreateEntity(ec, entityId) == false)
    {
        SDL_Log("Couldn't create square");
        return false;
    }

    struct Vector2* entityPos = ECDB_EnableEntityComponent(ec, *entityId, componentHandles->positions_handle);
    memcpy(entityPos, &position, sizeof(struct Vector2));
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

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    TTF_TextEngine* textEngine = NULL;

    if (InitializeSDL(&window, &renderer, &textEngine, SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        SDL_Log("SDL Initialized Successfully");
    }
    else
    {
        SDL_Log("SDL Initialization Failed");
        return 1;
    }

    TTF_Font* font = NULL;
    if (LoadFont(&font))
    {
        SDL_Log("Font loaded successfully");
    }
    else
    {
        SDL_Log("Failed to load font");
        return 1;
    }

    // init UI
    uint64_t totalMemorySize = Clay_MinMemorySize();
    void* clayArena = malloc(totalMemorySize);
    Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, clayArena), (Clay_Dimensions) { SCREEN_WIDTH, SCREEN_HEIGHT }, (Clay_ErrorHandler) { LogClayErrors });
    Clay_SetMeasureTextFunction(SDL_MeasureText, font);

    struct ECDB* ec = NULL;
    struct Component_Handles componentHandles;

    if (InitializeECDB(&ec, &componentHandles, ENTITY_COUNT))
    {
        SDL_Log("ECDB Initialized Successfully");
    }
    else
    {
        SDL_Log("ECDB Initialization Failed");
        return 1;
    }

    if (enet_initialize() == 0)
    {
        SDL_Log("ENet Initialized Successfully");
    }
    else
    {
        SDL_Log("ENet Initialization Failed");
        return 1;
    }

    unsigned int * entityNetworkId = NULL;
    entityNetworkId = calloc(ENTITY_COUNT, sizeof(unsigned int));
    unsigned int * networkIdEntity = NULL;
    networkIdEntity = calloc(ENTITY_COUNT, sizeof(unsigned int));
    bool* validNetworkIds = NULL;
    validNetworkIds = calloc(ENTITY_COUNT, sizeof(bool));

    ENetAddress address;
    enet_address_set_host (&address, "localhost");
    address.port = 1234;
    struct Net_Manager* netManager;
    if (Net_Connect(&netManager, &address))
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
    if (Net_Join_Game(netManager, &joinGamePacket) == false)
    {
        SDL_Log("Connection to server Failed");
        return 1;
    }

    int playerId;
    if (!AddSquare(ec, &componentHandles, joinGamePacket.position, (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f}, &playerId))
    {
        SDL_Log("Failed to create player, disconnecting");
        goto disconnect;
    }

    entityNetworkId[playerId] = joinGamePacket.networkId;
    networkIdEntity[joinGamePacket.networkId] = playerId;
    validNetworkIds[joinGamePacket.networkId] = true;
    SDL_Log("Successfully joined at position %f,%f with network ID of %i", joinGamePacket.position.x,  joinGamePacket.position.y, joinGamePacket.networkId);

    // create a local copy of the player so we can see movement divergence
    int localPlayerCopy;
    if (AddSquare(ec, &componentHandles, joinGamePacket.position, (SDL_FColor){0.80f, 0.80f, 0.80f, 1.0f}, &localPlayerCopy))
    {
        struct C_Input* entityInput = ECDB_EnableEntityComponent(ec, localPlayerCopy, componentHandles.inputs_handle);
        entityInput->speed=100;
    }
    else
    {
        SDL_Log("Failed to create player copy");
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
                            struct P_Update* packetData = (struct P_Update*) event.packet->data;
                            if (!validNetworkIds[packetData->networkId])
                            {
                                // if we don't know about the entity, add it
                                int entityId;
                                if (AddSquare(ec, &componentHandles, packetData->position, (SDL_FColor){0.5f, 0.5f, 0.5f, 0.0f}, &entityId))
                                {
                                    entityNetworkId[entityId] = packetData->networkId;
                                    networkIdEntity[packetData->networkId] = entityId;
                                    validNetworkIds[packetData->networkId] = true;
                                    SDL_Log("Player joined at position %f,%f with network ID of %i. Assigned to entity ID %i", packetData->position.x,  packetData->position.y, packetData->networkId, entityId);
                                }
                                else
                                {
                                    SDL_Log("Too many entities received from server. Disconnecting.");
                                    goto disconnect;
                                }
                            }

                            int localEntityId = networkIdEntity[packetData->networkId];
                            if(ECDB_EntityHasComponent(ec, localEntityId, componentHandles.positions_handle))
                            {
                                struct Vector2* actorPosition = (struct Vector2*)ECDB_GetEntityComponent(ec, localEntityId, componentHandles.positions_handle);
                                *actorPosition = packetData->position;
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
                            unsigned int chatNetworkId = ((struct P_Chat_Header*)event.packet->data)->networkId;
                            char* chatPointer = ((char*)event.packet->data) + sizeof(struct P_Chat_Header);
                            sprintf(chatHistoryBuffer[chatCount], "Player %i: %s", chatNetworkId, chatPointer);
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
                command_context = Handle_Chat_Input_Event(&e, chatInputMessageBuffer, &chatCursor, &charWritten);
                if (command_context != COMMAND_CHAT)
                {
                    // If we've stopped chatting, send the chat packet
                    int messageSize = (sizeof(char) * chatCursor) + 1; // Size is number of characters + the null termination character
                    ENetPacket* chatPacket = enet_packet_create(chatInputMessageBuffer, messageSize, ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(netManager->serverPeer, 1, chatPacket); // Send on channel 1 as the chat channel

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
            struct P_Input_Direction inputPacket = {.type = INPUT_DIRECTION, .networkId = entityNetworkId[playerId], .direction = direction};
            ENetPacket * packet = enet_packet_create(&inputPacket, sizeof(struct P_Input_Direction), 0);
            enet_peer_send(netManager->serverPeer, 0, packet);
        }

        s_apply_input(ec, componentHandles.inputs_handle, direction);
        s_move(ec, componentHandles.positions_handle, componentHandles.inputs_handle, deltaTimeS);
        s_render(ec, componentHandles.positions_handle, componentHandles.colors_handle, renderer);

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
        Clay_SDL3RendererData renderData = {.renderer = renderer, .textEngine = textEngine, .fonts = &font};
        SDL_Clay_RenderClayCommands(&renderData, &renderCommands);

        // Draw to screen
        SDL_RenderPresent(renderer);
    }

disconnect:
    Net_Disconnect(netManager);

cleanup:
    free(entityNetworkId);
    free(networkIdEntity);
    free(validNetworkIds);
    free(chatInputMessageBuffer);
    free(chatHistoryBuffer);

    // Close up
    SDL_Log("free netmgr");
    Net_Free(&netManager);
    SDL_Log("enet deinit");
    enet_deinitialize();

    SDL_Log("free ecdb");
    ECDB_Free(&ec);

    SDL_Log("Free clay arena");
    free(clayArena);

    SDL_Log("freeing font");
    TTF_CloseFont(font);
    SDL_Log("destroy text engine");
    TTF_DestroyRendererTextEngine(textEngine);
    SDL_Log("destroy renderer");
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_Log("destroy window");
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_Log("sdl quit");
    SDL_Quit();
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <enet/enet.h>
#include <windows.h> 
#include <SDL3/SDL.H>
#include <SDL3/SDL_main.h>
#include "ecdb.h"
#include "vector2.h"
#include "system_movement.h"
#include "system_render.h"
#include "system_apply_input.h"
#include "intstack.h"
#include "component_input.h"
#include "packets.h"
#include "net_manager.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define ENTITY_COUNT 100

struct Component_Handles
{
    int positions_handle;
    int colors_handle;
    int inputs_handle;
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

bool InitializeSDL(SDL_Window** window, SDL_Renderer** renderer, int screen_width, int screen_height)
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

    if (!SDL_CreateWindowAndRenderer("gaem", screen_width, screen_height, SDL_WINDOW_RESIZABLE, window, renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool AddSquare(struct ECDB* ec, struct Component_Handles* componentHandles, struct Vector2 position, SDL_FColor color, float speed, int* entityId)
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
    struct C_Input* entityInput = ECDB_EnableEntityComponent(ec, *entityId, componentHandles->inputs_handle);
    entityInput->speed=speed;
    return true;
}

int main(int argc, char* args[])
{
    bool quit = false;
    bool connected = false;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (InitializeSDL(&window, &renderer, SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        SDL_Log("SDL Initialized Successfully");
    }
    else
    {
        SDL_Log("SDL Initialization Failed");
        return 1;
    }

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

    ENetAddress address;
    enet_address_set_host (&address, "localhost");
    address.port = 1234;
    struct Net_Manager* netManager;
    if (Net_Connect(&netManager, &address))
    {
        SDL_Log("Connected to server Successfully");
        connected = true;
    }
    else
    {
        SDL_Log("Connection to server Failed");
        return 1;
    }

    // Request to join the game
    SDL_Log("Attempting to join game");
    enum Packet_Type joinPacketType = REQUEST_JOIN;
    ENetPacket * request_join_packet = enet_packet_create(&joinPacketType, sizeof(enum Packet_Type), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(netManager->serverPeer, 0, request_join_packet);

    int playerId;
    bool joined = false;
    ENetEvent event;


    // Wait up to 5 seconds to join the server
    while (enet_host_service(netManager->client, &event, 5000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:;
            // look at the first field in the packet to see what type it is
            enum Packet_Type type = (enum Packet_Type) *(event.packet->data);
            switch(type)
            {
            case ADD_SQUARE:;
                struct P_Add_Square* packetData = (struct P_Add_Square*) event.packet->data;
                if (AddSquare(ec, &componentHandles, (struct Vector2){.x = packetData->position.x, .y = packetData->position.y}, (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f}, 100, &playerId))
                {
                    entityNetworkId[playerId] = packetData->networkId;
                    joined = true;
                    SDL_Log("Successfully joined at position %f,%f", packetData->position.x,  packetData->position.y);
                    enet_packet_destroy(event.packet);
                    goto game_joined;
                }

                SDL_Log("Failed to create player, disconnecting");
                enet_packet_destroy(event.packet);
                goto disconnect;
            default:
                printf ("Received non-join packet of type %i\n", type);
                break;
            }
    
            // Clean up the packet now that we're done using it.
            enet_packet_destroy(event.packet);
            break;
        
        case ENET_EVENT_TYPE_DISCONNECT:
            SDL_Log("Disconnected from the server.");
            goto cleanup;
        }
    }
    
    // Check if we successfully joined
    if (joined == false)
    {
        SDL_Log("Could not join game");
        goto disconnect;
    }

game_joined:;
    struct Vector2 direction = {.x = 0, .y = 0};
    SDL_Event e;
    Uint64 currentFrameTimeMs = SDL_GetTicks();
    Uint64 previousFrameTimeMs = currentFrameTimeMs;

    while( quit == false && connected == true)
    {
        previousFrameTimeMs = currentFrameTimeMs;
        currentFrameTimeMs = SDL_GetTicks();
        float deltaTimeS = (float)(currentFrameTimeMs - previousFrameTimeMs) / 1000;

        /*struct P_Add_Square addSquareData = {.type = ADD_SQUARE, .position = (struct Vector2){.x = 293.44, .y = 8.0}};
        ENetPacket * packet = enet_packet_create(&addSquareData, sizeof(struct P_Add_Square), 0);
        enet_peer_send(peer, 0, packet);*/

        // Get network events
        while (enet_host_service(netManager->client, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                SDL_Log("A packet of length %u containing %s was received from %s on channel %u.\n",
                        event.packet -> dataLength,
                        event.packet -> data,
                        event.peer -> data,
                        event.channelID);
        
                // Clean up the packet now that we're done using it.
                enet_packet_destroy(event.packet);
                
                break;
            
            case ENET_EVENT_TYPE_DISCONNECT:
                SDL_Log("Disconnected from the server.");
                connected = false;
            }
        }

        while( SDL_PollEvent( &e ) == true )
        {
            if( e.type == SDL_EVENT_QUIT )
            {
                quit = true;
            }
            else if( e.type == SDL_EVENT_KEY_DOWN && e.key.repeat == 0)
            {
                if( e.key.key == SDLK_W )
                {
                    direction.y--;
                }
                else if( e.key.key == SDLK_A )
                {
                    direction.x--;
                }
                else if( e.key.key == SDLK_S )
                {
                    direction.y++;
                }
                else if( e.key.key == SDLK_D )
                {
                    direction.x++;
                }
            }
            else if( e.type == SDL_EVENT_KEY_UP && e.key.repeat == 0)
            {
                if( e.key.key == SDLK_W )
                {
                    direction.y++;
                }
                else if( e.key.key == SDLK_A )
                {
                    direction.x++;
                }
                else if( e.key.key == SDLK_S )
                {
                    direction.y--;
                }
                else if( e.key.key == SDLK_D )
                {
                    direction.x--;
                }
            }
        }

        s_apply_input(ec, componentHandles.inputs_handle, direction);
        s_move(ec, componentHandles.positions_handle, componentHandles.inputs_handle, deltaTimeS);
        s_render(ec, componentHandles.positions_handle, componentHandles.colors_handle, renderer);
    }

disconnect:
    // Disconnect if connected
    if (connected == true)
    {
         SDL_Log("Disconnecting from server.");

        // Disconnect after exit is hit
        enet_peer_disconnect(netManager->serverPeer, 0);
        
        // Allow up to 3 seconds for the disconnect to succeed.
        while (enet_host_service(netManager->client, & event, 3000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy (event.packet);
                break;
        
            case ENET_EVENT_TYPE_DISCONNECT:
                SDL_Log("Disconnection succeeded.");
                goto cleanup;
            }
        }
        
        // We've arrived here, so the disconnect attempt didn't succeed yet. Force the connection down.
        SDL_Log("Disconnection failed, force leaving.");
        enet_peer_reset(netManager->serverPeer);
    }

cleanup:

    // Close up
    SDL_Log("free netmgr");
    Net_Free(&netManager);
    SDL_Log("enet deinit");
    enet_deinitialize();

    SDL_Log("free ecdb");
    ECDB_Free(&ec);
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
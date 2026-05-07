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
#include "intstack.h"
#include "component_input.h"
#include "packets.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define ENTITY_COUNT 100

struct ECDB* ec = NULL;
int positions_handle;
int colors_handle;
int inputs_handle;

bool InitializeECDB(unsigned int entityCount)
{
    if (!ECDB_Init(&ec, entityCount, 3))
    {
        SDL_Log("Couldn't initialize component DB");
        return false;
    }

    if (!ECDB_RegisterComponent(ec, sizeof(struct Vector2), &positions_handle))
    {
        SDL_Log("Couldn't initialize positions component");
        ECDB_Free(&ec);
        return false;
    }
    if (!ECDB_RegisterComponent(ec, sizeof(SDL_FColor), &colors_handle))
    {
        SDL_Log("Couldn't initialize colors component");
        ECDB_Free(&ec);
        return false;
    }
    if (!ECDB_RegisterComponent(ec, sizeof(struct C_Input), &inputs_handle))
    {
        SDL_Log("Couldn't initialize input component");
        ECDB_Free(&ec);
        return false;
    }
    return true;
}

bool AddSquare(struct Vector2 position, SDL_FColor color, float speed, int* entityId)
{
    if (ECDB_CreateEntity(ec, entityId) == false)
    {
        SDL_Log("Couldn't create square");
        return false;
    }

    struct Vector2* entityPos = ECDB_EnableEntityComponent(ec, *entityId, positions_handle);
    memcpy(entityPos, &position, sizeof(struct Vector2));
    SDL_FColor* entityCol = ECDB_EnableEntityComponent(ec, *entityId, colors_handle);
    memcpy(entityCol, &color, sizeof(SDL_FColor));
    struct C_Input* entityInput = ECDB_EnableEntityComponent(ec, *entityId, inputs_handle);
    entityInput->speed=speed;
    return true;
}

bool InitializeSDL(SDL_Window** window, SDL_Renderer** renderer)
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

    if (!SDL_CreateWindowAndRenderer("gaem", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE, window, renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    return true;
}

struct NetworkingThreadInput
{
    const bool* QuitFlag;
    int num;
    ENetHost* client;
};

DWORD WINAPI NetworkingThread(struct NetworkingThreadInput* input)
{
    SDL_Log("Connecting to server");
    ENetAddress address;
    ENetEvent event;
    ENetPeer *peer;
    
    enet_address_set_host (&address, "localhost");
    address.port = 1234;
    
    // Initiate the connection, allocating the two channels 0 and 1.
    peer = enet_host_connect(input->client, &address, 2, 0);    
    
    if (peer == NULL)
    {
        SDL_Log("No available peers for initiating an ENet connection.\n");
        return 1;
    }
    
    // Wait up to 5 seconds for the connection attempt to succeed.
    if (enet_host_service(input->client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        SDL_Log("Connection succeeded.");
        peer->data = "my special server";
    }
    else
    {
        // Either the 5 seconds are up or a disconnect event was received.
        SDL_Log("Connection failed.");
        enet_peer_reset(peer);
        return 1;
    }

    while(*(input->QuitFlag) == false)
    {
        input->num++;
        SDL_Log("hello world %i\n", input->num);

        struct P_Add_Square addSquareData = {.type = ADD_SQUARE, .position = (struct Vector2){.x = 293.44, .y = 8.0}};
        ENetPacket * packet = enet_packet_create(&addSquareData, sizeof(struct P_Add_Square), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, packet);

        while (enet_host_service(input->client, &event, 1000) > 0)
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
                goto done;
            }
        }
    }

    SDL_Log("Disconnecting from server.");

    // Disconnect after exit is hit
    enet_peer_disconnect(peer, 0);
    
    // Allow up to 3 seconds for the disconnect to succeed.
    while (enet_host_service(input->client, & event, 3000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy (event.packet);
            break;
    
        case ENET_EVENT_TYPE_DISCONNECT:
            SDL_Log("Disconnection succeeded.");
            goto done;
        }
    }
    
    // We've arrived here, so the disconnect attempt didn't succeed yet. Force the connection down.
    enet_peer_reset(peer);
    SDL_Log("Disconnection failed, force leaving.");

done:
    return 0;
}

int main(int argc, char* args[])
{
    bool quit = false;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (InitializeSDL(&window, &renderer))
    {
        SDL_Log("SDL Initialized Successfully");
    }
    else
    {
        SDL_Log("SDL Initialization Failed");
        return 1;
    }

    if (InitializeECDB(ENTITY_COUNT))
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

    // Create a client to receive messages from the server
    ENetHost* client;
    client = enet_host_create(NULL, 1, 2, 0, 0);
    if (client != NULL)
    {
        SDL_Log("Client Host Created Successfully");
    }
    else
    {
        SDL_Log("Client Host Creation Failed");
        return 1;
    }

    struct NetworkingThreadInput threadInput = {.client = client, .num = 0, .QuitFlag = &quit};
    HANDLE networkingThreadHandle = CreateThread(NULL, 0, NetworkingThread, &threadInput, 0, NULL);
    if (networkingThreadHandle == NULL)
    {
        SDL_Log("Networking Thread Initialization Failed");
        return 1;
    }

    int playerId;
    int secondSquareId;
    AddSquare((struct Vector2){.x = SCREEN_WIDTH / 2, .y = SCREEN_HEIGHT / 2}, (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f}, 100, &playerId);
    AddSquare((struct Vector2){.x = SCREEN_WIDTH / 2 - 200, .y = SCREEN_HEIGHT / 2 - 200}, (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f}, 50, &secondSquareId);
    struct Vector2 direction = {.x = 0, .y = 0};

    SDL_Event e;
    Uint64 currentFrameTimeMs = SDL_GetTicks();
    Uint64 previousFrameTimeMs = currentFrameTimeMs;

    while( quit == false )
    {
        previousFrameTimeMs = currentFrameTimeMs;
        currentFrameTimeMs = SDL_GetTicks();
        float deltaTimeS = (float)(currentFrameTimeMs - previousFrameTimeMs) / 1000;

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

        struct C_Input* inputs = (struct C_Input*) ec->_componentArrays[inputs_handle];
        for(unsigned int i = 0; i < ec->_maxEntities; ++i)
        {
            if(ECDB_EntityHasComponent(ec, i, inputs_handle))
            {
                inputs[i].direction.x = direction.x;
                inputs[i].direction.y = direction.y;
            }
        }

        // Clear previous render before drawing
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE ); // Black
        SDL_RenderClear(renderer);

        s_move(ec, positions_handle, inputs_handle, deltaTimeS);
        s_render(ec, positions_handle, colors_handle, renderer);

        // Draw to screen
        SDL_RenderPresent(renderer);
    }

    // Close up
    WaitForSingleObject(networkingThreadHandle, INFINITE);
    enet_host_destroy(client);
    enet_deinitialize();

    ECDB_Free(&ec);
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_Quit();

    printf("final thread value: %i\n", threadInput.num);
    return 0;
}
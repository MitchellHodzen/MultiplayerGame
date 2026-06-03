#include <stdio.h>
#include <enet/enet.h>
#include "ecdb.h"
#include "packets.h"
#include "vector2.h"
#include "system_movement.h"
#include "component_input.h"
#include <stdbool.h>
#include "system_reset_input.h"
#include "system_movement.h"
#include <windows.h>

#define MAX_CONNECTIONS 10
#define CHANNELS 2
#define ENTITY_COUNT 100
#define TICK_RATE_PER_S 60

struct Component_Handles
{
    int positions_handle;
    int inputs_handle;
};

bool InitializeECDB(struct ECDB** ecdb, struct Component_Handles* componentHandles, unsigned int entityCount)
{
    if (!ECDB_Init(ecdb, entityCount, 2))
    {
        printf("Couldn't initialize component DB\n");
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct Vector2), &(componentHandles->positions_handle)))
    {
        printf("Couldn't initialize positions component\n");
        ECDB_Free(ecdb);
        return false;
    }
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Input), &(componentHandles->inputs_handle)))
    {
        printf("Couldn't initialize input component\n");
        ECDB_Free(ecdb);
        return false;
    }

    return true;
}

bool PlayerAdd(struct ECDB* ec, int* playerId)
{
    if (ECDB_CreateEntity(ec, playerId) == false)
    {
        printf("Couldn't create player\n");
        return false;
    }

    return true;
}

void PlayerAddCharacter(struct ECDB* ec, struct Component_Handles* componentHandles, int playerId, struct Vector2 position, float speed)
{
    struct Vector2* entityPos = ECDB_EnableEntityComponent(ec, playerId, componentHandles->positions_handle);
    memcpy(entityPos, &position, sizeof(struct Vector2));
    struct C_Input* entityInput = ECDB_EnableEntityComponent(ec, playerId, componentHandles->inputs_handle);
    entityInput->speed=speed;
}

int main(int argc, char* args[])
{
    if (enet_initialize () != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return 1;
    }

    struct ECDB* ecdb = NULL;
    struct Component_Handles componentHandles;
    if (InitializeECDB(&ecdb, &componentHandles, ENTITY_COUNT))
    {
        printf("ECDB Initialized Successfully\n");
    }
    else
    {
        printf("ECDB Initialization Failed\n");
        return 1;
    }
    
    ENetAddress address;
    ENetHost* server;
    address.host = ENET_HOST_ANY;
    address.port = 1234;
    server = enet_host_create(&address, MAX_CONNECTIONS, CHANNELS, 0, 0);
    if (server == NULL)
    {
        fprintf (stderr, "An error occurred while trying to create an ENet server host.\n");
        return 1;
    }

    float targetMsPerFrame = (1.0f / ((float)TICK_RATE_PER_S )) * 1000.0f;
    printf("target ms per frame %f\n", targetMsPerFrame);
    ENetEvent event;
    DWORD currentFrameTimeMs = GetTickCount();
    DWORD previousFrameTimeMs = currentFrameTimeMs;
    while(1)
    {
        previousFrameTimeMs = currentFrameTimeMs;
        currentFrameTimeMs = GetTickCount();
        float deltaTimeS = (float)(currentFrameTimeMs - previousFrameTimeMs) / 1000;

        while (enet_host_service (server, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
                int* playerId = malloc(sizeof(int)); // TODO: Don't do mem allocation in the networking loop, pull this out
                if (PlayerAdd(ecdb, playerId))
                {
                    // If we create a player, add the data to the event peer data field for easier tracking
                    printf("Player created for client from %x:%u. Player ID: %i\n", event.peer->address.host, event.peer->address.port, *playerId);
                    event.peer -> data = playerId;
                }
                else
                {
                    // Player couldn't join, disconnect
                    printf("Coule not create player for client from %x:%u. Disconnecting\n", event.peer->address.host, event.peer->address.port);
                    event.peer -> data = NULL;
                    enet_peer_disconnect(event.peer, 0);
                }
        
                break;

            case ENET_EVENT_TYPE_RECEIVE:;
                // See if the peer is connected. if not, throw out the packet
                if (event.peer->data == NULL)
                {
                    printf("Peer data from %x:%u is null, skipping processing\n", event.peer->address.host, event.peer->address.port);
                }
                else
                {
                    int playerId = *(int*)event.peer->data;
                    // Channel 0 is general packets
                    switch(event.channelID)
                    {
                        case 0: // General packets
                        {
                            // look at the first field in the packet to see what type it is
                            enum Packet_Type type = (enum Packet_Type) *(event.packet->data);
                            switch(type)
                            {
                            case REQUEST_JOIN:
                            {
                                printf("Request to join from: %i\n", playerId);
                                struct Vector2 position = {.x = 293.44, .y = 350.0};
                                PlayerAddCharacter(ecdb, &componentHandles, playerId, position, 100);

                                // Send the join packet with the position information
                                struct P_Add_Square addSquareData = {.type = ADD_SQUARE, .position = position, .networkId  = playerId};
                                ENetPacket * packet = enet_packet_create(&addSquareData, sizeof(struct P_Add_Square), 0);
                                enet_peer_send(event.peer, 0, packet);
                                break;
                            }
                            case ADD_SQUARE:
                            {
                                struct P_Add_Square* packetData = (struct P_Add_Square*) event.packet->data;
                                printf ("Add square packet received. Position: (%f, %f)\n", packetData->position.x, packetData->position.y);
                                break;
                            }
                            case INPUT_DIRECTION:
                            {
                                struct P_Input_Direction* packetData = (struct P_Input_Direction*) event.packet->data;
                                // Apply input
                                if(ECDB_EntityHasComponent(ecdb, packetData->networkId, componentHandles.inputs_handle))
                                {
                                    struct C_Input* playerInput = (struct C_Input*)ECDB_GetEntityComponent(ecdb, packetData->networkId, componentHandles.inputs_handle);
                                    playerInput->direction = packetData->direction;
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
                            // allocate memory on the stack for our custom packet which is the size of the chat string + the chat header
                            int chatWithHeaderLength = event.packet->dataLength + sizeof(struct P_Chat_Header);
                            void* chatWithHeader = _malloca(chatWithHeaderLength);

                            // set the first P_Chat_Header bytes to the chat header
                            struct P_Chat_Header chatHeader = {.networkId = playerId};
                            *(struct P_Chat_Header*)chatWithHeader = chatHeader;

                            // Append the chat message to the end of the packet
                            char* chatPtr = ((char*)chatWithHeader) + sizeof(struct P_Chat_Header);
                            strcpy(chatPtr, event.packet->data);

                            // Build and broadcast the packet; at this point, chatWithHeader is the header followed by the chat string
                            ENetPacket * chatPacket = enet_packet_create(chatWithHeader, chatWithHeaderLength, ENET_PACKET_FLAG_RELIABLE);
                            enet_host_broadcast(server, 1, chatPacket);
                            break;
                        }
                        default:
                        {
                            printf("Message received from %i on unexpected channel %i\n", playerId, event.channelID);
                            break;
                        }
                    }
                }

                // Clean up the packet now that we're done using it.
                enet_packet_destroy (event.packet);
                break;
            
            case ENET_EVENT_TYPE_DISCONNECT:
                if (event.peer->data == NULL)
                {
                    printf("Peer from %x:%u disconnected\n", event.peer->address.host, event.peer->address.port);
                }
                else
                {
                    int playerId = *(int*)event.peer->data;
                    printf("Player %i disconnected.\n", playerId);
                    ECDB_DestroyEntity(ecdb, playerId);
                    // Reset the peer's client information.
                    free(event.peer->data);
                    event.peer -> data = NULL;
                }        
            }
        }

        // Sim loop
        s_move(ecdb, componentHandles.positions_handle, componentHandles.inputs_handle, deltaTimeS);
        struct Vector2* positions = (struct Vector2*) ecdb->_componentArrays[componentHandles.positions_handle];
        struct C_Input* inputs = (struct C_Input*) ecdb->_componentArrays[componentHandles.inputs_handle];

        // broadcast each players position
        for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
        {
            if(ECDB_EntityHasComponent(ecdb, i, componentHandles.positions_handle) && ECDB_EntityHasComponent(ecdb, i, componentHandles.inputs_handle))
            {
                if (inputs[i].direction.x != 0.0f || inputs[i].direction.y != 0.0f)
                {
                    printf("Player %i position: %f, %f\n", i, positions[i].x, positions[i].y);
                    struct P_Update inputPacket = {.type = UPDATE, .networkId = i, .position = positions[i]};
                    ENetPacket * packet = enet_packet_create(&inputPacket, sizeof(struct P_Update), 0);
                    enet_host_broadcast(server, 0, packet);
                }
            }
        }

        // TODO: bad way to cap tick rate, change it
        DWORD endFrameTimeMs = GetTickCount();
        DWORD simTimeMs = (endFrameTimeMs - currentFrameTimeMs);
        // Each frame should take targetMsPerFrame seconds
        float waitTimeMs = targetMsPerFrame - simTimeMs;
        Sleep(waitTimeMs);
    }

    enet_host_destroy(server);
    enet_deinitialize();
    return 0;
}
#include <stdio.h>
#include <enet/enet.h>
#include "ecdb.h"
#include "packets.h"
#include "vector2.h"
#include "system_movement.h"
#include <stdbool.h>

#define MAX_CONNECTIONS 10
#define CHANNELS 2
#define ENTITY_COUNT 100

struct Component_Handles
{
    int positions_handle;
};

bool InitializeECDB(struct ECDB** ecdb, struct Component_Handles* componentHandles, unsigned int entityCount)
{
    if (!ECDB_Init(ecdb, entityCount, 1))
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

void PlayerAddPosition(struct ECDB* ec, struct Component_Handles* componentHandles, int playerId, struct Vector2 position)
{
    struct Vector2* entityPos = ECDB_EnableEntityComponent(ec, playerId, componentHandles->positions_handle);
    memcpy(entityPos, &position, sizeof(struct Vector2));
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

    ENetEvent event;
    while(1)
    {
        while (enet_host_service (server, &event, 1000) > 0)
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
                    // look at the first field in the packet to see what type it is
                    enum Packet_Type type = (enum Packet_Type) *(event.packet->data);
                    switch(type)
                    {
                    case REQUEST_JOIN:;
                        printf("Request to join from: %i\n", playerId);
                        struct Vector2 position = {.x = 293.44, .y = 350.0};
                        PlayerAddPosition(ecdb, &componentHandles, playerId, position);

                        // Send the join packet with the position information
                        struct P_Add_Square addSquareData = {.type = ADD_SQUARE, .position = position};
                        ENetPacket * packet = enet_packet_create(&addSquareData, sizeof(struct P_Add_Square), 0);
                        enet_peer_send(event.peer, 0, packet);

                        break;
                    case ADD_SQUARE:;
                        struct P_Add_Square* packetData = (struct P_Add_Square*) event.packet->data;
                        printf ("Add square packet received. Position: (%f, %f)\n", packetData->position.x, packetData->position.y);
                        break;
                    default:
                        printf ("Some weird packet of type %i\n", type);
                        break;
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
                    event.peer -> data = NULL;
                }        
            }
        }
    }

    enet_host_destroy(server);
    enet_deinitialize();
    return 0;
}
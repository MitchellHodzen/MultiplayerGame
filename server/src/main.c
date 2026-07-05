#include <stdio.h>
#include <enet/enet.h>
#include "ecdb.h"
#include "packets.h"
#include "vector2.h"
#include "system_movement.h"
#include "component_input.h"
#include "component_physics_2d.h"
#include "system_physics.h"
#include <stdbool.h>
#include "system_reset_input.h"
#include "system_movement.h"
#include "component_transform.h"
#include <windows.h>

#define MAX_CONNECTIONS 10
#define CHANNELS 2
#define ENTITY_COUNT 100
#define TICK_PER_S 60
#define MAX_CHAT_LENGTH 100
#define TIME_SYNC_SEND_S 15
#define UPDATE_SEND_PER_S 10

struct Component_Handles
{
    int transforms_handle;
    int inputs_handle;
    int physics_2d_handle;
};

bool InitializeECDB(struct ECDB** ecdb, struct Component_Handles* componentHandles, unsigned int entityCount)
{
    if (!ECDB_Init(ecdb, entityCount, 3))
    {
        printf("Couldn't initialize component DB\n");
        return false;
    }

    struct C_Transform defaultTransform = {.parent_id = (*ecdb)->invalidEntityId};
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Transform), &(componentHandles->transforms_handle), &defaultTransform))
    {
        printf("Couldn't initialize positions component\n");
        ECDB_Free(ecdb);
        return false;
    }
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Input), &(componentHandles->inputs_handle), NULL))
    {
        printf("Couldn't initialize input component\n");
        ECDB_Free(ecdb);
        return false;
    }
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Physics_2d), &(componentHandles->physics_2d_handle), NULL))
    {
        printf("Couldn't initialize physics component\n");
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
    struct C_Transform* entityTransform = ECDB_EnableEntityComponent(ec, playerId, componentHandles->transforms_handle);
    entityTransform->position = position;
    struct C_Input* entityInput = ECDB_EnableEntityComponent(ec, playerId, componentHandles->inputs_handle);
    entityInput->speed=speed;
    struct C_Physics_2d* physics = ECDB_EnableEntityComponent(ec, playerId, componentHandles->physics_2d_handle);
    physics->friction = 25;
}

void BroadcastChatMessage(ENetHost* server, struct P_Chat_Header header, char* string, size_t strlen)
{
    // allocate memory on the stack for our custom packet which is the size of the chat string + the chat header
    int chatWithHeaderLength = strlen + sizeof(struct P_Chat_Header);
    void* chatWithHeader = _malloca(chatWithHeaderLength);

    // set the first P_Chat_Header bytes to the chat header
    *(struct P_Chat_Header*)chatWithHeader = header;

    // Append the chat message to the end of the packet
    char* chatPtr = ((char*)chatWithHeader) + sizeof(struct P_Chat_Header);
    strcpy(chatPtr, string);

    // Build and broadcast the packet; at this point, chatWithHeader is the header followed by the chat string
    ENetPacket * chatPacket = enet_packet_create(chatWithHeader, chatWithHeaderLength, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 1, chatPacket);
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

    float targetSecPerFrame = (1.0f / (float)TICK_PER_S );
    float targetSecPerUpdate = (1.0f / (float)UPDATE_SEND_PER_S);
    printf("target ms per frame %f. Target ms per update packet %f\n", targetSecPerFrame * 1000.0f, targetSecPerUpdate * 1000.0f);
    ENetEvent event;
    DWORD currentFrameTimeMs = GetTickCount();
    DWORD previousFrameTimeMs = currentFrameTimeMs;

    float time_packet_accumulator_s = 0;
    float sim_accumulator_s = 0;
    float update_packet_accumulator_s = 0;
    unsigned int current_tick = 0;
    while(1)
    {
        previousFrameTimeMs = currentFrameTimeMs;
        currentFrameTimeMs = GetTickCount();
        float deltaTimeS = (float)(currentFrameTimeMs - previousFrameTimeMs) / 1000;

        time_packet_accumulator_s += deltaTimeS;
        if (time_packet_accumulator_s > TIME_SYNC_SEND_S)
        {
            // Send a time sync packet to all connected users. TODO: Update to send on a per-user basis as needed
            struct P_Server_Time time = {.type = SERVER_TIME, .server_time_ms = currentFrameTimeMs};
            ENetPacket * packet = enet_packet_create(&time, sizeof(struct P_Server_Time), 0);
            enet_host_broadcast(server, 0, packet);

            // pull back the accumulator
            time_packet_accumulator_s -= TIME_SYNC_SEND_S;
        }

        // service enet outside of sim loop to ensure timely message ack
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

                                // Send the join packet
                                struct P_JOIN_SERVER joinServerData = {.type = JOIN_SERVER, .max_entities = ENTITY_COUNT, .max_chat_length = MAX_CHAT_LENGTH, .ticks_per_s = TICK_PER_S, .server_time_ms = currentFrameTimeMs, .network_id = playerId, .position = position };
                                ENetPacket * packet = enet_packet_create(&joinServerData, sizeof(struct P_JOIN_SERVER), ENET_PACKET_FLAG_RELIABLE);
                                enet_peer_send(event.peer, 0, packet);

                                // Send a chat message indicating a player has joined
                                struct P_Chat_Header chatHeader = {.isServerMessage = true, .messageImportance = MESSAGE_IMPORTANCE_LOW};
                                char joinedMessageBuffer[50];
                                int strlen = sprintf(joinedMessageBuffer, "Player %i has joined the game", playerId);
                                BroadcastChatMessage(server, chatHeader, joinedMessageBuffer, strlen + 1);
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
                            // Broadcast out the player's message
                            struct P_Chat_Header chatHeader = {.isServerMessage = false, .messageImportance = MESSAGE_IMPORTANCE_STANDARD, .networkId = playerId};
                            BroadcastChatMessage(server, chatHeader, event.packet->data, event.packet->dataLength);
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
                    // Send a chat message indicating a player has left
                    struct P_Chat_Header chatHeader = {.isServerMessage = true, .messageImportance = MESSAGE_IMPORTANCE_LOW};
                    char joinedMessageBuffer[50];
                    int strlen = sprintf(joinedMessageBuffer, "Player %i has left the game", playerId);
                    BroadcastChatMessage(server, chatHeader, joinedMessageBuffer, strlen + 1);
                    // Reset the peer's client information.
                    free(event.peer->data);
                    event.peer -> data = NULL;
                }        
            }
        }

        sim_accumulator_s += deltaTimeS;
        if (sim_accumulator_s > targetSecPerFrame)
        {
            // Run the sim
            s_update_physics(ecdb, componentHandles.physics_2d_handle, componentHandles.inputs_handle, deltaTimeS);
            s_apply_physics(ecdb, componentHandles.physics_2d_handle, componentHandles.transforms_handle, deltaTimeS);

            // pull back the accumulator
            sim_accumulator_s -= targetSecPerFrame;
        }

        update_packet_accumulator_s += deltaTimeS;
        if (update_packet_accumulator_s > targetSecPerUpdate)
        {
            struct C_Transform* transforms = (struct C_Transform*) ecdb->data.componentArrays[componentHandles.transforms_handle];
            // Generate update packet
            unsigned int entities_to_update = 0;

            // allocate memory on the stack for our custom packet which is the size of the chat string + the chat header. TODO: Too big for stack? make a dedicated malloced buffer before hand and re-use
            unsigned int max_update_packet_length = sizeof(struct P_Update_Header) + (ecdb->_maxEntities * sizeof(struct P_Update_Entity_Data));
            void* update_packet_memory = _malloca(max_update_packet_length);

            // Calculate the update buffer pointer by skipping ahead P_Update_Header size
            struct P_Update_Entity_Data* update_buffer_ptr = ((char*)update_packet_memory) + sizeof(struct P_Update_Header);

            // Write updates to the update buffer
            for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
            {
                if(ECDB_EntityHasComponent(ecdb, i, componentHandles.transforms_handle) && ECDB_EntityHasComponent(ecdb, i, componentHandles.inputs_handle))
                {
                    struct P_Update_Entity_Data data = {.networkId = i, .position = transforms[i].position};
                    update_buffer_ptr[entities_to_update] = data;
                    entities_to_update++;
                }
            }

            // set the first P_Update_Header bytes to the update header
            struct P_Update_Header update_header = {.type = UPDATE, .server_time_ms = currentFrameTimeMs, .updates_count = entities_to_update};
            *(struct P_Update_Header*)update_packet_memory = update_header;

            // calculate the actual packet length with the entities to update count
            unsigned int actual_update_packet_length = sizeof(struct P_Update_Header) + (entities_to_update * sizeof(struct P_Update_Entity_Data));

            // Build and broadcast the packet. TODO: make packet creation malloc free
            ENetPacket * update_packet = enet_packet_create(update_packet_memory, actual_update_packet_length, ENET_PACKET_FLAG_RELIABLE);

            enet_host_broadcast(server, 0, update_packet);

            // pull back the accumulator
            update_packet_accumulator_s -= targetSecPerUpdate;
        }

        // Increment tick
        current_tick++;
    }

    enet_host_destroy(server);
    enet_deinitialize();
    return 0;
}
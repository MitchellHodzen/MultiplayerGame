#include "net_manager.h"
#include "packets.h"
#include <enet/enet.h>
#include <stdbool.h>
#include "ecdb.h"

bool Net_Initialize(struct Net_Manager** netManager, struct ECDB* ecdb)
{
    *netManager = (struct Net_Manager*) malloc(sizeof(struct Net_Manager));
    if (*netManager == NULL)
    {
        // Couldnt instantiate net manager
        return false;
    }

    // Create a client to receive messages from the server with 1 outgoing connection, 2 channels, and unlimited incoming and outgoing bandwidth
    (*netManager)->client = enet_host_create(NULL, 1, 2, 0, 0);
    if ((*netManager)->client == NULL)
    {
        printf("Client Host Creation Failed\n");
        Net_Free(netManager);
        return false;
    }

    (*netManager)->entityNetworkIdMap = calloc(ecdb->_maxEntities, sizeof(unsigned int));
    (*netManager)->networkIdEntityMap = malloc(sizeof(unsigned int) * ecdb->_maxEntities);
    if ((*netManager)->entityNetworkIdMap == NULL || (*netManager)->networkIdEntityMap == NULL)
    {
        printf("Allocation of entity tracking data structures failed\n");
        Net_Free(netManager);
        return false;
    }

    // for each network ID, set the value to the invalid value
    for(unsigned int i = 0; i < ecdb->_maxEntities; i++)
    {
        (*netManager)->networkIdEntityMap[i] = ecdb->invalidEntityId;
    }

    return true;
}

bool Net_Try_Connect(struct Net_Manager* netManager, ENetAddress* address)
{
    printf("Connecting to server at %x:%u.\n", address->host, address->port);
    ENetEvent event;
    
    // Initiate the connection, allocating the two channels 0 and 1.
    netManager->serverPeer = enet_host_connect(netManager->client, address, 2, 0);    
    
    if (netManager->serverPeer == NULL)
    {
        printf("No available peers for initiating an ENet connection.\n");
        return false;
    }
    
    // Wait up to 5 seconds for the connection attempt to succeed.
    if (enet_host_service(netManager->client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        printf("Connection succeeded.\n");
        netManager->serverPeer->data = "my special server";
        netManager->connected = true;
        return true;
    }

    // Either the 5 seconds are up or a disconnect event was received.
    printf("Connection failed.\n");
    enet_peer_reset(netManager->serverPeer);
    return false;
}

bool Net_Join_Game(struct Net_Manager* netManager, struct P_Add_Square* output)
{
    // Request to join the game
    SDL_Log("Attempting to join game");
    enum Packet_Type joinPacketType = REQUEST_JOIN;
    ENetPacket * request_join_packet = enet_packet_create(&joinPacketType, sizeof(enum Packet_Type), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(netManager->serverPeer, 0, request_join_packet);

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
                output->type = packetData->type;
                output->networkId = packetData->networkId;
                output->position = packetData->position;
                enet_packet_destroy(event.packet);
                return true;

            default:
                printf ("Received non-join packet of type %i\n", type);
                break;
            }
    
            // Clean up the packet now that we're done using it.
            enet_packet_destroy(event.packet);
            break;
        
        case ENET_EVENT_TYPE_DISCONNECT:
            SDL_Log("Disconnected from the server.");
            netManager->connected = false;
            return false;
        }
    }

    // never received the join packet, return false
    return false;
}

void Net_Add_Networked_Entity(struct Net_Manager* netManager, unsigned int entityId, unsigned int networkId)
{
    netManager->entityNetworkIdMap[entityId] = networkId;
    netManager->networkIdEntityMap[networkId] = entityId;
}

void Net_Remove_Networked_Entity(struct Net_Manager* netManager, struct ECDB* ecdb, unsigned int entityId, unsigned int networkId)
{
    netManager->entityNetworkIdMap[entityId] = ecdb->invalidEntityId;
    netManager->networkIdEntityMap[networkId] = ecdb->invalidEntityId;
}

void Net_Disconnect(struct Net_Manager* netManager)
{
    // If we are connected, disconnect
    if (netManager->connected)
    {
        SDL_Log("Disconnecting from server.");
        enet_peer_disconnect(netManager->serverPeer, 0);
        ENetEvent event;
        
        // Allow up to 3 seconds for the disconnect to succeed.
        while (enet_host_service(netManager->client, &event, 3000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy (event.packet);
                break;
        
            case ENET_EVENT_TYPE_DISCONNECT:
                SDL_Log("Disconnection succeeded.");
                netManager->connected = false;
                return;
            }
        }
    }
    
    // if the disconnect attempt didn't succeed, force the connection down.
    SDL_Log("Disconnection failed, force leaving.");
    enet_peer_reset(netManager->serverPeer);
    netManager->connected = false;
}

void Net_Free(struct Net_Manager** netManager)
{
    free((*netManager)->entityNetworkIdMap);
    free((*netManager)->networkIdEntityMap);
    enet_peer_reset((*netManager)->serverPeer);
    enet_host_destroy((*netManager)->client);
    free(*netManager);
    *netManager = NULL;
}
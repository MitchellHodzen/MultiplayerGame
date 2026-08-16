#include "net_manager.h"
#include "packets.h"
#include <enet/enet.h>
#include <stdbool.h>
#include "ecdb.h"
#include <SDL3/SDL.H>

bool Net_Initialize(struct Net_Manager** netManager)
{
    // Initialize ENet
    if (enet_initialize() != 0)
    {
        SDL_Log("ENet Initialization Failed");
        return false;
    }

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
        SDL_Log("Client Host Creation Failed");
        Net_Free(netManager);
        return false;
    }

    return true;
}

bool Listen_For_Packet(struct Net_Manager* netManager, enet_uint32 timeoutMs, enum Packet_Type type, void* data)
{
    // Wait for specific packet for timeoutMs amount of time
    ENetEvent event;
    while (enet_host_service(netManager->client, &event, timeoutMs) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_RECEIVE:
            {
                // look at the first field in the packet to see what type it is
                enum Packet_Type received_type = (enum Packet_Type) *(event.packet->data);
                if (received_type == type)
                {
                    memcpy(data, event.packet->data, event.packet->dataLength);
                    enet_packet_destroy(event.packet);
                    return true;
                }
                else
                {
                    // Clean up packets we're not listening for
                    enet_packet_destroy(event.packet);
                    SDL_Log("Received packet of type %i while listening for %i", received_type, type);
                    break;
                }

                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                SDL_Log("Disconnected from the server.");
                netManager->connected = false;
                return false;
            }
        }
    }

    // never received the packet listening for, return false
    SDL_Log("Listen for packet of type %i timed out after %ims", type, timeoutMs);
    return false;
}

bool Net_Join_Server(struct Net_Manager* netManager, ENetAddress* address, struct P_JOIN_SERVER* output)
{
    SDL_Log("Connecting to server at %x:%u.", address->host, address->port);
    
    // Initiate the connection, allocating the two channels 0 and 1.
    netManager->serverPeer = enet_host_connect(netManager->client, address, 2, 0);    
    
    if (netManager->serverPeer == NULL)
    {
        SDL_Log("No available peers for initiating an ENet connection.");
        return false;
    }
    
    // Wait up to 5 seconds for the connection attempt to succeed.
    ENetEvent event;
    if (!(enet_host_service(netManager->client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT))
    {
        // Either the 5 seconds are up or a disconnect event was received.
        SDL_Log("Connection failed.");
        enet_peer_reset(netManager->serverPeer);
        return false;
    }

    SDL_Log("Connection succeeded.");
    netManager->serverPeer->data = "my special server";
    netManager->connected = true;

    // Request to join the game
    SDL_Log("Attempting to join game");
    enum Packet_Type joinPacketType = REQUEST_JOIN;
    ENetPacket * request_join_packet = enet_packet_create(&joinPacketType, sizeof(enum Packet_Type), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(netManager->serverPeer, 0, request_join_packet);

    // Wait for the join success packet
    if (Listen_For_Packet(netManager, 5000, JOIN_SERVER, (struct P_JOIN_SERVER*) output))
    {
        netManager->update_packets_per_s = output->update_packets_per_s;
        return true;
    }

    // never received the join packet, return false
    return false;
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

uint64_t Net_Estimate_Server_Time(const struct Net_Manager* netManager, uint64_t client_time_ms)
{
    return client_time_ms + netManager->server_time_offset_ms;
}


uint64_t Net_Estimate_Client_Time(const struct Net_Manager* netManager, uint64_t server_time_ms)
{
    return server_time_ms - netManager->server_time_offset_ms;
}

unsigned int Net_Get_Round_Trip_Time_Ms(const struct Net_Manager* netManager)
{
    return netManager->serverPeer->roundTripTime;
}

void Net_Calculate_Server_Time_Offset(struct Net_Manager* netManager, uint64_t client_time_ms, uint64_t server_time_ms)
{
    unsigned int half_round_trip_time = netManager->serverPeer->roundTripTime / 2;
    netManager->server_time_offset_ms = server_time_ms + half_round_trip_time - client_time_ms;
}


void Net_Free(struct Net_Manager** netManager)
{
    enet_peer_reset((*netManager)->serverPeer);
    enet_host_destroy((*netManager)->client);
    free(*netManager);
    *netManager = NULL;
}
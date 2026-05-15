#include "net_manager.h"
#include <enet/enet.h>
#include <stdbool.h>

bool Net_Connect(struct Net_Manager** netManager, ENetAddress* address)
{
    *netManager = (struct Net_Manager*) malloc(sizeof(struct Net_Manager));
    if (*netManager == NULL)
    {
        // Couldnt instantiate net manager
        return false;
    }

    // Create a client to receive messages from the server
    (*netManager)->client = enet_host_create(NULL, 1, 2, 0, 0);
    if ((*netManager)->client != NULL)
    {
        printf("Client Host Created Successfully\n");
    }
    else
    {
        printf("Client Host Creation Failed\n");
        Net_Free(netManager);
        return false;
    }

    printf("Connecting to server at %x:%u.\n", address->host, address->port);
    ENetEvent event;
    
    // Initiate the connection, allocating the two channels 0 and 1.
    (*netManager)->serverPeer = enet_host_connect((*netManager)->client, address, 2, 0);    
    
    if ((*netManager)->serverPeer == NULL)
    {
        printf("No available peers for initiating an ENet connection.\n");
        Net_Free(netManager);
        return false;
    }
    
    // Wait up to 5 seconds for the connection attempt to succeed.
    if (enet_host_service((*netManager)->client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        printf("Connection succeeded.\n");
        (*netManager)->serverPeer->data = "my special server";
        return true;
    }

    // Either the 5 seconds are up or a disconnect event was received.
    printf("Connection failed.\n");
    Net_Free(netManager);
    return false;
}

void Net_Free(struct Net_Manager** netManager)
{
    enet_peer_reset((*netManager)->serverPeer);
    enet_host_destroy((*netManager)->client);
    free(*netManager);
    *netManager = NULL;
}
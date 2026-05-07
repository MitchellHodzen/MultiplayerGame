#include <stdio.h>
#include <enet/enet.h>

#define MAX_CONNECTIONS 10
#define CHANNELS 2

int main(int argc, char* args[])
{
    if (enet_initialize () != 0)
    {
        fprintf (stderr, "An error occurred while initializing ENet.\n");
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
                printf ("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
        
                // Store any relevant client information here.
                event.peer -> data = "Client information";
        
                break;
        
            case ENET_EVENT_TYPE_RECEIVE:
                printf ("A packet of length %u containing %i was received from %s on channel %u.\n",
                        event.packet -> dataLength,
                        *(event.packet -> data),
                        event.peer -> data,
                        event.channelID);
        
                // Clean up the packet now that we're done using it.
                enet_packet_destroy (event.packet);
                break;
            
            case ENET_EVENT_TYPE_DISCONNECT:
                printf ("%s disconnected.\n", event.peer -> data);
        
                // Reset the peer's client information.
                event.peer -> data = NULL;
            }
        }
    }

    enet_host_destroy(server);
    enet_deinitialize();
    return 0;
}
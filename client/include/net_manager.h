#ifndef NET_MANAGER
#define NET_MANAGER
#include <stdbool.h>

struct _ENetHost;
struct _ENetPeer;
struct _ENetAddress;

struct Net_Manager
{
    struct _ENetHost* client;
    struct _ENetPeer* serverPeer;
};

bool Net_Connect(struct Net_Manager** netManager, struct _ENetAddress* address);
void Net_Free(struct Net_Manager** netManager);

#endif /* NET_MANAGER */
#ifndef NET_MANAGER
#define NET_MANAGER
#include <stdbool.h>

struct _ENetHost;
struct _ENetPeer;
struct _ENetAddress;
struct P_Add_Square;

struct Net_Manager
{
    struct _ENetHost* client;
    struct _ENetPeer* serverPeer;
    bool connected;
};

bool Net_Connect(struct Net_Manager** netManager, struct _ENetAddress* address);
bool Net_Join_Game(struct Net_Manager* netManager, struct P_Add_Square* output);
void Net_Disconnect(struct Net_Manager* netManager);
void Net_Free(struct Net_Manager** netManager);

#endif /* NET_MANAGER */
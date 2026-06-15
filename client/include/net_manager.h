#ifndef NET_MANAGER
#define NET_MANAGER
#include <stdbool.h>

struct _ENetHost;
struct _ENetPeer;
struct _ENetAddress;
struct P_JOIN_SERVER;

struct Net_Manager
{
    struct _ENetHost* client;
    struct _ENetPeer* serverPeer;
    bool connected;
};

bool Net_Initialize(struct Net_Manager** netManager);
bool Net_Try_Connect(struct Net_Manager* netManager, struct _ENetAddress* address);
bool Net_Join_Server(struct Net_Manager* netManager, struct P_JOIN_SERVER* output);
void Net_Disconnect(struct Net_Manager* netManager);
void Net_Free(struct Net_Manager** netManager);

#endif /* NET_MANAGER */
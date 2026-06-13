#ifndef NET_MANAGER
#define NET_MANAGER
#include <stdbool.h>

struct _ENetHost;
struct _ENetPeer;
struct _ENetAddress;
struct P_Add_Square;
struct ECDB;

struct Net_Manager
{
    struct _ENetHost* client;
    struct _ENetPeer* serverPeer;
    bool connected;
    unsigned int* entityNetworkIdMap;
    unsigned int* networkIdEntityMap;
};

bool Net_Initialize(struct Net_Manager** netManager, struct ECDB* ecdb);
bool Net_Try_Connect(struct Net_Manager* netManager, struct _ENetAddress* address);
bool Net_Join_Game(struct Net_Manager* netManager, struct P_Add_Square* output);
void Net_Add_Networked_Entity(struct Net_Manager* netManager, unsigned int entityId, unsigned int networkId);
void Net_Remove_Networked_Entity(struct Net_Manager* netManager, struct ECDB* ecdb, unsigned int entityId, unsigned int networkId);
void Net_Disconnect(struct Net_Manager* netManager);
void Net_Free(struct Net_Manager** netManager);

#endif /* NET_MANAGER */
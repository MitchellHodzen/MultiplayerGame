#ifndef NET_MANAGER
#define NET_MANAGER
#include <stdbool.h>
#include <stdint.h>

struct _ENetHost;
struct _ENetPeer;
struct _ENetAddress;
struct P_JOIN_SERVER;

struct Net_Manager
{
    struct _ENetHost* client;
    struct _ENetPeer* serverPeer;
    bool connected;
    int server_time_offset_ms;
    unsigned int update_packets_per_s;
};

bool Net_Initialize(struct Net_Manager** netManager);
bool Net_Join_Server(struct Net_Manager* netManager, struct _ENetAddress* address, struct P_JOIN_SERVER* output);
void Net_Disconnect(struct Net_Manager* netManager);
void Net_Receive(struct Net_Manager* netManager, bool (*on_recv[3])(struct Net_Manager* net_mgr_src, unsigned char* data, size_t data_len, void* callback_data), void* callback_data[3]);
void Net_Free(struct Net_Manager** netManager);
uint64_t Net_Estimate_Server_Time(const struct Net_Manager* netManager, uint64_t client_time_ms);
uint64_t Net_Estimate_Client_Time(const struct Net_Manager* netManager, uint64_t server_time_ms);
unsigned int Net_Get_Round_Trip_Time_Ms(const struct Net_Manager* netManager);
unsigned int Net_Get_Packet_Loss(const struct Net_Manager* netManager);
void Net_Calculate_Server_Time_Offset(struct Net_Manager* netManager, uint64_t client_time_ms, uint64_t server_time_ms);

#endif /* NET_MANAGER */
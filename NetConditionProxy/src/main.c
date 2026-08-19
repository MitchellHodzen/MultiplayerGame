#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>

#define DEFAULT_BUFLEN 512
#define PROXY_PORT "1234"
#define SERVER_IP "localhost"
#define SERVER_PORT "1235"
#define MAX_CLIENTS 10

bool Init_Entry_Socket(SOCKET* entry_socket, const char* port)
{
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // TODO: Support ipv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    struct addrinfo *addr_info = NULL;
    int iResult = getaddrinfo(NULL, port, &hints, &addr_info);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        return false;
    }

    // Create a SOCKET for the server to listen for client connections.
    *entry_socket = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (*entry_socket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(addr_info);
        return false;
    }

    // Setup the UDP listening socket
    iResult = bind( *entry_socket, addr_info->ai_addr, (int)addr_info->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(addr_info);
        closesocket(*entry_socket);
        *entry_socket = INVALID_SOCKET;
        return false;
    }

    freeaddrinfo(addr_info);
    return true;
}

bool Get_Server_Address_Info(const char* addr, const char* port, struct addrinfo **server_addr_info)
{
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // TODO: Support ipv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    //hints.ai_flags = AI_PASSIVE; // necessary?
    
    int res = getaddrinfo(addr, port, &hints, server_addr_info);
    if ( res != 0 ) {
        printf("Server getaddrinfo call failed: %d\n", res);
        return false;
    }

    return true;
}

bool Init_Client_Proxy_Socket(SOCKET* client_proxy_socket, const struct addrinfo *server_addr_info)
{
    // Create the socket
    *client_proxy_socket = socket(server_addr_info->ai_family, server_addr_info->ai_socktype, server_addr_info->ai_protocol);
    if (*client_proxy_socket == INVALID_SOCKET) {
        printf("Client proxy socket failed with error: %ld\n", WSAGetLastError());
        return false;
    }

    // Set the socket to non-blocking
    u_long iMode = 1;
    int res = ioctlsocket(*client_proxy_socket, FIONBIO, &iMode);
    if (res != NO_ERROR)
    {
        printf("client ioctlsocket failed with error: %ld\n", res);
        closesocket(*client_proxy_socket);
        *client_proxy_socket = INVALID_SOCKET;
        return false;
    }

    // Point it at the server we're proxying so we can just use rec and send
    res = connect(*client_proxy_socket, server_addr_info->ai_addr, (int)server_addr_info->ai_addrlen);
    if (res == SOCKET_ERROR)
    {
        printf("Client proxy socket failed to connect with error: %ld\n", WSAGetLastError());
        closesocket(*client_proxy_socket);
        *client_proxy_socket = INVALID_SOCKET;
        return false;
    }
    return true;
}

bool Sockaddr_storage_match(SOCKADDR_STORAGE* a, SOCKADDR_STORAGE* b)
{
    if (a->ss_family != b->ss_family)
    {
        return false;
    }

    if (a->__ss_align != b->__ss_align)
    {
        return false;
    }

    for (unsigned int i = 0; i < _SS_PAD1SIZE; ++i)
    {
        if (a->__ss_pad1[i] != b->__ss_pad1[i])
        {
            return false;
        }
    }

    for (unsigned int i = 0; i < _SS_PAD2SIZE; ++i)
    {
        if (a->__ss_pad2[i] != b->__ss_pad2[i])
        {
            return false;
        }
    }
    return true;
}

struct Client_Proxy_Socket_Map
{
    SOCKADDR_STORAGE from;
    int from_len;
    SOCKET client_proxy_socket;
};

int main(int argc, char* args[])
{
    WSADATA wsaData; //winsock data container
    int iResult;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    // Create socket which interacts with clients
    SOCKET entry_socket = INVALID_SOCKET;
    if (Init_Entry_Socket(&entry_socket, PROXY_PORT) == false)
    {
        printf("Unable to initialize proxy socket\n");
        WSACleanup();
        return 1;
    }

    // Build addrinfo about the server to route traffic to
    struct addrinfo *server_addr_info = NULL;
    if(Get_Server_Address_Info(SERVER_IP, SERVER_PORT, &server_addr_info) == false)
    {
        printf("Unable to get server address info for %s:%s\n", SERVER_IP, SERVER_PORT);
        closesocket(entry_socket);
        WSACleanup();
        return 1;
    }

    // make both sockets non-blocking
    u_long iMode = 1;
    iResult = ioctlsocket(entry_socket, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
    {
        printf("listener ioctlsocket failed with error: %ld\n", iResult);
    }
    
    struct Client_Proxy_Socket_Map client_proxy_maps[MAX_CLIENTS];
    int client_proxy_cnt = 0;

    int bytecount;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    SOCKADDR_STORAGE from;
    int fromlen;
    char servstr[NI_MAXSERV];
    char hoststr[NI_MAXHOST];
    while(1)
    {
        fromlen = sizeof(from);

        bytecount = recvfrom(entry_socket, recvbuf, recvbuflen, 0, (SOCKADDR *)&from, &fromlen);
        if (bytecount == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
            {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(entry_socket);
                WSACleanup();
                return 1;
            }
        }
        else
        {
            // Display the source of the datagram
            int retval = getnameinfo((SOCKADDR *)&from, fromlen, hoststr, NI_MAXHOST, servstr, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
            if (retval != 0)
            {
                fprintf(stderr, "getnameinfo failed: %d\n", retval);
                closesocket(entry_socket);
                WSACleanup();
                return 1;
            }
            
            struct Client_Proxy_Socket_Map* client_proxy_map = NULL;
            // Find the proxy associated with the client
            for(unsigned int i = 0; i < client_proxy_cnt; ++i)
            {
                if (Sockaddr_storage_match(&from, &(client_proxy_maps[i].from)))
                {
                    client_proxy_map = &client_proxy_maps[i];
                    break;
                }
            }

            if (client_proxy_map == NULL)
            {
                if (client_proxy_cnt == MAX_CLIENTS)
                {
                    printf("max clients reached\n");
                    closesocket(entry_socket);
                    WSACleanup();
                    return 1;
                }

                // if we cant find a client proxy for this client, create a new one
                printf("Received first packet from client at host %s and port %s\n", hoststr, servstr);
                client_proxy_map = &client_proxy_maps[client_proxy_cnt];
                client_proxy_map->client_proxy_socket = INVALID_SOCKET;
                if (Init_Client_Proxy_Socket(&(client_proxy_map->client_proxy_socket), server_addr_info) == false)
                {
                    printf("Unable to initialize client proxy socket\n");
                    closesocket(entry_socket);
                    WSACleanup();
                    return 1;
                }
                client_proxy_map->from = from;
                client_proxy_map->from_len = fromlen;
                client_proxy_cnt++;
            }
            else
            {
                printf("Received packet from client at host %s and port %s\n", hoststr, servstr);
            }

            printf("\tread %d bytes from host %s and port %s\n", bytecount, hoststr, servstr);

            // Send the packet to the server
            bytecount = send(client_proxy_map->client_proxy_socket, recvbuf, bytecount, 0);
            if (bytecount == SOCKET_ERROR)
            {
                fprintf(stderr, "Call to server failed: %d\n", WSAGetLastError());
                closesocket(entry_socket);
                WSACleanup();
                return 1;
            }

            printf("\tsent %d bytes to server\n", bytecount, hoststr, servstr);
        }


        // listen for response for any client
        for(unsigned int i = 0; i < client_proxy_cnt; ++i)
        {
            bytecount = recv(client_proxy_maps[i].client_proxy_socket, recvbuf, recvbuflen, 0);
            if (bytecount == SOCKET_ERROR)
            {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK)
                {
                    printf("recv from server failed with error: %d\n", WSAGetLastError());
                    closesocket(entry_socket);
                    WSACleanup();
                    return 1;
                }
            }
            else
            {
                printf("received packet from server\n");
                printf("\tread %d bytes from server\n", bytecount);

                // send response to original client
                bytecount = sendto(entry_socket, recvbuf, bytecount, 0, (SOCKADDR *)&(client_proxy_maps[i].from), client_proxy_maps[i].from_len);
                if (bytecount == SOCKET_ERROR)
                {
                    fprintf(stderr, "Call back to client failed: %d\n", WSAGetLastError());
                    closesocket(entry_socket);
                    WSACleanup();
                    return 1;
                }

                printf("\tsent %d bytes to host %s and port %s\n", bytecount, hoststr, servstr);
            }
        }

    }

    // TODO: Close client sockets


    // shutdown the connection since we're done
    iResult = shutdown(entry_socket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(entry_socket);
        WSACleanup();
        return 1;
    }

    // cleanup
    freeaddrinfo(server_addr_info);
    closesocket(entry_socket);
    WSACleanup();

    return 0;
}
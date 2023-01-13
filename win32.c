/** 
 @file  win32.c
 @brief ENet Win32 system specific functions
*/
#ifdef _WIN32

#define ENET_BUILDING_LIB 1
#include "enet/enet.h"
#include <windows.h>
#include <mmsystem.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>

static enet_uint32 timeBase = 0;

static int addressFamily[] = {
    AF_UNSPEC, //< ENET_ADDRESS_TYPE_ANY
    AF_INET,   //< ENET_ADDRESS_TYPE_IPV4
    AF_INET6   //< ENET_ADDRESS_TYPE_IPV6
};

static int 
enet_address_from_sock_addr4(ENetAddress * address, const struct sockaddr_in* sockAddr)
{
    address->type = ENET_ADDRESS_TYPE_IPV4;
    address->port = ENET_NET_TO_HOST_16(sockAddr->sin_port);

    address->host.v4[0] = sockAddr->sin_addr.S_un.S_un_b.s_b1;
    address->host.v4[1] = sockAddr->sin_addr.S_un.S_un_b.s_b2;
    address->host.v4[2] = sockAddr->sin_addr.S_un.S_un_b.s_b3;
    address->host.v4[3] = sockAddr->sin_addr.S_un.S_un_b.s_b4;

    return 0;
}

static int 
enet_address_from_sock_addr6(ENetAddress * address, const struct sockaddr_in6* sockAddr)
{
    int i;

    address->type = ENET_ADDRESS_TYPE_IPV6;
    address->port = ENET_NET_TO_HOST_16(sockAddr->sin6_port);

    for (i = 0; i < 8; ++i)
        address->host.v6[i] = ((enet_uint16) sockAddr->sin6_addr.s6_addr[i * 2]) << 8 | sockAddr->sin6_addr.s6_addr[i * 2 + 1];

    return 0;
}

static int 
enet_address_from_addr_info(ENetAddress * address, const struct addrinfo * info)
{
    switch (info->ai_family)
    {
        case AF_INET:
            return enet_address_from_sock_addr4(address, (struct sockaddr_in*) info->ai_addr);

        case AF_INET6:
            return enet_address_from_sock_addr6(address, (struct sockaddr_in6*) info->ai_addr);

        default:
            return -1;
    }
}

static int 
enet_address_from_sock_addr(ENetAddress * address, const struct sockaddr * sockAddr)
{
    switch (sockAddr->sa_family)
    {
        case AF_INET:
            return enet_address_from_sock_addr4(address, (struct sockaddr_in*) sockAddr);

        case AF_INET6:
            return enet_address_from_sock_addr6(address, (struct sockaddr_in6*) sockAddr);

        default:
            return -1;
    }
}

static int 
enet_address_to_sock_addr(const ENetAddress * address, void * sockAddr)
{
    switch (address->type)
    {
        case ENET_ADDRESS_TYPE_IPV4:
        {
            struct sockaddr_in* socketAddress = (struct sockaddr_in*) sockAddr;
            int addr;

            memset(socketAddress, 0, sizeof(struct sockaddr_in));
            socketAddress->sin_family = AF_INET;
            socketAddress->sin_port = ENET_HOST_TO_NET_16(address->port);

            addr = ((unsigned int) address->host.v4[0]) << 24
                 | ((unsigned int) address->host.v4[1]) << 16
                 | ((unsigned int) address->host.v4[2]) <<  8
                 | ((unsigned int) address->host.v4[3]) <<  0;

            socketAddress->sin_addr.s_addr = htonl(addr);

            return sizeof(struct sockaddr_in);
        }

        case ENET_ADDRESS_TYPE_IPV6:
        {
            struct sockaddr_in6* socketAddress = (struct sockaddr_in6*) sockAddr;
            int i;

            memset(socketAddress, 0, sizeof(struct sockaddr_in6));
            socketAddress->sin6_family = AF_INET6;
            socketAddress->sin6_port = ENET_HOST_TO_NET_16(address->port);

            for (i = 0; i < 8; ++i)
            {
                u_short addressPart = ENET_HOST_TO_NET_16(address->host.v6[i]);
                socketAddress->sin6_addr.s6_addr[i * 2 + 0] = addressPart >> 0;
                socketAddress->sin6_addr.s6_addr[i * 2 + 1] = addressPart >> 8;
            }

            return sizeof(struct sockaddr_in6);
        }

        default:
            return 0;
    }
}

int
enet_initialize (void)
{
    WORD versionRequested = MAKEWORD (1, 1);
    WSADATA wsaData;
   
    if (WSAStartup (versionRequested, & wsaData))
       return -1;

    if (LOBYTE (wsaData.wVersion) != 1||
        HIBYTE (wsaData.wVersion) != 1)
    {
       WSACleanup ();
       
       return -1;
    }

    timeBeginPeriod (1);

    return 0;
}

void
enet_deinitialize (void)
{
    timeEndPeriod (1);

    WSACleanup ();
}

enet_uint32
enet_host_random_seed (void)
{
    return (enet_uint32) timeGetTime ();
}

enet_uint32
enet_time_get (void)
{
    return (enet_uint32) timeGetTime () - timeBase;
}

void
enet_time_set (enet_uint32 newTimeBase)
{
    timeBase = (enet_uint32) timeGetTime () - newTimeBase;
}

int
enet_address_set_host(ENetAddress * address, ENetAddressType type, const char * name)
{
    struct addrinfo hints;
    struct addrinfo* result;
    struct addrinfo* resultList = NULL;
    enet_uint16 port;
    ENetAddress tempAddress;
    int bestScore = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;

    if (getaddrinfo(name, NULL, &hints, &resultList) != 0)
        return -1;

    port = address->port; /* preserve port */

    for (result = resultList; result != NULL; result = result->ai_next)
    {
        if (result->ai_addr != NULL)
        {
            if (enet_address_from_addr_info (&tempAddress, result) == 0)
            {
                tempAddress.port = port; /* preserve port */

                int addressScore = 0;
                if (type == ENET_ADDRESS_TYPE_ANY)
                {
                    if (tempAddress.type == ENET_ADDRESS_TYPE_IPV6)
                        addressScore += 10;
                    else if (tempAddress.type == ENET_ADDRESS_TYPE_IPV4)
                    {
                        /* ANY basically means IPv6 with support for IPv4, but we still have to map our IPv4 to IPv6 */
                        addressScore += 5;
                        enet_address_convert_ipv6(&tempAddress);
                    }
                }
                else if (tempAddress.type == type)
                    addressScore += 10;

                if (addressScore > bestScore)
                {
                    memcpy(address, &tempAddress, sizeof(ENetAddress));
                    bestScore = addressScore;
                }
            }
        }
    }

    if (resultList != NULL)
        freeaddrinfo(resultList);

    if (bestScore >= 0)
        return 0;
    else
    {
        if (enet_address_set_host_ip(address, name) == 0)
        {
            if (type == ENET_ADDRESS_TYPE_ANY)
                enet_address_convert_ipv6(address);

            return 0;
        }
        else
            return -1;
    }
}

int
enet_address_get_host (const ENetAddress * address, char * name, size_t nameLength)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)];
    int socketAddressLen = enet_address_to_sock_addr(address, sockAddrBuf);

    int result = getnameinfo((struct sockaddr*) sockAddrBuf, socketAddressLen, name, nameLength, NULL, 0, NI_NAMEREQD);
    if (result != 0)
        return enet_address_get_host_ip (address, name, nameLength);
    else
        return 0;
}

int
enet_socket_bind (ENetSocket socket, const ENetAddress * address)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)];
    int socketAddressLen = enet_address_to_sock_addr(address, sockAddrBuf);

    return bind (socket, (struct sockaddr *) sockAddrBuf, socketAddressLen) == SOCKET_ERROR ? -1 : 0;
}

int
enet_socket_get_address (ENetSocket socket, ENetAddress * address)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)] = { 0 };
    int bufferLength;

    if (getsockname (socket, (struct sockaddr *) sockAddrBuf, &bufferLength) == -1)
      return -1;

    return enet_address_from_sock_addr(address, (struct sockaddr *) sockAddrBuf);
}

int
enet_socket_listen (ENetSocket socket, int backlog)
{
    return listen (socket, backlog < 0 ? SOMAXCONN : backlog) == SOCKET_ERROR ? -1 : 0;
}

ENetSocket
enet_socket_create (ENetAddressType addressType, ENetSocketType socketType)
{
    return socket (addressType == ENET_ADDRESS_TYPE_IPV4 ? PF_INET : PF_INET6, socketType == ENET_SOCKET_TYPE_DATAGRAM ? SOCK_DGRAM : SOCK_STREAM, 0);
}

int
enet_socket_set_option (ENetSocket socket, ENetSocketOption option, int value)
{
    int result = SOCKET_ERROR;
    switch (option)
    {
        case ENET_SOCKOPT_NONBLOCK:
        {
            u_long nonBlocking = (u_long) value;
            result = ioctlsocket (socket, FIONBIO, & nonBlocking);
            break;
        }

        case ENET_SOCKOPT_BROADCAST:
            result = setsockopt (socket, SOL_SOCKET, SO_BROADCAST, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_REUSEADDR:
            result = setsockopt (socket, SOL_SOCKET, SO_REUSEADDR, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_RCVBUF:
            result = setsockopt (socket, SOL_SOCKET, SO_RCVBUF, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_SNDBUF:
            result = setsockopt (socket, SOL_SOCKET, SO_SNDBUF, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_RCVTIMEO:
            result = setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_SNDTIMEO:
            result = setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_NODELAY:
            result = setsockopt (socket, IPPROTO_TCP, TCP_NODELAY, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_TTL:
            result = setsockopt (socket, IPPROTO_IP, IP_TTL, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_IPV6ONLY:
        {
            DWORD option = value;
            result = setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) & value, sizeof(option));
            break;
        }

        default:
            break;
    }
    return result == SOCKET_ERROR ? -1 : 0;
}

int
enet_socket_get_option (ENetSocket socket, ENetSocketOption option, int * value)
{
    int result = SOCKET_ERROR, len;
    switch (option)
    {
        case ENET_SOCKOPT_ERROR:
            len = sizeof(int);
            result = getsockopt (socket, SOL_SOCKET, SO_ERROR, (char *) value, & len);
            break;

        case ENET_SOCKOPT_TTL:
            len = sizeof(int);
            result = getsockopt (socket, IPPROTO_IP, IP_TTL, (char *) value, & len);
            break;

        default:
            break;
    }
    return result == SOCKET_ERROR ? -1 : 0;
}

int
enet_socket_connect (ENetSocket socket, const ENetAddress * address)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)];
    int socketAddressLen = enet_address_to_sock_addr(address, sockAddrBuf);
    int result;

    result = connect (socket, (struct sockaddr*) sockAddrBuf, socketAddressLen);
    if (result == SOCKET_ERROR && WSAGetLastError () != WSAEWOULDBLOCK)
      return -1;

    return 0;
}

ENetSocket
enet_socket_accept (ENetSocket socket, ENetAddress * address)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)] = { 0 };
    int socketAddressLen = sizeof(sockAddrBuf);
    SOCKET result;

    result = accept (socket, 
                     address != NULL ? (struct sockaddr*) sockAddrBuf : NULL,
                     address != NULL ? & socketAddressLen : NULL);

    if (result == INVALID_SOCKET)
      return ENET_SOCKET_NULL;

    if (address != NULL)
    {
        if (enet_address_from_sock_addr(address, (struct sockaddr*) sockAddrBuf) != 0)
            return ENET_SOCKET_NULL;
    }

    return result;
}

int
enet_socket_shutdown (ENetSocket socket, ENetSocketShutdown how)
{
    return shutdown (socket, (int) how) == SOCKET_ERROR ? -1 : 0;
}

void
enet_socket_destroy (ENetSocket socket)
{
    if (socket != INVALID_SOCKET)
      closesocket (socket);
}

int
enet_socket_send (ENetSocket socket,
                  const ENetAddress * address,
                  const ENetBuffer * buffers,
                  size_t bufferCount)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)];
    int socketAddressLen;

    DWORD sentLength = 0;

    if (address != NULL)
    {
        socketAddressLen = enet_address_to_sock_addr(address, sockAddrBuf);
        if (socketAddressLen == 0)
            return -1;
    }

    if (WSASendTo (socket, 
                   (LPWSABUF) buffers,
                   (DWORD) bufferCount,
                   & sentLength,
                   0,
                   address != NULL ? (struct sockaddr *) sockAddrBuf : NULL,
                   address != NULL ? socketAddressLen : 0,
                   NULL,
                   NULL) == SOCKET_ERROR)
    {
       if (WSAGetLastError() == WSAEWOULDBLOCK)
         return 0;

       return -1;
    }

    return (int) sentLength;
}

int
enet_socket_receive (ENetSocket socket,
                     ENetAddress * address,
                     ENetBuffer * buffers,
                     size_t bufferCount)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)] = { 0 };
    int socketAddressLen = sizeof(sockAddrBuf);
    DWORD flags = 0,
          recvLength = 0;
    struct sockaddr_in sin;

    if (WSARecvFrom (socket,
                     (LPWSABUF) buffers,
                     (DWORD) bufferCount,
                     & recvLength,
                     & flags,
                     address != NULL ? (struct sockaddr *) & sockAddrBuf : NULL,
                     address != NULL ? & socketAddressLen : NULL,
                     NULL,
                     NULL) == SOCKET_ERROR)
    {
       switch (WSAGetLastError())
       {
       case WSAEWOULDBLOCK:
       case WSAECONNRESET:
          return 0;
       }

       return -1;
    }

    if (flags & MSG_PARTIAL)
      return -1;

    if (address != NULL)
    {
        if (enet_address_from_sock_addr(address, (struct sockaddr*) sockAddrBuf) != 0)
            return -1;
    }

    return (int) recvLength;
}

int
enet_socketset_select (ENetSocket maxSocket, ENetSocketSet * readSet, ENetSocketSet * writeSet, enet_uint32 timeout)
{
    struct timeval timeVal;

    timeVal.tv_sec = timeout / 1000;
    timeVal.tv_usec = (timeout % 1000) * 1000;

    return select (maxSocket + 1, readSet, writeSet, NULL, & timeVal);
}

int
enet_socket_wait (ENetSocket socket, enet_uint32 * condition, enet_uint32 timeout)
{
    fd_set readSet, writeSet;
    struct timeval timeVal;
    int selectCount;
    
    timeVal.tv_sec = timeout / 1000;
    timeVal.tv_usec = (timeout % 1000) * 1000;
    
    FD_ZERO (& readSet);
    FD_ZERO (& writeSet);

    if (* condition & ENET_SOCKET_WAIT_SEND)
      FD_SET (socket, & writeSet);

    if (* condition & ENET_SOCKET_WAIT_RECEIVE)
      FD_SET (socket, & readSet);

    selectCount = select (socket + 1, & readSet, & writeSet, NULL, & timeVal);

    if (selectCount < 0)
      return -1;

    * condition = ENET_SOCKET_WAIT_NONE;

    if (selectCount == 0)
      return 0;

    if (FD_ISSET (socket, & writeSet))
      * condition |= ENET_SOCKET_WAIT_SEND;
    
    if (FD_ISSET (socket, & readSet))
      * condition |= ENET_SOCKET_WAIT_RECEIVE;

    return 0;
} 

#endif


/**
 @file  win32.c
 @brief ENet Win32 system specific functions
*/
#ifdef _WIN32

#include <time.h>
#define ENET_BUILDING_LIB 1
#include "enet/enet.h"

static enet_uint32 timeBase = 0;

static inline size_t
enet_address_get_size (const ENetAddress * address)
{
    switch (address -> family)
    {
        case AF_INET:  return (sizeof (struct sockaddr_in));
        case AF_INET6: return (sizeof (struct sockaddr_in6));
    };
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
enet_time_get (void)
{
    return (enet_uint32) timeGetTime () - timeBase;
}

void
enet_time_set (enet_uint32 newTimeBase)
{
    timeBase = (enet_uint32) timeGetTime () - newTimeBase;
}

const static struct addrinfo hints = {
  /*.ai_flags     =*/ AI_PASSIVE, // For wildcard IP address
  /*.ai_family    =*/ AF_UNSPEC,  // Allow IPv4 or IPv6
  /*.ai_socktype  =*/ SOCK_DGRAM, // Datagram socket
  /*.ai_protocol  =*/ 0,          // Any protocol
  /*.ai_addrlen   =*/ 0,
  /*.ai_addr      =*/ NULL,
  /*.ai_canonname =*/ NULL,
  /*.ai_next      =*/ NULL,
};

int
enet_address_set_host (ENetAddress * address, const char * name)
{
    struct addrinfo * result_box;
    int error_code;

    error_code = getaddrinfo(name, NULL, &hints, &result_box);
    if (error_code != 0) return error_code;
    if (result_box == NULL) return -1;

    memcopy(&address, result_box -> ai_addr, result_box -> ai_addrlen);
    address -> port = ENET_NET_TO_HOST_16 (address -> port);
    return error_code;
}

int
enet_address_get_host_ip (const ENetAddress * address, char * name, size_t nameLength)
{
    void * host_ptr;
    switch (address -> family) {
        case AF_INET:
             host_ptr = & address -> ip.v4.host;
             break;
        case AF_INET6:
             host_ptr = & address -> ip.v6.host;
             break;
        default:
             host_ptr == NULL; // avoid wild pointer
    }
    return (inet_ntop (address -> family, host_ptr, name, nameLength) == NULL) ? -1 : 0;
}

int
enet_address_get_host (const ENetAddress * address, char * name, size_t nameLength)
{
    int error_code = getnameinfo((struct sockaddr *) address, enet_address_get_size(address),
                                 name, nameLength,
                                 NULL, 0,   // disregard service/socket name
                                 NI_DGRAM); // lookup via UPD when different

    // return IP address if name lookup is unsuccessful
    return (error_code == 0) ? 0 : enet_address_get_host_ip (address, name, nameLength);
}

int
enet_socket_bind (ENetSocket socket, const ENetAddress * address)
{
    const size_t length = enet_address_get_size (address);
    ENetAddress * clone;

    memcopy (& clone, address, length);
    clone -> port = ENET_HOST_TO_NET_16 (address -> port);

    return bind (socket, (struct sockaddr *) clone, length);
}

int
enet_socket_get_address (ENetSocket socket, ENetAddress * address)
{
    int length = sizeof (struct sockaddr_in6);

    if (getsockname (socket, (struct sockaddr *) & address, & length) == -1)
      return -1;

    address -> port = ENET_NET_TO_HOST_16 (address -> port);

    return 0;
}

int
enet_socket_listen (ENetSocket socket, int backlog)
{
    return listen (socket, backlog < 0 ? SOMAXCONN : backlog) == SOCKET_ERROR ? -1 : 0;
}

ENetSocket
enet_socket_create (ENetSocketType type)
{
    return socket (PF_INET, type == ENET_SOCKET_TYPE_DATAGRAM ? SOCK_DGRAM : SOCK_STREAM, 0);
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

        default:
            break;
    }
    return result == SOCKET_ERROR ? -1 : 0;
}

int
enet_socket_connect (ENetSocket socket, const ENetAddress * address)
{
    int result;
    size_t length = enet_address_get_size (address);
    ENetAddress * clone;

    memcopy (& clone, address, length);
    clone -> port = ENET_HOST_TO_NET_16 (address -> port);

    result = connect (socket, (struct sockaddr *) & clone, length);

    if (result == SOCKET_ERROR && WSAGetLastError () != WSAEWOULDBLOCK)
      return -1;

    return 0;
}

ENetSocket
enet_socket_accept (ENetSocket socket, ENetAddress * address)
{
    SOCKET result;
    // only call enet_address_get_size if ptr is valid
    int length = address != NULL ? enet_address_get_size (address) : 0;

    result = accept (socket,
                     address != NULL ? (struct sockaddr *) & address : NULL,
                     address != NULL ? & length : NULL);
    if (result == INVALID_SOCKET)
      return ENET_SOCKET_NULL;

    if (address != NULL)
        address -> port = ENET_NET_TO_HOST_16 (address -> port);

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
    ENetAddress address_clone;
    int address_length = enet_address_get_size (address);
    DWORD sentLength;

    if (address != NULL)
    {
        memcopy (& address_clone, address, address_length);

        address_clone.port = ENET_HOST_TO_NET_16 (address -> port);
    }

    if (WSASendTo (socket,
                   (LPWSABUF) buffers,
                   (DWORD) bufferCount,
                   & sentLength,
                   0,
                   address != NULL ? (struct sockaddr *) & address_clone : NULL,
                   address != NULL ? address_length : 0,
                   NULL,
                   NULL) == SOCKET_ERROR)
    {
       if (WSAGetLastError () == WSAEWOULDBLOCK)
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
    INT length = sizeof (struct sockaddr_in6);
    DWORD flags = 0,
          recvLength;

    if (WSARecvFrom (socket,
                     (LPWSABUF) buffers,
                     (DWORD) bufferCount,
                     & recvLength,
                     & flags,
                     address != NULL ? (struct sockaddr *) address : NULL,
                     address != NULL ? & length : NULL,
                     NULL,
                     NULL) == SOCKET_ERROR)
    {
       switch (WSAGetLastError ())
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
        address -> port = ENET_NET_TO_HOST_16 (address -> port);

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

/** 
 @file  unix.c
 @brief ENet Unix system specific functions
*/
#ifndef _WIN32

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define ENET_BUILDING_LIB 1
#include "enet/enet.h"

#ifdef __APPLE__
#ifdef HAS_POLL
#undef HAS_POLL
#endif
#ifndef HAS_FCNTL
#define HAS_FCNTL 1
#endif
#ifndef HAS_INET_PTON
#define HAS_INET_PTON 1
#endif
#ifndef HAS_INET_NTOP
#define HAS_INET_NTOP 1
#endif
#ifndef HAS_MSGHDR_FLAGS
#define HAS_MSGHDR_FLAGS 1
#endif
#ifndef HAS_SOCKLEN_T
#define HAS_SOCKLEN_T 1
#endif
#ifndef HAS_GETADDRINFO
#define HAS_GETADDRINFO 1
#endif
#ifndef HAS_GETNAMEINFO
#define HAS_GETNAMEINFO 1
#endif
#endif

#ifdef HAS_FCNTL
#include <fcntl.h>
#endif

#ifdef HAS_POLL
#include <poll.h>
#endif

#if !defined(HAS_SOCKLEN_T) && !defined(__socklen_t_defined)
typedef int socklen_t;
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

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

    memcpy(&address->host.v4[0], &sockAddr->sin_addr.s_addr, 4 * sizeof(enet_uint8));

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
    return 0;
}

void
enet_deinitialize (void)
{
}

enet_uint32
enet_host_random_seed (void)
{
    return (enet_uint32) time (NULL);
}

enet_uint32
enet_time_get (void)
{
    struct timeval timeVal;

    gettimeofday (& timeVal, NULL);

    return timeVal.tv_sec * 1000 + timeVal.tv_usec / 1000 - timeBase;
}

void
enet_time_set (enet_uint32 newTimeBase)
{
    struct timeval timeVal;

    gettimeofday (& timeVal, NULL);
    
    timeBase = timeVal.tv_sec * 1000 + timeVal.tv_usec / 1000 - newTimeBase;
}

int
enet_address_set_host (ENetAddress * address, ENetAddressType type, const char * name)
{
#ifdef HAS_GETADDRINFO
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
            if (enet_address_from_addr_info(&tempAddress, result) == 0)
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
#else
    struct hostent * hostEntry = NULL;
#ifdef HAS_GETHOSTBYNAME_R
    struct hostent hostData;
    char buffer [2048];
    int errnum;

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__) || defined(__GNU__)
    gethostbyname_r (name, & hostData, buffer, sizeof (buffer), & hostEntry, & errnum);
#else
    hostEntry = gethostbyname_r (name, & hostData, buffer, sizeof (buffer), & errnum);
#endif
#else
    hostEntry = gethostbyname (name);
#endif

    /* TODO */
    /*if (hostEntry != NULL && hostEntry -> h_addrtype == AF_INET)
    {
        address -> host = * (enet_uint32 *) hostEntry -> h_addr_list [0];

        return 0;
    }*/
#endif

    if (enet_address_set_host_ip(address, name) == 0)
    {
        if (type == ENET_ADDRESS_TYPE_ANY)
            enet_address_convert_ipv6(address);

        return 0;
    }
    else
        return -1;
}

int
enet_address_get_host (const ENetAddress * address, char * name, size_t nameLength)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)];
    int socketAddressLen = enet_address_to_sock_addr(address, sockAddrBuf);
#ifdef HAS_GETNAMEINFO
    int err;

    err = getnameinfo ((struct sockaddr *) sockAddrBuf, socketAddressLen, name, nameLength, NULL, 0, NI_NAMEREQD);
    if (! err)
    {
        if (name != NULL && nameLength > 0 && ! memchr (name, '\0', nameLength))
          return -1;

        return 0;
    }
    if (err != EAI_NONAME)
      return -1;
#else
    struct in_addr in;
    struct hostent * hostEntry = NULL;
#ifdef HAS_GETHOSTBYADDR_R
    struct hostent hostData;
    char buffer [2048];
    int errnum;

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__) || defined(__GNU__)
    gethostbyaddr_r ((char *) sockAddrBuf, socketAddressLen, addressFamily[address->type], & hostData, buffer, sizeof (buffer), & hostEntry, & errnum);
#else
    hostEntry = gethostbyaddr_r ((char *) sockAddrBuf, socketAddressLen, addressFamily[address->type], & hostData, buffer, sizeof (buffer), & errnum);
#endif
#else
    hostEntry = gethostbyaddr ((char *) sockAddrBuf, socketAddressLen, addressFamily[address->type]);
#endif

    if (hostEntry != NULL)
    {
       size_t hostLen = strlen (hostEntry -> h_name);
       if (hostLen >= nameLength)
         return -1;
       memcpy (name, hostEntry -> h_name, hostLen + 1);
       return 0;
    }
#endif

    return enet_address_get_host_ip (address, name, nameLength);
}

int
enet_socket_bind (ENetSocket socket, const ENetAddress * address)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)];
    int socketAddressLen = enet_address_to_sock_addr(address, sockAddrBuf);

    return bind(socket, (struct sockaddr *) sockAddrBuf, socketAddressLen);
}

int
enet_socket_get_address (ENetSocket socket, ENetAddress * address)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)] = { 0 };
    int bufferLength;

    if (getsockname(socket, (struct sockaddr *) sockAddrBuf, &bufferLength) == -1)
        return -1;

    return enet_address_from_sock_addr(address, (struct sockaddr *) sockAddrBuf);
}

int 
enet_socket_listen (ENetSocket socket, int backlog)
{
    return listen (socket, backlog < 0 ? SOMAXCONN : backlog);
}

ENetSocket
enet_socket_create (ENetAddressType addressType, ENetSocketType socketType)
{
    return socket(addressType == ENET_ADDRESS_TYPE_IPV4 ? PF_INET : PF_INET6, socketType == ENET_SOCKET_TYPE_DATAGRAM ? SOCK_DGRAM : SOCK_STREAM, 0);
}

int
enet_socket_set_option (ENetSocket socket, ENetSocketOption option, int value)
{
    int result = -1;
    switch (option)
    {
        case ENET_SOCKOPT_NONBLOCK:
#ifdef HAS_FCNTL
            result = fcntl (socket, F_SETFL, (value ? O_NONBLOCK : 0) | (fcntl (socket, F_GETFL) & ~O_NONBLOCK));
#else
            result = ioctl (socket, FIONBIO, & value);
#endif
            break;

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
        {
            struct timeval timeVal;
            timeVal.tv_sec = value / 1000;
            timeVal.tv_usec = (value % 1000) * 1000;
            result = setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *) & timeVal, sizeof (struct timeval));
            break;
        }

        case ENET_SOCKOPT_SNDTIMEO:
        {
            struct timeval timeVal;
            timeVal.tv_sec = value / 1000;
            timeVal.tv_usec = (value % 1000) * 1000;
            result = setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *) & timeVal, sizeof (struct timeval));
            break;
        }

        case ENET_SOCKOPT_NODELAY:
            result = setsockopt (socket, IPPROTO_TCP, TCP_NODELAY, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_TTL:
            result = setsockopt (socket, IPPROTO_IP, IP_TTL, (char *) & value, sizeof (int));
            break;

        case ENET_SOCKOPT_IPV6ONLY:
            result = setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) & value, sizeof(int));
            break;

        default:
            break;
    }
    return result == -1 ? -1 : 0;
}

int
enet_socket_get_option (ENetSocket socket, ENetSocketOption option, int * value)
{
    int result = -1;
    socklen_t len;
    switch (option)
    {
        case ENET_SOCKOPT_ERROR:
            len = sizeof (int);
            result = getsockopt (socket, SOL_SOCKET, SO_ERROR, value, & len);
            break;

        case ENET_SOCKOPT_TTL:
            len = sizeof (int);
            result = getsockopt (socket, IPPROTO_IP, IP_TTL, (char *) value, & len);
            break;

        default:
            break;
    }
    return result == -1 ? -1 : 0;
}

int
enet_socket_connect (ENetSocket socket, const ENetAddress * address)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)];
    int socketAddressLen = enet_address_to_sock_addr(address, sockAddrBuf);
    int result;

    result = connect(socket, (struct sockaddr*) sockAddrBuf, socketAddressLen);
    if (result == -1 && errno == EINPROGRESS)
      return 0;

    return result;
}

ENetSocket
enet_socket_accept (ENetSocket socket, ENetAddress * address)
{
    int result;
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)] = { 0 };
    int socketAddressLen = sizeof(sockAddrBuf);

    result = accept (socket, 
                     address != NULL ? (struct sockaddr*) sockAddrBuf : NULL,
                     address != NULL ? & socketAddressLen : NULL);
    
    if (result == -1)
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
    return shutdown (socket, (int) how);
}

void
enet_socket_destroy (ENetSocket socket)
{
    if (socket != -1)
      close (socket);
}

int
enet_socket_send (ENetSocket socket,
                  const ENetAddress * address,
                  const ENetBuffer * buffers,
                  size_t bufferCount)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)];
    struct msghdr msgHdr;
    int sentLength;

    memset (& msgHdr, 0, sizeof (struct msghdr));

    if (address != NULL)
    {
        msgHdr.msg_namelen = enet_address_to_sock_addr(address, sockAddrBuf);
        if (msgHdr.msg_namelen == 0)
            return -1;

        msgHdr.msg_name = (struct sockaddr *) sockAddrBuf;
    }

    msgHdr.msg_iov = (struct iovec *) buffers;
    msgHdr.msg_iovlen = bufferCount;

    sentLength = sendmsg (socket, & msgHdr, MSG_NOSIGNAL);
    
    if (sentLength == -1)
    {
       if (errno == EWOULDBLOCK)
         return 0;

       return -1;
    }

    return sentLength;
}

int
enet_socket_receive (ENetSocket socket,
                     ENetAddress * address,
                     ENetBuffer * buffers,
                     size_t bufferCount)
{
    unsigned char sockAddrBuf[sizeof(struct sockaddr_in6)] = { 0 };
    int socketAddressLen = sizeof(sockAddrBuf);
    struct msghdr msgHdr;
    struct sockaddr_in sin;
    int recvLength;

    memset (& msgHdr, 0, sizeof (struct msghdr));

    if (address != NULL)
    {
        msgHdr.msg_name = (struct sockaddr*) &sockAddrBuf;
        msgHdr.msg_namelen = socketAddressLen;
    }

    msgHdr.msg_iov = (struct iovec *) buffers;
    msgHdr.msg_iovlen = bufferCount;

    recvLength = recvmsg (socket, & msgHdr, MSG_NOSIGNAL);

    if (recvLength == -1)
    {
       if (errno == EWOULDBLOCK)
         return 0;

       return -1;
    }

#ifdef HAS_MSGHDR_FLAGS
    if (msgHdr.msg_flags & MSG_TRUNC)
      return -1;
#endif

    if (address != NULL)
    {
        if (enet_address_from_sock_addr(address, (struct sockaddr*) sockAddrBuf) != 0)
            return -1;
    }

    return recvLength;
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
#ifdef HAS_POLL
    struct pollfd pollSocket;
    int pollCount;
    
    pollSocket.fd = socket;
    pollSocket.events = 0;

    if (* condition & ENET_SOCKET_WAIT_SEND)
      pollSocket.events |= POLLOUT;

    if (* condition & ENET_SOCKET_WAIT_RECEIVE)
      pollSocket.events |= POLLIN;

    pollCount = poll (& pollSocket, 1, timeout);

    if (pollCount < 0)
    {
        if (errno == EINTR && * condition & ENET_SOCKET_WAIT_INTERRUPT)
        {
            * condition = ENET_SOCKET_WAIT_INTERRUPT;

            return 0;
        }

        return -1;
    }

    * condition = ENET_SOCKET_WAIT_NONE;

    if (pollCount == 0)
      return 0;

    if (pollSocket.revents & POLLOUT)
      * condition |= ENET_SOCKET_WAIT_SEND;
    
    if (pollSocket.revents & POLLIN)
      * condition |= ENET_SOCKET_WAIT_RECEIVE;

    return 0;
#else
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
    {
        if (errno == EINTR && * condition & ENET_SOCKET_WAIT_INTERRUPT)
        {
            * condition = ENET_SOCKET_WAIT_INTERRUPT;

            return 0;
        }
      
        return -1;
    }

    * condition = ENET_SOCKET_WAIT_NONE;

    if (selectCount == 0)
      return 0;

    if (FD_ISSET (socket, & writeSet))
      * condition |= ENET_SOCKET_WAIT_SEND;

    if (FD_ISSET (socket, & readSet))
      * condition |= ENET_SOCKET_WAIT_RECEIVE;

    return 0;
#endif
}

#endif


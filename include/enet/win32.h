/** 
 @file  win32.h
 @brief ENet Win32 header
*/
#ifndef __ENET_WIN32_H__
#define __ENET_WIN32_H__

#include <stdlib.h>
#include <winsock2.h>

typedef SOCKET ENetSocket;

enum
{
    ENET_SOCKET_NULL = INVALID_SOCKET
};

#define ENET_HOST_TO_NET_16(value) (htons (value))
#define ENET_HOST_TO_NET_32(value) (htonl (value))

#define ENET_NET_TO_HOST_16(value) (ntohs (value))
#define ENET_NET_TO_HOST_32(value) (ntohl (value))

typedef struct
{
    size_t dataLength;
    void * data;
} ENetBuffer;

#define ENET_CALLBACK __cdecl

#if defined ENET_DLL
#if defined ENET_BUILDING_LIB
#define ENET_API __declspec( dllexport )
#else
#define ENET_API __declspec( dllimport )
#endif /* ENET_BUILDING_LIB */
#else /* !ENET_DLL */
#define ENET_API extern
#endif /* ENET_DLL */

#endif /* __ENET_WIN32_H__ */



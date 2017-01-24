/** 
 @file  win32.h
 @brief ENet Win32 header
*/
#ifndef __ENET_WIN32_H__
#define __ENET_WIN32_H__

#ifdef _MSC_VER
#ifdef ENET_BUILDING_LIB
#pragma warning (disable: 4267) // size_t to int conversion
#pragma warning (disable: 4244) // 64bit to 32bit int
#pragma warning (disable: 4018) // signed/unsigned mismatch
#pragma warning (disable: 4146) // unary minus operator applied to unsigned type
#pragma warning (disable: 4142) // benign redefinition of type
#endif
#endif

#include <stdlib.h>

#ifdef ENET_BUILDING_LIB
#include <winsock2.h>
#endif

// If these typedefs don't match what's in winsock2.h, compilation will fail.
#ifdef _WIN64
typedef __int64 SOCKET;
#else
typedef __int32 SOCKET;
#endif
typedef SOCKET ENetSocket;

#define ENET_SOCKET_NULL INVALID_SOCKET

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

#ifdef ENET_DLL
#ifdef ENET_BUILDING_LIB
#define ENET_API __declspec( dllexport )
#else
#define ENET_API __declspec( dllimport )
#endif /* ENET_BUILDING_LIB */
#else /* !ENET_DLL */
#define ENET_API extern
#endif /* ENET_DLL */

#ifdef ENET_BUILDING_LIB
typedef fd_set ENetSocketSet;
#else
// This struct will never be defined; it can only be used for pointers.
typedef struct ENetSocketSet ENetSocketSet;
#endif

#define ENET_SOCKETSET_EMPTY(sockset)          FD_ZERO (& (sockset))
#define ENET_SOCKETSET_ADD(sockset, socket)    FD_SET (socket, & (sockset))
#define ENET_SOCKETSET_REMOVE(sockset, socket) FD_CLR (socket, & (sockset))
#define ENET_SOCKETSET_CHECK(sockset, socket)  FD_ISSET (socket, & (sockset))

#endif /* __ENET_WIN32_H__ */



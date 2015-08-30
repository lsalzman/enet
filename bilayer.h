#ifndef BILAYER_H
#define BILAYER_H

#include "enet/enet.h"

ENET_API ENetTransport enet_socket_create_transport(ENetSocketType, ENetSocket);

ENET_API ENetHost * enet_host_create_bilayer (ENetTransportType type, const ENetAddress *, size_t, size_t, enet_uint32, enet_uint32);
ENET_API ENetHost * enet_host_create_notp (const ENetAddress *, size_t, size_t, enet_uint32, enet_uint32);
ENET_API void enet_host_transport(ENetHost *host, const ENetTransport *transport);

#endif /* BILAYER_H */

#include "enet/enet.h"

#define TYPE_OFFSET(s, m) ((size_t) & (((s *)0)->m))
#define TYPE_SIZE(ID, VAL) if (typeinfo_id == (ID)) return (VAL)


size_t enet_interophelper_sizeoroffset(enet_uint32 typeinfo_id)
{
    // Add offsets you need below and keep the id unique.
    TYPE_SIZE(0,  sizeof(ENetSocket));
    TYPE_SIZE(1,  sizeof(ENetHost));
    TYPE_SIZE(2,  TYPE_OFFSET(ENetHost, checksum));
    TYPE_SIZE(3,  TYPE_OFFSET(ENetHost, intercept));
    TYPE_SIZE(4,  TYPE_OFFSET(ENetHost, totalSentData));
    TYPE_SIZE(5,  TYPE_OFFSET(ENetHost, totalSentPackets));
    TYPE_SIZE(6,  TYPE_OFFSET(ENetHost, totalReceivedPackets));
    TYPE_SIZE(7,  TYPE_OFFSET(ENetHost, connectedPeers));
    TYPE_SIZE(8,  TYPE_OFFSET(ENetHost, duplicatePeers));
    TYPE_SIZE(9,  TYPE_OFFSET(ENetHost, peerCount));
    TYPE_SIZE(10, TYPE_OFFSET(ENetHost, peers));
    TYPE_SIZE(11, TYPE_OFFSET(ENetHost, mtu));
    TYPE_SIZE(12, TYPE_OFFSET(ENetHost, totalReceivedData));

    return -1;
}
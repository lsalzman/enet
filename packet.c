/** 
 @file  packet.c
 @brief ENet packet management functions
*/
#include <string.h>
#define ENET_BUILDING_LIB 1
#include "enet/enet.h"

/** @defgroup Packet ENet packet functions 
    @{ 
*/

/** Creates a packet that may be sent to a peer.
    @param dataContents initial contents of the packet's data; the packet's data will remain uninitialized if dataContents is NULL.
    @param dataLength   size of the data allocated for this packet
    @param flags        flags for this packet as described for the ENetPacket structure.
    @returns the packet on success, NULL on failure
*/
ENetPacket *
enet_packet_create (const void * data, size_t dataLength, enet_uint32 flags)
{
    ENetPacket * packet = (ENetPacket *) enet_malloc (sizeof (ENetPacket));

    packet -> data = (enet_uint8 *) enet_malloc (dataLength);

    if (data != NULL)
      memcpy (packet -> data, data, dataLength);

    packet -> referenceCount = 0;
    packet -> flags = flags;
    packet -> dataLength = dataLength;

    return packet;
}

/** Destroys the packet and deallocates its data.
    @param packet packet to be destroyed
*/
void
enet_packet_destroy (ENetPacket * packet)
{
    enet_free (packet -> data);
    enet_free (packet);
}

/** Attempts to resize the data in the packet to length specified in the 
    dataLength parameter 
    @param packet packet to resize
    @param dataLength new size for the packet data
    @returns 0 on success, < 0 on failure
*/
int
enet_packet_resize (ENetPacket * packet, size_t dataLength)
{
    enet_uint8 * newData;
   
    if (dataLength <= packet -> dataLength)
    {
       packet -> dataLength = dataLength;

       return 0;
    }

    newData = (enet_uint8 *) enet_malloc (dataLength);
    memcpy (newData, packet -> data, packet -> dataLength);
    enet_free (packet -> data);
    
    packet -> data = newData;
    packet -> dataLength = dataLength;

    return 0;
}

/** @} */

/** 
 @file  protocol.h
 @brief ENet protocol
*/
#ifndef __ENET_PROTOCOL_H__
#define __ENET_PROTOCOL_H__

#include "enet/types.h"

enum
{
   ENET_PROTOCOL_MINIMUM_MTU             = 576,
   ENET_PROTOCOL_MAXIMUM_MTU             = 4096,
   ENET_PROTOCOL_MAXIMUM_PACKET_COMMANDS = 32,
   ENET_PROTOCOL_MINIMUM_WINDOW_SIZE     = 4096,
   ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE     = 32768,
   ENET_PROTOCOL_MINIMUM_CHANNEL_COUNT   = 1,
   ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT   = 255
};

typedef enum
{
   ENET_PROTOCOL_COMMAND_NONE               = 0,
   ENET_PROTOCOL_COMMAND_ACKNOWLEDGE        = 1,
   ENET_PROTOCOL_COMMAND_CONNECT            = 2,
   ENET_PROTOCOL_COMMAND_VERIFY_CONNECT     = 3,
   ENET_PROTOCOL_COMMAND_DISCONNECT         = 4,
   ENET_PROTOCOL_COMMAND_PING               = 5,
   ENET_PROTOCOL_COMMAND_SEND_RELIABLE      = 6,
   ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE    = 7,
   ENET_PROTOCOL_COMMAND_SEND_FRAGMENT      = 8,
   ENET_PROTOCOL_COMMAND_BANDWIDTH_LIMIT    = 9,
   ENET_PROTOCOL_COMMAND_THROTTLE_CONFIGURE = 10,
   ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED   = 11
} ENetProtocolCommand;

typedef enum
{
   ENET_PROTOCOL_FLAG_ACKNOWLEDGE = (1 << 0),
   ENET_PROTOCOL_FLAG_UNSEQUENCED = (1 << 1)
} ENetProtocolFlag;

typedef struct
{
   enet_uint16 peerID;
   enet_uint8 flags;
   enet_uint8 commandCount;
   enet_uint32 sentTime;
   enet_uint32 challenge;
} ENetProtocolHeader;

typedef struct
{
   enet_uint8 command;
   enet_uint8 channelID;
   enet_uint8 flags;
   enet_uint8 reserved;
   enet_uint32 commandLength;
   enet_uint32 reliableSequenceNumber;
} ENetProtocolCommandHeader;

typedef struct
{
   ENetProtocolCommandHeader header;
   enet_uint32 receivedReliableSequenceNumber;
   enet_uint32 receivedSentTime;
} ENetProtocolAcknowledge;

typedef struct
{
   ENetProtocolCommandHeader header;
   enet_uint16 outgoingPeerID;
   enet_uint16 mtu;
   enet_uint32 windowSize;
   enet_uint32 channelCount;
   enet_uint32 incomingBandwidth;
   enet_uint32 outgoingBandwidth;
   enet_uint32 packetThrottleInterval;
   enet_uint32 packetThrottleAcceleration;
   enet_uint32 packetThrottleDeceleration;
} ENetProtocolConnect;

typedef struct
{
   ENetProtocolCommandHeader header;
   enet_uint16 outgoingPeerID;
   enet_uint16 mtu;
   enet_uint32 windowSize;
   enet_uint32 channelCount;
   enet_uint32 incomingBandwidth;
   enet_uint32 outgoingBandwidth;
   enet_uint32 packetThrottleInterval;
   enet_uint32 packetThrottleAcceleration;
   enet_uint32 packetThrottleDeceleration;
} ENetProtocolVerifyConnect;

typedef struct
{
   ENetProtocolCommandHeader header;
   enet_uint32 incomingBandwidth;
   enet_uint32 outgoingBandwidth;
} ENetProtocolBandwidthLimit;

typedef struct
{
   ENetProtocolCommandHeader header;
   enet_uint32 packetThrottleInterval;
   enet_uint32 packetThrottleAcceleration;
   enet_uint32 packetThrottleDeceleration;
} ENetProtocolThrottleConfigure;

typedef struct
{
   ENetProtocolCommandHeader header;
   enet_uint32 data;
} ENetProtocolDisconnect;

typedef struct
{
   ENetProtocolCommandHeader header;
} ENetProtocolPing;

typedef struct
{
   ENetProtocolCommandHeader header;
} ENetProtocolSendReliable;

typedef struct
{
   ENetProtocolCommandHeader header;
   enet_uint32 unreliableSequenceNumber;
} ENetProtocolSendUnreliable;

typedef struct
{
   ENetProtocolCommandHeader header;
   enet_uint32 unsequencedGroup;
} ENetProtocolSendUnsequenced;

typedef struct
{
   ENetProtocolCommandHeader header;
   enet_uint32 startSequenceNumber;
   enet_uint32 fragmentCount;
   enet_uint32 fragmentNumber;
   enet_uint32 totalLength;
   enet_uint32 fragmentOffset;
} ENetProtocolSendFragment;

typedef union
{
   ENetProtocolCommandHeader header;
   ENetProtocolAcknowledge acknowledge;
   ENetProtocolConnect connect;
   ENetProtocolVerifyConnect verifyConnect;
   ENetProtocolDisconnect disconnect;
   ENetProtocolPing ping;
   ENetProtocolSendReliable sendReliable;
   ENetProtocolSendUnreliable sendUnreliable;
   ENetProtocolSendUnsequenced sendUnsequenced;
   ENetProtocolSendFragment sendFragment;
   ENetProtocolBandwidthLimit bandwidthLimit;
   ENetProtocolThrottleConfigure throttleConfigure;
} ENetProtocol;

#endif /* __ENET_PROTOCOL_H__ */


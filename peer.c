/** 
 @file  peer.c
 @brief ENet peer management functions
*/
#define ENET_BUILDING_LIB 1
#include "enet/enet.h"

/** @defgroup peer ENet peer functions 
    @{
*/

/** Configures throttle parameter for a peer.

    Unreliable packets are dropped by ENet in response to the varying conditions
    of the Internet connection to the peer.  The throttle represents a probability
    that an unreliable packet should not be dropped and thus sent by ENet to the peer.
    The lowest mean round trip time from the sending of a reliable packet to the
    receipt of its acknowledgement is measured over an amount of time specified by
    the interval parameter in milliseconds.  If a measured round trip time happens to
    be significantly less than the mean round trip time measured over the interval, 
    then the throttle probability is increased to allow more traffic by an amount
    specified in the acceleration parameter, which is a ratio to the ENET_PEER_PACKET_THROTTLE_SCALE
    constant.  If a measured round trip time happens to be significantly greater than
    the mean round trip time measured over the interval, then the throttle probability
    is decreased to limit traffic by an amount specified in the deceleration parameter, which
    is a ratio to the ENET_PEER_PACKET_THROTTLE_SCALE constant.  When the throttle has
    a value of ENET_PEER_PACKET_THROTTLE_SCALE, on unreliable packets are dropped by 
    ENet, and so 100% of all unreliable packets will be sent.  When the throttle has a
    value of 0, all unreliable packets are dropped by ENet, and so 0% of all unreliable
    packets will be sent.  Intermediate values for the throttle represent intermediate
    probabilities between 0% and 100% of unreliable packets being sent.  The bandwidth
    limits of the local and foreign hosts are taken into account to determine a 
    sensible limit for the throttle probability above which it should not raise even in
    the best of conditions.

    @param peer peer to configure 
    @param interval interval, in milliseconds, over which to measure lowest mean RTT; the default value is ENET_PEER_PACKET_THROTTLE_INTERVAL.
    @param acceleration rate at which to increase the throttle probability as mean RTT declines
    @param deceleration rate at which to decrease the throttle probability as mean RTT increases
*/
void
enet_peer_throttle_configure (ENetPeer * peer, enet_uint32 interval, enet_uint32 acceleration, enet_uint32 deceleration)
{
    ENetProtocol command;

    peer -> packetThrottleInterval = interval;
    peer -> packetThrottleAcceleration = acceleration;
    peer -> packetThrottleDeceleration = deceleration;

    command.header.command = ENET_PROTOCOL_COMMAND_THROTTLE_CONFIGURE;
    command.header.channelID = 0xFF;
    command.header.flags = ENET_PROTOCOL_FLAG_ACKNOWLEDGE;
    command.header.commandLength = sizeof (ENetProtocolThrottleConfigure);

    command.throttleConfigure.packetThrottleInterval = ENET_HOST_TO_NET_32 (interval);
    command.throttleConfigure.packetThrottleAcceleration = ENET_HOST_TO_NET_32 (acceleration);
    command.throttleConfigure.packetThrottleDeceleration = ENET_HOST_TO_NET_32 (deceleration);

    enet_peer_queue_outgoing_command (peer, & command, NULL, 0, 0);
}

int
enet_peer_throttle (ENetPeer * peer, enet_uint32 rtt)
{
    if (peer -> lastRoundTripTime <= peer -> lastRoundTripTimeVariance)
    {
        peer -> packetThrottle = peer -> packetThrottleLimit;
    }
    else
    if (rtt < peer -> lastRoundTripTime)
    {
        peer -> packetThrottle += peer -> packetThrottleAcceleration;

        if (peer -> packetThrottle > peer -> packetThrottleLimit)
          peer -> packetThrottle = peer -> packetThrottleLimit;

        return 1;
    }
    else
    if (rtt > peer -> lastRoundTripTime + 2 * peer -> lastRoundTripTimeVariance)
    {
        if (peer -> packetThrottle > peer -> packetThrottleDeceleration)
          peer -> packetThrottle -= peer -> packetThrottleDeceleration;
        else
          peer -> packetThrottle = 0;

        return -1;
    }

    return 0;
}

/** Queues a packet to be sent.
    @param peer destination for the packet
    @param channelID channel on which to send
    @param packet packet to send
    @retval 0 on success
    @retval < 0 on failure
*/
int
enet_peer_send (ENetPeer * peer, enet_uint8 channelID, ENetPacket * packet)
{
   ENetChannel * channel = & peer -> channels [channelID];
   ENetProtocol command;
   size_t fragmentLength;

   if (peer -> state != ENET_PEER_STATE_CONNECTED ||
       channelID >= peer -> channelCount)
     return -1;

   fragmentLength = peer -> mtu - sizeof (ENetProtocolHeader) - sizeof (ENetProtocolSendFragment);

   if (packet -> dataLength > fragmentLength)
   {
      enet_uint32 fragmentCount = ENET_HOST_TO_NET_32 ((packet -> dataLength + fragmentLength - 1) / fragmentLength),
             startSequenceNumber = ENET_HOST_TO_NET_32 (channel -> outgoingReliableSequenceNumber + 1),
             fragmentNumber,
             fragmentOffset;

      packet -> flags = ENET_PACKET_FLAG_RELIABLE;

      for (fragmentNumber = 0,
             fragmentOffset = 0;
           fragmentOffset < packet -> dataLength;
           ++ fragmentNumber,
             fragmentOffset += fragmentLength)
      {
         command.header.command = ENET_PROTOCOL_COMMAND_SEND_FRAGMENT;
         command.header.channelID = channelID;
         command.header.flags = ENET_PROTOCOL_FLAG_ACKNOWLEDGE;
         command.header.commandLength = sizeof (ENetProtocolSendFragment);
         command.sendFragment.startSequenceNumber = startSequenceNumber;
         command.sendFragment.fragmentCount = fragmentCount;
         command.sendFragment.fragmentNumber = ENET_HOST_TO_NET_32 (fragmentNumber);
         command.sendFragment.totalLength = ENET_HOST_TO_NET_32 (packet -> dataLength);
         command.sendFragment.fragmentOffset = ENET_NET_TO_HOST_32 (fragmentOffset);

         if (packet -> dataLength - fragmentOffset < fragmentLength)
           fragmentLength = packet -> dataLength - fragmentOffset;

         enet_peer_queue_outgoing_command (peer, & command, packet, fragmentOffset, fragmentLength);
      }

      return 0;
   }

   command.header.channelID = channelID;

   if (packet -> flags & ENET_PACKET_FLAG_RELIABLE)
   {
      command.header.command = ENET_PROTOCOL_COMMAND_SEND_RELIABLE;
      command.header.flags = ENET_PROTOCOL_FLAG_ACKNOWLEDGE;
      command.header.commandLength = sizeof (ENetProtocolSendReliable);
   }
   else
   if (packet -> flags & ENET_PACKET_FLAG_UNSEQUENCED)
   {
      command.header.command = ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED;
      command.header.flags = ENET_PROTOCOL_FLAG_UNSEQUENCED;
      command.header.commandLength = sizeof (ENetProtocolSendUnsequenced);
      command.sendUnsequenced.unsequencedGroup = ENET_HOST_TO_NET_32 (peer -> outgoingUnsequencedGroup + 1);
   }
   else
   {
      command.header.command = ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE;
      command.header.flags = 0;
      command.header.commandLength = sizeof (ENetProtocolSendUnreliable);
      command.sendUnreliable.unreliableSequenceNumber = ENET_HOST_TO_NET_32 (channel -> outgoingUnreliableSequenceNumber + 1);
   }

   enet_peer_queue_outgoing_command (peer, & command, packet, 0, packet -> dataLength);

   return 0;
}

/** Attempts to dequeue any incoming queued packet.
    @param peer peer to dequeue packets from
    @param channelID channel on which to receive
    @returns a pointer to the packet, or NULL if there are no available incoming queued packets
*/
ENetPacket *
enet_peer_receive (ENetPeer * peer, enet_uint8 channelID)
{
   ENetChannel * channel = & peer -> channels [channelID];
   ENetIncomingCommand * incomingCommand = NULL;
   ENetPacket * packet;

   if (enet_list_empty (& channel -> incomingUnreliableCommands) == 0)
   {
      incomingCommand = (ENetIncomingCommand *) enet_list_front (& channel -> incomingUnreliableCommands);

      if (incomingCommand -> unreliableSequenceNumber > 0)
      {
         if (incomingCommand -> reliableSequenceNumber > channel -> incomingReliableSequenceNumber)
           incomingCommand = NULL;
         else
           channel -> incomingUnreliableSequenceNumber = incomingCommand -> unreliableSequenceNumber;
      }
   }

   if (incomingCommand == NULL &&
       enet_list_empty (& channel -> incomingReliableCommands) == 0)
   {
      do
      {
        incomingCommand = (ENetIncomingCommand *) enet_list_front (& channel -> incomingReliableCommands);

        if (incomingCommand -> fragmentsRemaining > 0 ||
            incomingCommand -> reliableSequenceNumber > channel -> incomingReliableSequenceNumber + 1)
          return NULL;

        if (incomingCommand -> reliableSequenceNumber <= channel -> incomingReliableSequenceNumber)
        {
           -- incomingCommand -> packet -> referenceCount;

           if (incomingCommand -> packet -> referenceCount == 0)
             enet_packet_destroy (incomingCommand -> packet);

           if (incomingCommand -> fragments != NULL)
             enet_free (incomingCommand -> fragments);

           enet_list_remove (& incomingCommand -> incomingCommandList);

           enet_free (incomingCommand);

           incomingCommand = NULL;
        }
      } while (incomingCommand == NULL &&
               enet_list_empty (& channel -> incomingReliableCommands) == 0);

      if (incomingCommand == NULL)
        return NULL;

      channel -> incomingReliableSequenceNumber = incomingCommand -> reliableSequenceNumber;

      if (incomingCommand -> fragmentCount > 0)
        channel -> incomingReliableSequenceNumber += incomingCommand -> fragmentCount - 1;
   }

   if (incomingCommand == NULL)
     return NULL;

   enet_list_remove (& incomingCommand -> incomingCommandList);

   packet = incomingCommand -> packet;

   -- packet -> referenceCount;

   if (incomingCommand -> fragments != NULL)
     enet_free (incomingCommand -> fragments);

   enet_free (incomingCommand);

   return packet;
}

static void
enet_peer_reset_outgoing_commands (ENetList * queue)
{
    ENetOutgoingCommand * outgoingCommand;

    while (enet_list_empty (queue) == 0)
    {
       outgoingCommand = (ENetOutgoingCommand *) enet_list_remove (enet_list_begin (queue));

       if (outgoingCommand -> packet != NULL)
       {
          -- outgoingCommand -> packet -> referenceCount;

          if (outgoingCommand -> packet -> referenceCount == 0)
            enet_packet_destroy (outgoingCommand -> packet);
       }

       enet_free (outgoingCommand);
    }
}

static void
enet_peer_reset_incoming_commands (ENetList * queue)
{
    ENetIncomingCommand * incomingCommand;

    while (enet_list_empty (queue) == 0)
    {
       incomingCommand = (ENetIncomingCommand *) enet_list_remove (enet_list_begin (queue));

       if (incomingCommand -> packet != NULL)
       {
          -- incomingCommand -> packet -> referenceCount;

          if (incomingCommand -> packet -> referenceCount == 0)
            enet_packet_destroy (incomingCommand -> packet);
       }

       if (incomingCommand -> fragments != NULL)
         enet_free (incomingCommand -> fragments);

       enet_free (incomingCommand);
    }
}

void
enet_peer_reset_queues (ENetPeer * peer)
{
    ENetChannel * channel;

    while (enet_list_empty (& peer -> acknowledgements) == 0)
      enet_free (enet_list_remove (enet_list_begin (& peer -> acknowledgements)));

    enet_peer_reset_outgoing_commands (& peer -> sentReliableCommands);
    enet_peer_reset_outgoing_commands (& peer -> sentUnreliableCommands);
    enet_peer_reset_outgoing_commands (& peer -> outgoingReliableCommands);
    enet_peer_reset_outgoing_commands (& peer -> outgoingUnreliableCommands);

    if (peer -> channels != NULL && peer -> channelCount > 0)
    {
        for (channel = peer -> channels;
             channel < & peer -> channels [peer -> channelCount];
             ++ channel)
        {
            enet_peer_reset_incoming_commands (& channel -> incomingReliableCommands);
            enet_peer_reset_incoming_commands (& channel -> incomingUnreliableCommands);
        }

        enet_free (peer -> channels);
    }

    peer -> channels = NULL;
    peer -> channelCount = 0;
}

/** Forcefully disconnects a peer.
    @param peer peer to forcefully disconnect
    @remarks The foreign host represented by the peer is not notified of the disconnection and will timeout
    on its connection to the local host.
*/
void
enet_peer_reset (ENetPeer * peer)
{
    peer -> outgoingPeerID = 0xFFFF;
    peer -> challenge = 0;

    peer -> address.host = ENET_HOST_ANY;
    peer -> address.port = 0;

    peer -> state = ENET_PEER_STATE_DISCONNECTED;

    peer -> incomingBandwidth = 0;
    peer -> outgoingBandwidth = 0;
    peer -> incomingBandwidthThrottleEpoch = 0;
    peer -> outgoingBandwidthThrottleEpoch = 0;
    peer -> incomingDataTotal = 0;
    peer -> outgoingDataTotal = 0;
    peer -> lastSendTime = 0;
    peer -> lastReceiveTime = 0;
    peer -> nextTimeout = 0;
    peer -> earliestTimeout = 0;
    peer -> packetLossEpoch = 0;
    peer -> packetsSent = 0;
    peer -> packetsLost = 0;
    peer -> packetLoss = 0;
    peer -> packetLossVariance = 0;
    peer -> packetThrottle = ENET_PEER_DEFAULT_PACKET_THROTTLE;
    peer -> packetThrottleLimit = ENET_PEER_PACKET_THROTTLE_SCALE;
    peer -> packetThrottleCounter = 0;
    peer -> packetThrottleEpoch = 0;
    peer -> packetThrottleAcceleration = ENET_PEER_PACKET_THROTTLE_ACCELERATION;
    peer -> packetThrottleDeceleration = ENET_PEER_PACKET_THROTTLE_DECELERATION;
    peer -> packetThrottleInterval = ENET_PEER_PACKET_THROTTLE_INTERVAL;
    peer -> lastRoundTripTime = ENET_PEER_DEFAULT_ROUND_TRIP_TIME;
    peer -> lowestRoundTripTime = ENET_PEER_DEFAULT_ROUND_TRIP_TIME;
    peer -> lastRoundTripTimeVariance = 0;
    peer -> highestRoundTripTimeVariance = 0;
    peer -> roundTripTime = ENET_PEER_DEFAULT_ROUND_TRIP_TIME;
    peer -> roundTripTimeVariance = 0;
    peer -> mtu = peer -> host -> mtu;
    peer -> reliableDataInTransit = 0;
    peer -> outgoingReliableSequenceNumber = 0;
    peer -> windowSize = ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE;
    peer -> incomingUnsequencedGroup = 0;
    peer -> outgoingUnsequencedGroup = 0;

    memset (peer -> unsequencedWindow, 0, sizeof (peer -> unsequencedWindow));
    
    enet_peer_reset_queues (peer);
}

/** Sends a ping request to a peer.
    @param peer destination for the ping request
    @remarks ping requests factor into the mean round trip time as designated by the 
    roundTripTime field in the ENetPeer structure.  Enet automatically pings all connected
    peers at regular intervals, however, this function may be called to ensure more
    frequent ping requests.
*/
void
enet_peer_ping (ENetPeer * peer)
{
    ENetProtocol command;

    if (peer -> state != ENET_PEER_STATE_CONNECTED)
      return;

    command.header.command = ENET_PROTOCOL_COMMAND_PING;
    command.header.channelID = 0xFF;
    command.header.flags = ENET_PROTOCOL_FLAG_ACKNOWLEDGE;
    command.header.commandLength = sizeof (ENetProtocolPing);
   
    enet_peer_queue_outgoing_command (peer, & command, NULL, 0, 0);
}

/** Force an immediate disconnection from a peer.
    @param peer peer to disconnect
    @remarks No ENET_EVENT_DISCONNECT event will be generated. The foreign peer is not
    guarenteed to receive the disconnect notification, and is reset immediately upon
    return from this function.
*/
void
enet_peer_disconnect_now (ENetPeer * peer)
{
    ENetProtocol command;

    if (peer -> state == ENET_PEER_STATE_DISCONNECTED)
      return;

    if (peer -> state != ENET_PEER_STATE_ZOMBIE &&
        peer -> state != ENET_PEER_STATE_DISCONNECTING)
    {
        enet_peer_reset_queues (peer);

        command.header.command = ENET_PROTOCOL_COMMAND_DISCONNECT;
        command.header.channelID = 0xFF;
        command.header.flags = ENET_PROTOCOL_FLAG_UNSEQUENCED;
        command.header.commandLength = sizeof (ENetProtocolDisconnect);

        enet_peer_queue_outgoing_command (peer, & command, NULL, 0, 0);

        enet_host_flush (peer -> host);
    }

    enet_peer_reset (peer);
}

/** Request a disconnection from a peer.
    @param peer peer to request a disconnection
    @remarks An ENET_EVENT_DISCONNECT event will be generated by enet_host_service()
    once the disconnection is complete.
*/
void
enet_peer_disconnect (ENetPeer * peer)
{
    ENetProtocol command;

    if (peer -> state == ENET_PEER_STATE_DISCONNECTING ||
        peer -> state == ENET_PEER_STATE_DISCONNECTED ||
        peer -> state == ENET_PEER_STATE_ZOMBIE)
      return;

    enet_peer_reset_queues (peer);

    command.header.command = ENET_PROTOCOL_COMMAND_DISCONNECT;
    command.header.channelID = 0xFF;
    command.header.flags = ENET_PROTOCOL_FLAG_UNSEQUENCED;
    command.header.commandLength = sizeof (ENetProtocolDisconnect);

    if (peer -> state == ENET_PEER_STATE_CONNECTED)
      command.header.flags = ENET_PROTOCOL_FLAG_ACKNOWLEDGE;
    
    enet_peer_queue_outgoing_command (peer, & command, NULL, 0, 0);

    if (peer -> state == ENET_PEER_STATE_CONNECTED)
      peer -> state = ENET_PEER_STATE_DISCONNECTING;
    else
    {
        enet_host_flush (peer -> host);
        enet_peer_reset (peer);
    }
}

ENetAcknowledgement *
enet_peer_queue_acknowledgement (ENetPeer * peer, const ENetProtocol * command, enet_uint32 sentTime)
{
    ENetAcknowledgement * acknowledgement;

    peer -> outgoingDataTotal += sizeof (ENetProtocolAcknowledge);

    acknowledgement = (ENetAcknowledgement *) enet_malloc (sizeof (ENetAcknowledgement));

    acknowledgement -> sentTime = sentTime;
    acknowledgement -> command = * command;
    
    enet_list_insert (enet_list_end (& peer -> acknowledgements), acknowledgement);
    
    return acknowledgement;
}

ENetOutgoingCommand *
enet_peer_queue_outgoing_command (ENetPeer * peer, const ENetProtocol * command, ENetPacket * packet, enet_uint32 offset, enet_uint16 length)
{
    ENetChannel * channel = & peer -> channels [command -> header.channelID];
    ENetOutgoingCommand * outgoingCommand;

    peer -> outgoingDataTotal += command -> header.commandLength + length;

    outgoingCommand = (ENetOutgoingCommand *) enet_malloc (sizeof (ENetOutgoingCommand));

    if (command -> header.channelID == 0xFF)
    {
       ++ peer -> outgoingReliableSequenceNumber;

       outgoingCommand -> reliableSequenceNumber = peer -> outgoingReliableSequenceNumber;
       outgoingCommand -> unreliableSequenceNumber = 0;
    }
    else
    if (command -> header.flags & ENET_PROTOCOL_FLAG_ACKNOWLEDGE)
    {
       ++ channel -> outgoingReliableSequenceNumber;
       
       outgoingCommand -> reliableSequenceNumber = channel -> outgoingReliableSequenceNumber;
       outgoingCommand -> unreliableSequenceNumber = 0;
    }
    else
    if (command -> header.flags & ENET_PROTOCOL_FLAG_UNSEQUENCED)
    {
       ++ peer -> outgoingUnsequencedGroup;

       outgoingCommand -> reliableSequenceNumber = 0;
       outgoingCommand -> unreliableSequenceNumber = 0;
    }
    else
    {
       ++ channel -> outgoingUnreliableSequenceNumber;
        
       outgoingCommand -> reliableSequenceNumber = channel -> outgoingReliableSequenceNumber;
       outgoingCommand -> unreliableSequenceNumber = channel -> outgoingUnreliableSequenceNumber;
    }
   
    outgoingCommand -> sentTime = 0;
    outgoingCommand -> roundTripTimeout = 0;
    outgoingCommand -> roundTripTimeoutLimit = 0;
    outgoingCommand -> fragmentOffset = offset;
    outgoingCommand -> fragmentLength = length;
    outgoingCommand -> packet = packet;
    outgoingCommand -> command = * command;
    outgoingCommand -> command.header.reliableSequenceNumber = ENET_HOST_TO_NET_32 (outgoingCommand -> reliableSequenceNumber);

    if (packet != NULL)
      ++ packet -> referenceCount;

    if (command -> header.flags & ENET_PROTOCOL_FLAG_ACKNOWLEDGE)
      enet_list_insert (enet_list_end (& peer -> outgoingReliableCommands), outgoingCommand);
    else
      enet_list_insert (enet_list_end (& peer -> outgoingUnreliableCommands), outgoingCommand);

    return outgoingCommand;
}

ENetIncomingCommand *
enet_peer_queue_incoming_command (ENetPeer * peer, const ENetProtocol * command, ENetPacket * packet, enet_uint32 fragmentCount)
{
    ENetChannel * channel = & peer -> channels [command -> header.channelID];
    enet_uint32 unreliableSequenceNumber = 0;
    ENetIncomingCommand * incomingCommand;
    ENetListIterator currentCommand;

    switch (command -> header.command)
    {
    case ENET_PROTOCOL_COMMAND_SEND_FRAGMENT:
    case ENET_PROTOCOL_COMMAND_SEND_RELIABLE:
       for (currentCommand = enet_list_previous (enet_list_end (& channel -> incomingReliableCommands));
            currentCommand != enet_list_end (& channel -> incomingReliableCommands);
            currentCommand = enet_list_previous (currentCommand))
       {
          incomingCommand = (ENetIncomingCommand *) currentCommand;

          if (incomingCommand -> reliableSequenceNumber <= command -> header.reliableSequenceNumber)
          {
             if (incomingCommand -> reliableSequenceNumber < command -> header.reliableSequenceNumber)
               break;

             goto freePacket;
          }
       }
       break;

    case ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE:
       unreliableSequenceNumber = ENET_NET_TO_HOST_32 (command -> sendUnreliable.unreliableSequenceNumber);

       if (command -> header.reliableSequenceNumber < channel -> incomingReliableSequenceNumber)
         goto freePacket;

       if (unreliableSequenceNumber <= channel -> incomingUnreliableSequenceNumber)
         goto freePacket;

       for (currentCommand = enet_list_previous (enet_list_end (& channel -> incomingUnreliableCommands));
            currentCommand != enet_list_end (& channel -> incomingUnreliableCommands);
            currentCommand = enet_list_previous (currentCommand))
       {
          incomingCommand = (ENetIncomingCommand *) currentCommand;

          if (incomingCommand -> unreliableSequenceNumber <= unreliableSequenceNumber)
          {
             if (incomingCommand -> unreliableSequenceNumber < unreliableSequenceNumber)
               break;

             goto freePacket;
          }
       }
       break;

    case ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED:
       currentCommand = enet_list_end (& channel -> incomingUnreliableCommands);
       break;

    default:
       goto freePacket;
    }

    incomingCommand = (ENetIncomingCommand *) enet_malloc (sizeof (ENetIncomingCommand));

    incomingCommand -> reliableSequenceNumber = command -> header.reliableSequenceNumber;
    incomingCommand -> unreliableSequenceNumber = unreliableSequenceNumber;
    incomingCommand -> command = * command;
    incomingCommand -> fragmentCount = fragmentCount;
    incomingCommand -> fragmentsRemaining = fragmentCount;
    incomingCommand -> packet = packet;
    incomingCommand -> fragments = NULL;
    
    if (fragmentCount > 0)
    { 
       incomingCommand -> fragments = (enet_uint32 *) enet_malloc ((fragmentCount + 31) / 32 * sizeof (enet_uint32));
       memset (incomingCommand -> fragments, 0, (fragmentCount + 31) / 32 * sizeof (enet_uint32));
    }

    if (packet != NULL)
      ++ packet -> referenceCount;

    enet_list_insert (enet_list_next (currentCommand), incomingCommand);

    return incomingCommand;

freePacket:
    if (packet != NULL)
    {
       if (packet -> referenceCount == 0)
         enet_packet_destroy (packet);
    }

    return NULL;
}

/** @} */

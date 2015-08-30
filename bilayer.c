#include <string.h>

#include "bilayer.h"

ENetTransport enet_socket_create_transport(ENetSocketType type, ENetSocket socket)
{
    ENetTransport transport;

    transport.type = ENET_TRANSPORT_TYPE_SOCKET;
    transport.context = (void*)0x1;
    transport.handle.socket = socket;
    
    transport.send = enet_socket_send;
    transport.recv = enet_socket_receive;

    return transport;
}


ENetHost *
enet_host_create_bilayer (ENetTransportType type, const ENetAddress * address, size_t peerCount, size_t channelLimit, enet_uint32 incomingBandwidth, enet_uint32 outgoingBandwidth)
{
    if (type == ENET_TRANSPORT_TYPE_CUSTOM) {
        ENetHost *host = enet_host_create_notp(address, peerCount, channelLimit, incomingBandwidth, outgoingBandwidth);
        host->address = *address;
        return host;
    } else {
        ENetHost *host = enet_host_create(address, peerCount, channelLimit, incomingBandwidth, outgoingBandwidth);
        ENetTransport transport = enet_socket_create_transport(ENET_SOCKET_TYPE_DATAGRAM, host->socket);
        host->transport = transport;
        return host;
    }
    return NULL;
}

void enet_host_transport(ENetHost *host, const ENetTransport *transport)
{
    if (transport->type != ENET_TRANSPORT_TYPE_CUSTOM) {
        // error   
    }

    if (transport->context == NULL) {
        // error
    }
    
    host->transport = *transport;
}

ENetHost *
enet_host_create_notp (const ENetAddress * address, size_t peerCount, size_t channelLimit, enet_uint32 incomingBandwidth, enet_uint32 outgoingBandwidth)
{
    ENetHost * host;
    ENetPeer * currentPeer;

    if (peerCount > ENET_PROTOCOL_MAXIMUM_PEER_ID)
      return NULL;

    host = (ENetHost *) enet_malloc (sizeof (ENetHost));
    if (host == NULL)
      return NULL;
    memset (host, 0, sizeof (ENetHost));

    host -> peers = (ENetPeer *) enet_malloc (peerCount * sizeof (ENetPeer));
    if (host -> peers == NULL)
    {
       enet_free (host);

       return NULL;
    }
    memset (host -> peers, 0, peerCount * sizeof (ENetPeer));

    /*
    host -> socket = enet_socket_create (ENET_SOCKET_TYPE_DATAGRAM);
    if (host -> socket == ENET_SOCKET_NULL || (address != NULL && enet_socket_bind (host -> socket, address) < 0))
    {
       if (host -> socket != ENET_SOCKET_NULL)
         enet_socket_destroy (host -> socket);

       enet_free (host -> peers);
       enet_free (host);

       return NULL;
    }

    enet_socket_set_option (host -> socket, ENET_SOCKOPT_NONBLOCK, 1);
    enet_socket_set_option (host -> socket, ENET_SOCKOPT_BROADCAST, 1);
    enet_socket_set_option (host -> socket, ENET_SOCKOPT_RCVBUF, ENET_HOST_RECEIVE_BUFFER_SIZE);
    enet_socket_set_option (host -> socket, ENET_SOCKOPT_SNDBUF, ENET_HOST_SEND_BUFFER_SIZE);
    
    if (address != NULL && enet_socket_get_address (host -> socket, & host -> address) < 0)   
      host -> address = * address;
    
    */
    
    if (! channelLimit || channelLimit > ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT)
      channelLimit = ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT;
    else
    if (channelLimit < ENET_PROTOCOL_MINIMUM_CHANNEL_COUNT)
      channelLimit = ENET_PROTOCOL_MINIMUM_CHANNEL_COUNT;

    host -> randomSeed = (enet_uint32) (size_t) host;
    host -> randomSeed += enet_host_random_seed ();
    host -> randomSeed = (host -> randomSeed << 16) | (host -> randomSeed >> 16);
    host -> channelLimit = channelLimit;
    host -> incomingBandwidth = incomingBandwidth;
    host -> outgoingBandwidth = outgoingBandwidth;
    host -> bandwidthThrottleEpoch = 0;
    host -> recalculateBandwidthLimits = 0;
    host -> mtu = ENET_HOST_DEFAULT_MTU;
    host -> peerCount = peerCount;
    host -> commandCount = 0;
    host -> bufferCount = 0;
    host -> checksum = NULL;
    host -> receivedAddress.host = ENET_HOST_ANY;
    host -> receivedAddress.port = 0;
    host -> receivedData = NULL;
    host -> receivedDataLength = 0;
     
    host -> totalSentData = 0;
    host -> totalSentPackets = 0;
    host -> totalReceivedData = 0;
    host -> totalReceivedPackets = 0;

    host -> connectedPeers = 0;
    host -> bandwidthLimitedPeers = 0;
    host -> duplicatePeers = ENET_PROTOCOL_MAXIMUM_PEER_ID;
    host -> maximumPacketSize = ENET_HOST_DEFAULT_MAXIMUM_PACKET_SIZE;
    host -> maximumWaitingData = ENET_HOST_DEFAULT_MAXIMUM_WAITING_DATA;

    host -> compressor.context = NULL;
    host -> compressor.compress = NULL;
    host -> compressor.decompress = NULL;
    host -> compressor.destroy = NULL;

    host -> intercept = NULL;

    enet_list_clear (& host -> dispatchQueue);

    for (currentPeer = host -> peers;
         currentPeer < & host -> peers [host -> peerCount];
         ++ currentPeer)
    {
       currentPeer -> host = host;
       currentPeer -> incomingPeerID = currentPeer - host -> peers;
       currentPeer -> outgoingSessionID = currentPeer -> incomingSessionID = 0xFF;
       currentPeer -> data = NULL;

       enet_list_clear (& currentPeer -> acknowledgements);
       enet_list_clear (& currentPeer -> sentReliableCommands);
       enet_list_clear (& currentPeer -> sentUnreliableCommands);
       enet_list_clear (& currentPeer -> outgoingReliableCommands);
       enet_list_clear (& currentPeer -> outgoingUnreliableCommands);
       enet_list_clear (& currentPeer -> dispatchedCommands);

       enet_peer_reset (currentPeer);
    }

    return host;
}

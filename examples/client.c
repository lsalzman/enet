#include <stdio.h>
#include <enet/enet.h>

int main(int argc, char** argv)
{
    ENetHost *client = NULL;
    ENetAddress address;
    ENetEvent event;
    ENetPeer *peer;
    ENetPacket *packet;
    int retcode;

    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return 1;
    }

    client = enet_host_create(NULL /* create a client host */,
                              1    /* only allow 1 outgoing connection */,
                              2    /* allow up 2 channels to be used, 0 and 1 */,
                              0    /* assume any amount of incoming bandwidth */,
                              0    /* assume any amount of outgoing bandwidth */);
    if (client == NULL)
    {
        fprintf(stderr, "An error occurred while trying to create an ENet client host.\n");
        goto Exit;
    }

    /* Initiate the connection. */
    enet_address_set_host(&address, "localhost");
    address.port = 1234;
    peer = enet_host_connect(client   /* host seeking the connection */, 
                             &address /* destination for the connection */, 
                             2        /* number of channels to allocate */, 
                             0        /* user data supplied to the receiving host */);
    if (peer == NULL)
    {
        fprintf(stderr, "No available peers for initiating an ENet connection.\n");
        goto Exit;
    }

    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        fprintf(stdout, "Connect succeeded.\n");
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);

        fprintf(stderr, "Connect failed.\n");
        goto Exit;
    }

    /* Sending packets to an ENet peer. */
    for (int i = 0; i < 4; i++)
    {
        packet = enet_packet_create(NULL /* initial contents of the packet's data */, 
                                    9    /* size of the data allocated for this packet */, 
                                    ENET_PACKET_FLAG_RELIABLE /* flags for this packet */);
        
        sprintf((char*)packet->data, "packet %d", i);

        enet_peer_send(peer   /* destination for the packet */,
                       i % 2  /* channel on which to send */,
                       packet /* packet to send */);
    }
    /* Send out queued packets. One could just use enet_host_service() instead. */
    enet_host_flush(client);

    /* Initiate the disconnection. */
    enet_peer_disconnect(peer /* peer to request a disconnection */, 
                         0    /* data describing the disconnection */);

    /* Allow up to 3 seconds for the disconnect to succeed and drop any packets received packets. */
    retcode = 1;
    while (retcode > 0)
    {
        retcode = enet_host_service(client, &event, 3000);
        if (retcode > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                fprintf(stdout, "Disconnect succeeded.\n");
                retcode = 0;
                break;

            default:
                ;
            }
        }
        else
        {
            /* Timeout or failure */
            fprintf(stderr, "Disconnect failed.\n");
            enet_peer_reset(peer);
        }
    }

Exit:
    if (client != NULL)
    {
        enet_host_destroy(client);
    }
    enet_deinitialize();

    return 0;
}


#include <stdio.h>
#include <string.h>
#include <enet/enet.h>

int main(int argc, char **argv)
{
    int retcode;
    ENetAddress address;
    ENetHost *server = NULL;
    ENetEvent event;
    char peerIP[30];

    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return 1;
    }

    address.host = ENET_HOST_ANY;
    address.port = 1234;
    server = enet_host_create(&address /* the address to bind the server host to */,
                              32       /* allow up to 32 clients and/or outgoing connections */,
                              2        /* allow up to 2 channels to be used, 0 and 1 */,
                              0        /* assume any amount of incoming bandwidth */,
                              0        /* assume any amount of outgoing bandwidth */);
    if (server == NULL)
    {
        fprintf(stderr, "An error occurred while trying to create an ENet server host.\n");
        goto Exit;
    }
    
    fprintf(stdout, "Server started\n");
   
    for (;;)
    {
        retcode = enet_host_service(server, /* host to service */ 
                                    &event, /* an event structure where event details will be placed if one occurs */
                                    1000    /* number of milliseconds that ENet should wait for events */);
        if (retcode > 0)
        {
            /* an event occurred within the specified time limit */
            if (enet_address_get_host_ip(&(event.peer->address), peerIP, 30) != 0)
            {
                strcpy(peerIP, "?");
            }
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                fprintf(stdout, 
                        "Client connected: %s:%u\n",
                        peerIP,
                        (unsigned int)event.peer->address.port);
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                fprintf(stdout,
                        "Packet received: %s:%u channel=%u length=%u data=\"%s\"\n",
                        peerIP,
                        (unsigned int)event.peer->address.port,
                        event.channelID,
                        (unsigned int)event.packet->dataLength,
                        (const char*)event.packet->data);
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                fprintf(stdout,
                        "Client disconnected: %s:%u\n",
                        peerIP,
                        (unsigned int)event.peer->address.port);
                break;

            default:
                fprintf(stderr, "Should never be reached");
            }
        }
        else if (retcode < 0)
        {
            /* failure */
            fprintf(stderr, "An error occurred while waiting for events.\n");
            break;
        }
        else
        {
            /* no event occurred */
        }
    }

Exit:
    if (server != NULL)
    {
        enet_host_destroy(server);
    }
    enet_deinitialize();
    
    return 0;
}


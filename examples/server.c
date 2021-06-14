#include <stdio.h>
#include <string.h>
#include <enet/enet.h>

typedef struct Client
{
    char ip[16];
    unsigned short port;
    unsigned int packet_count;
} Client;

int main(int argc, char **argv)
{
    int retcode;
    ENetAddress address;
    ENetHost *server = NULL;
    ENetEvent event;
    Client *client;
    ENetPacket *packet;
    int verbose = 0;

    if (argc > 1)
    {
        if (strcmp(argv[1], "-v") == 0)
            verbose = 1;
    }

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
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                /* Create a client */
                client = (Client*)calloc(1, sizeof(Client));
                client->ip[0] = '?';
                enet_address_get_host_ip(&(event.peer->address), client->ip, sizeof(client->ip));
                client->port = event.peer->address.port;
                /* Save the Client* in peer->data */
                event.peer->data = client;
                fprintf(stdout, 
                        "Client connected: %s:%u\n",
                        client->ip,
                        (unsigned int)client->port);
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                /* Get the Client* */
                client = (Client*)event.peer->data;
                if (verbose)
                {
                    fprintf(stdout,
                        "Packet received: %s:%u channel=%u length=%u data=\"%s\"\n",
                        client->ip,
                        (unsigned int)client->port,
                        event.channelID,
                        (unsigned int)event.packet->dataLength,
                        (const char*)event.packet->data);
                }
                /* Update packet_count */
                client->packet_count++;
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                /* Create a new packet. */
                packet = enet_packet_create(NULL, 50, ENET_PACKET_FLAG_RELIABLE);
                memset(packet->data, 0, 50);
                sprintf((char*)packet->data, "Hi, I have got the packet %u.", client->packet_count - 1);
                /* Send it back. */
                enet_peer_send(event.peer, event.channelID, packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                /* Get the Client* */
                client = (Client*)event.peer->data;
                fprintf(stdout,
                        "Client disconnected: %s:%u packet_count=%u\n",
                        client->ip,
                        (unsigned int)client->port,
                        client->packet_count);
                /* Destroy the client */
                event.peer->data = NULL;
                free(client);
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


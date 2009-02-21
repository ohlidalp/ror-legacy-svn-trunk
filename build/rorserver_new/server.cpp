/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as 
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "GetTime.h"
#include "rornetv2.h"
#include "nethelper.h"
#include <vector>

using namespace RakNet;

RakPeerInterface *peer=0;
typedef struct {
	SystemAddress sa;
	string name;
} client_t;

std::vector<client_t> clients;

char MSG3_NAMES[255][255] = {"ERROR", "MSG3_VERSION", "MSG3_USE_VEHICLE", "MSG3_BUFFER_SIZE", "MSG3_VEHICLE_DATA", "MSG3_HELLO", "MSG3_USER_CREDENTIALS", "MSG3_TERRAIN_RESP", "MSG3_SERVER_FULL", "MSG3_USER_BANNED", "MSG3_WRONG_SERVER_PW", "MSG3_WELCOME", "MSG3_CHAT", "MSG3_DELETE", "MSG3_GAME_CMD"};

// fill a buffer with random data
void random_numbers(unsigned int size, unsigned char *ptr)
{
    srand((unsigned)time(0));
    for(int index=0; index<(int)size; index++, ptr++)
        (*ptr) = int(255.0f*rand()/(RAND_MAX + 1.0f));
}

// hex 'editor like' display to check the content of a buffer
void display_buffer(unsigned int size, unsigned char *buf)
{
    unsigned int c1=1;
    printf("%06x ", 0);
    for(unsigned int c=0;c<size;c++, c1++)
    {
        printf("%02x ", *(buf+c));
        if(c1>20)
        {
            printf("\n%06x ", c);
            c1=0;
        }
    }
    printf("\n");
}


// get correct packet type, even with timestamp
unsigned char GetPacketIdentifier(Packet *p)
{
    if (p==0)
        return 255;

    if ((unsigned char)p->data[0] == ID_TIMESTAMP)
        return (unsigned char) p->data[sizeof(unsigned char) + sizeof(unsigned long)];
    else
        return (unsigned char) p->data[0];
}

void registerClient(SystemAddress sa, std::string name)
{
	client_t c;
	c.sa = sa;
	c.name = name;
	printf(" ** client registered: %s\n", c.name.c_str());
	clients.push_back(c);
}

void unregisterClient(SystemAddress sa)
{
	for(std::vector<client_t>::iterator it=clients.begin();it!=clients.end(); it++)
	{
		if(it->sa == sa)
		{
			printf(" ** client unregistered: %s\n", it->name.c_str());
			clients.erase(it);
			return;
		}
	}
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("usage: %s <IP to listen on>\n", argv[0]);
        return 1;
    }
    peer = RakNetworkFactory::GetRakPeerInterface();
    Packet *packet;
    peer->Startup(MAX_CLIENTS, 30, &SocketDescriptor(SERVER_PORT, argv[1]), 1);
    printf("Starting the server.\n");
    // We need to let the server accept incoming connections from the clients
    peer->SetMaximumIncomingConnections(MAX_CLIENTS);
    
    while (1)
    {
        packet=peer->Receive();
        if (packet)
        {
            switch (GetPacketIdentifier(packet))
            {
                case ID_REMOTE_DISCONNECTION_NOTIFICATION:
                    printf("Another client has disconnected.\n");
					unregisterClient(packet->systemAddress);
                    break;
                case ID_REMOTE_CONNECTION_LOST:
                    printf("Another client has lost the connection.\n");
					unregisterClient(packet->systemAddress);
                    break;
                case ID_REMOTE_NEW_INCOMING_CONNECTION:
                    printf("Another client has connected.\n");
                    break;
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    printf("Our connection request has been accepted.\n");
                
                    break;
                    
                case ROR_DATA_MSG:
                {
                    // The false is for efficiency so we don't make a copy of the passed data
					BitStream stream(packet->data, packet->length, false);
					
					// read basic stuff
					unsigned char useTimeStamp=0, typeId=0;
					unsigned long timeStamp=0, size=0;
					unsigned char contentType=0, contentSource=0;
					unsigned long contentSize;
					int mySize=0;
					stream.Read(useTimeStamp);
					stream.Read(timeStamp);
					stream.Read(typeId); // should be ROR_DATA_MSG
					stream.Read(contentType);
					stream.Read(contentSource);
					stream.Read(contentSize);
					if(contentType != MSG3_VEHICLE_DATA)
						printf("GOT message %s (%d) (%d bytes) from client %d, %s\n", MSG3_NAMES[contentType], contentType, contentSize, contentSource, packet->guid.ToString());
                    
                    // allocate buffer
                    //unsigned char *buf = (unsigned char *)malloc(mySize);
                    
                    // read buffer out of bitstream
                    //myBitStream.SerializeBits(false, buf, mySize*8);
                    // *8 because this is using bits instead of bytes
                    
					// broadcast
					peer->Send(&stream, MEDIUM_PRIORITY, RELIABLE_SEQUENCED, 0, packet->systemAddress, true);
                    // do something useful with the buffer here
                    // we just display it for now
                    //if(mySize<256)
                    //    display_buffer(mySize, buf);

                    //free(buf);
                }
                break;

                case ID_NEW_INCOMING_CONNECTION:
					{
                    printf("A connection is incoming.\n");
					printf("sending our server version\n");
					sendmessage(peer, packet->systemAddress, MSG3_HELLO, 0, (unsigned int)strlen(RORNETv2_VERSION), RORNETv2_VERSION);

					printf("sending user his ID: %d\n", packet->systemIndex);
					sendmessage(peer, packet->systemAddress, MSG3_WELCOME, 0, sizeof(unsigned short), (char *)&packet->systemIndex);
					}
					break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                    printf("The server is full.\n");
                    break;
                case ID_DISCONNECTION_NOTIFICATION:
                    printf("A client has disconnected.\n");
					unregisterClient(packet->systemAddress);
                    break;
                case ID_CONNECTION_LOST:
                    printf("A client lost the connection.\n");
					unregisterClient(packet->systemAddress);
                    break;
                default:
                    printf("Message with identifier %i has arrived.\n", packet->data[0]);
                    break;
            }

            peer->DeallocatePacket(packet);
        }
    }
    
    RakNetworkFactory::DestroyRakPeerInterface(peer);

    return 0;
}

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
#include <vector>

using namespace RakNet;

RakPeerInterface *peer=0;
typedef struct {
	SystemAddress sa;
	string name;
} client_t;

std::vector<client_t> clients;

void registerClient(RPCParameters *rpcParameters)
{
	client_t c;
	c.sa=rpcParameters->sender;
	c.name="(noname)";
	if(rpcParameters->input)
		c.name = string((char*)rpcParameters->input);
	printf(" ** client registered: %s\n", c.name.c_str());
	clients.push_back(c);
}

void getTerrainName(RPCParameters *rpcParameters)
{
	printf(" ** getTerrainName()");
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
    
	// register RPC stuff
    REGISTER_STATIC_RPC(peer, registerClient);
    
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
                
                    peer->RPC("PrintMessage",0,0,HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true, 0, UNASSIGNED_NETWORK_ID, 0);
                
                    break;
                    
                case RANDOM_DATA_MSG:
                {
                    // The false is for efficiency so we don't make a copy of the passed data
                    BitStream myBitStream(packet->data, packet->length, false);
                    
                    // read basic stuff
                    unsigned char useTimeStamp=0, typeId=0;
                    unsigned long timeStamp=0;
                    int mySize=0;
                    myBitStream.Read(useTimeStamp);
                    myBitStream.Read(timeStamp);
                    myBitStream.Read(typeId);
                    
                    // now our own data, first the size
                    myBitStream.Read(mySize);
					printf("server got binary data (%d bytes) from client %s\n", mySize, packet->guid.ToString());
                    
                    // allocate buffer
                    //unsigned char *buf = (unsigned char *)malloc(mySize);
                    
                    // read buffer out of bitstream
                    //myBitStream.SerializeBits(false, buf, mySize*8);
                    // *8 because this is using bits instead of bytes
                    
					// broadcast
					peer->Send(&myBitStream, MEDIUM_PRIORITY, RELIABLE_SEQUENCED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
                    // do something useful with the buffer here
                    // we just display it for now
                    //if(mySize<256)
                    //    display_buffer(mySize, buf);

                    //free(buf);
                }
                break;

                case ID_NEW_INCOMING_CONNECTION:
                    printf("A connection is incoming.\n");
					peer->RPC("PrintMessage",0,0,HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false, 0, UNASSIGNED_NETWORK_ID, 0);

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

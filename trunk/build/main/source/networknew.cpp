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
// created by thomas fischer 18th of january 2009
#ifdef NEWNET

#include "networknew.h"
#include <stdio.h>
#include <string.h>
#include "RakNetworkFactory.h"

#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "GetTime.h"
#include "Settings.h"

using namespace std;
using namespace Ogre;
using namespace RakNet;

NetworkNew::NetworkNew(Beam **btrucks, Ogre::String servername, long sport, ExampleFrameListener *efl) : NetworkBase(btrucks, servername, sport, efl), peer(0)
{
	LogManager::getSingleton().logMessage("NetworkNew::NetworkNew()");
	for (int i=0; i<MAX_PEERS; i++) clients[i].used=false;
	
	myServerName = servername;
	myServerPort = sport;
}

NetworkNew::~NetworkNew()
{
	LogManager::getSingleton().logMessage("NetworkNew::~NetworkNew()");

}


bool NetworkNew::vehicle_to_spawn(char* name, unsigned int *uid, unsigned int *label)
{
	LogManager::getSingleton().logMessage("NetworkNew::vehicle_to_spawn()");
	return false;
}

client_t NetworkNew::vehicle_spawned(unsigned int uid, int trucknum)
{
	LogManager::getSingleton().logMessage("NetworkNew::vehicle_to_spawn()");
	client_t res;
	res.authed=false;
	return res;
}

void NetworkNew::sendVehicleType(char* name, int numnodes)
{
	LogManager::getSingleton().logMessage("NetworkNew::sendVehicleType()");
}

void NetworkNew::sendData(Beam* truck)
{
	LogManager::getSingleton().logMessage("NetworkNew::sendData()");
}

void NetworkNew::sendChat(char* line)
{
	LogManager::getSingleton().logMessage("NetworkNew::sendChat()");
}

bool NetworkNew::connect()
{
	LogManager::getSingleton().logMessage("NetworkNew::connect()");

    peer = RakNetworkFactory::GetRakPeerInterface();
    
	Packet *packet;
    peer->Startup(1, 30, &SocketDescriptor(), 1);
	peer->Connect(myServerName.c_str(), myServerPort, 0, 0);

	char *nickname = const_cast<char*>(SETTINGS.getSetting("Nickname").c_str());

    //REGISTER_CLASS_MEMBER_RPC(peer, NetworkNew, printMessage);
    while (1)
    {
        packet=peer->Receive();

        if (packet)
        {
            switch (getPacketIdentifier(packet))
            {
                case ID_CONNECTION_ATTEMPT_FAILED:
                    LogManager::getSingleton().logMessage("connection failed");
                    return 1;
                    break;
                case ID_REMOTE_DISCONNECTION_NOTIFICATION:
                    LogManager::getSingleton().logMessage("Another client has disconnected.");
                    break;
                case ID_REMOTE_CONNECTION_LOST:
                    LogManager::getSingleton().logMessage("Another client has lost the connection.");
                    break;
                case ID_REMOTE_NEW_INCOMING_CONNECTION:
                    LogManager::getSingleton().logMessage("Another client has connected.");
                    break;
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    LogManager::getSingleton().logMessage("Our connection request has been accepted.");
                
                    //peer->RPC("registerClient", nickname, (int)(strlen(nickname)+1)*8,HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true, 0, UNASSIGNED_NETWORK_ID, 0);
                    // send random data :D
                    //send_random_data(300, packet->systemAddress);
                    //send_random_data(300, packet->systemAddress);
                    //send_random_data(300, packet->systemAddress);
                    //send_random_data(300, packet->systemAddress);
                    //send_random_data(300, packet->systemAddress);
                    //send_random_data(6000, packet->systemAddress);
                    break;
                
                case ROR_DATA_MSG:
                {
                    // see server for more documentation
                    // The false is for efficiency so we don't make a copy of the passed data
                    BitStream myBitStream(packet->data, packet->length, false);
                    unsigned char useTimeStamp=0, typeId=0;
                    unsigned long timeStamp=0;
                    int mySize=0;
                    myBitStream.Read(useTimeStamp);
                    myBitStream.Read(timeStamp);
                    myBitStream.Read(typeId);
                    myBitStream.Read(mySize);
					LogManager::getSingleton().logMessage("client got binary data " + StringConverter::toString(mySize) + " from client " + packet->guid.ToString());
                    
                    unsigned char *buf = (unsigned char *)malloc(mySize);
                    myBitStream.SerializeBits(false, buf, mySize*8);
                    // *8 because this is using bits instead of bytes
                    
                    // do something useful with the buffer here
                    // we just display it for now
                    //if(mySize<256)
                    //    display_buffer(mySize, buf);
                    
                    free(buf);
                }
                break;

                case ID_NEW_INCOMING_CONNECTION:
                    LogManager::getSingleton().logMessage("A connection is incoming.");
                    break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                    LogManager::getSingleton().logMessage("The server is full.");
                    break;
                case ID_DISCONNECTION_NOTIFICATION:
                    LogManager::getSingleton().logMessage("We have been disconnected.");
                    break;
                case ID_CONNECTION_LOST:
                    LogManager::getSingleton().logMessage("Connection lost.");
                    break;
                default:
					LogManager::getSingleton().logMessage("Message with identifier " + StringConverter::toString(packet->data[0]) + " has arrived.");
                    break;
            }
            peer->DeallocatePacket(packet);
        }
    }
    RakNetworkFactory::DestroyRakPeerInterface(peer);

	return true;
}

void NetworkNew::disconnect()
{
	LogManager::getSingleton().logMessage("NetworkNew::disconnect()");

}

int NetworkNew::rconlogin(char* rconpasswd)
{
	LogManager::getSingleton().logMessage("NetworkNew::rconlogin()");
	return 0;
}

int NetworkNew::rconcommand(char* rconcmd)
{
	LogManager::getSingleton().logMessage("NetworkNew::rconcommand()");
	return 0;
}

int NetworkNew::getConnectedClientCount()
{
	LogManager::getSingleton().logMessage("NetworkNew::getConnectedClientCount()");
	return 0;
}

char *NetworkNew::getTerrainName()
{
	LogManager::getSingleton().logMessage("NetworkNew::getTerrainName()");
    //peer->RPC("getTerrainName", 0, 0, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, false, 0, UNASSIGNED_NETWORK_ID, 0);
	return "nhelens";
}

char *NetworkNew::getNickname()
{
	LogManager::getSingleton().logMessage("NetworkNew::getNickname()");
	return "notimplemented";
}

int NetworkNew::getRConState()
{
	LogManager::getSingleton().logMessage("NetworkNew::getRConState()");
	return 0;
}

/*
void __cdecl NetworkNew::printMessage(RPCParameters *rpcParameters)
{
	LogManager::getSingleton().logMessage("NetworkNew::printMessage()");
    printf("Hello World (from the server)!\n");
}
*/

// get correct packet type, even with timestamp
unsigned char NetworkNew::getPacketIdentifier(Packet *p)
{
    if (p==0)
        return 255;

    if ((unsigned char)p->data[0] == ID_TIMESTAMP)
        return (unsigned char) p->data[sizeof(unsigned char) + sizeof(unsigned long)];
    else
        return (unsigned char) p->data[0];
}


#if 0
// old code

/// generates some random data and displays it - ignore it!
void generateTestData()
{
    unsigned int size = 256;
    unsigned char *buf = (unsigned char *)malloc(sizeof(unsigned int) * size);
    memset(buf, sizeof(unsigned char)*size, 0);
    random_numbers(size, buf);
    display_buffer(size, buf);
}

/// this generates and sends random data :)
void send_random_data(unsigned long size, SystemAddress sa)
{
    printf("sending binary data (%d bytes)\n", size);
    
    // create a bitstream
    BitStream myBitStream;
    // write basics into it
    // fist that its a timestamped packet
    myBitStream.Write((unsigned char) ID_TIMESTAMP);
    // second the timestamp itself
    myBitStream.Write((unsigned long) RakNet::GetTime());
    // third our message ID
    myBitStream.Write((unsigned char) RANDOM_DATA_MSG);
    
    // then our own data, in this case some size field
    myBitStream.Write((int) size);
    
    // allocate buffer
    unsigned char *buf = (unsigned char *)malloc(sizeof(unsigned char) * size);
    memset(buf, sizeof(unsigned char)*size, 0);
    // fill buffer randomly
    random_numbers(size, buf);
    
    // display buffer if small enoug to fit on screen
    if(size<256)
        display_buffer(size, buf);
    
    // copy buffer into the bitstream
    myBitStream.SerializeBits(true, buf, size*8);
    
    // send the whole thing
	peer->Send(&myBitStream, MEDIUM_PRIORITY, RELIABLE_SEQUENCED, 0, sa, false);
	//peer->Send(&myBitStream, MEDIUM_PRIORITY, RELIABLE_SEQUENCED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
    
    // free buffer
    free(buf);
}
int main(int argc, char **argv)
{
  
}
#endif //0

#endif //NEWNET

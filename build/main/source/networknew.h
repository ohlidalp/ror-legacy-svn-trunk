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

#ifndef NETWORKNEW_H
#define NETWORKNEW_H

#include "network.h"
#include "RakPeerInterface.h"

#define ROR_DATA_MSG ID_USER_PACKET_ENUM

class NetworkNew : public NetworkBase
{
protected:
	RakPeerInterface *peer;
	
	client_t clients[MAX_PEERS];
	Ogre::String myServerName;
	long myServerPort;


public:
	NetworkNew(Beam **btrucks, std::string servername, long sport, ExampleFrameListener *efl);
	~NetworkNew();

	//external call to check if a vehicle is to be spawned
	bool vehicle_to_spawn(char* name, unsigned int *uid, unsigned int *label);
	client_t vehicle_spawned(unsigned int uid, int trucknum);
	
	//external call to set vehicle type
	void sendVehicleType(char* name, int numnodes);
	
	//external call to send vehicle data
	void sendData(Beam* truck);
	void sendChat(char* line);
	bool connect();
	void disconnect();
	int rconlogin(char* rconpasswd);
	int rconcommand(char* rconcmd);

	int getConnectedClientCount();

	char *getTerrainName();
	char *getNickname();
	int getRConState();


	// custom functions
	//void __cdecl printMessage(RPCParameters *rpcParameters); /// this is the RPC method to be called by peers
	unsigned char getPacketIdentifier(Packet *p);
};

#endif //NETWORKNEW_H

#endif //NEWNET

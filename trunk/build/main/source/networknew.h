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
#include "BitStream.h"
#include "rornetv2.h"

class NetworkNew : public NetworkBase
{
protected:
	RakPeerInterface *peer;
	
	client_t clients[MAX_PEERS];
	Ogre::String myServerName;
	long myServerPort;


public:
	NetworkNew(Beam **btrucks, Ogre::String servername, long sport, ExampleFrameListener *efl);
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


	void sendthreadstart();
	void receivethreadstart();
	pthread_mutex_t send_work_mutex;
	pthread_cond_t send_work_cv;
	pthread_mutex_t clients_mutex;
	pthread_mutex_t chat_mutex;
	pthread_t sendthread;
	pthread_t receivethread;
	bool shutdown;
	char* send_buffer;
	int send_buffer_len;
	int last_time;
	Timer timer;
	oob2_t send_oob;
	unsigned myuid;
	Beam** trucks;
	ExampleFrameListener *mefl;

	char terrainName[255];
	char ourTruckname[255];

	void handlePacket(unsigned char type, unsigned char source, unsigned long size, char *buffer);

	SoundScriptManager* soundManager;
	char sendthreadstart_buffer[MAX_MESSAGE_LENGTH];
	void netFatalError(String error, bool exit=true);

	SystemAddress serverAddress;
};

#endif //NETWORKNEW_H

#endif //NEWNET

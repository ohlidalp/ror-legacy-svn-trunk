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
#ifndef __Network_H__
#define __Network_H__
#include "Ogre.h"
using namespace Ogre;

#include "SocketW.h"
#include "rornet.h"
#include "pthread.h"
#include "Beam.h"
#include "engine.h"
#include "SoundScriptManager.h"
#include "networkinfo.h"


class ExampleFrameListener;


class ExampleFrameListener;

class NetworkBase
{
public:
	NetworkBase(Beam **btrucks, Ogre::String servername, long sport, ExampleFrameListener *efl) {};
	~NetworkBase() {};

	//external call to check if a vehicle is to be spawned
	virtual bool vehicle_to_spawn(char* name, unsigned int *uid, unsigned int *label) = 0;
	virtual int vehicle_spawned(unsigned int uid, int trucknum, client_t &client) = 0;
	
	//external call to set vehicle type
	virtual void sendVehicleType(char* name, int numnodes) = 0;
	
	//external call to send vehicle data
	virtual void sendData(Beam* truck) = 0;
	virtual void sendChat(char* line) = 0;
	virtual bool connect() = 0;
	virtual void disconnect() = 0;
	virtual int rconlogin(char* rconpasswd) = 0;
	virtual int rconcommand(char* rconcmd) = 0;

	virtual int getConnectedClientCount() = 0;

	virtual char *getTerrainName() = 0;
	virtual Ogre::String getNickname(bool colour=false) = 0;
	virtual int getRConState() = 0;
	virtual int getSpeedUp() = 0;
	virtual int getSpeedDown() = 0;
};

class Network : public NetworkBase
{
private:
	SWInetSocket socket;
	unsigned int myuid;
	int myauthlevel;
	pthread_t sendthread;
	pthread_t receivethread;
	Timer timer;
	int last_time;
	int speed_time;
	int speed_bytes_sent, speed_bytes_sent_tmp, speed_bytes_recv, speed_bytes_recv_tmp;
	char* send_buffer;
	int send_buffer_len;
	oob_t send_oob;
	pthread_mutex_t send_work_mutex;
	pthread_cond_t send_work_cv;
	client_t clients[MAX_PEERS];
	pthread_mutex_t clients_mutex;
	pthread_mutex_t chat_mutex;
	Beam** trucks;
	netlock_t netlock;
	std::string mySname;
	long mySport;
	char sendthreadstart_buffer[MAX_MESSAGE_LENGTH];
	pthread_mutex_t msgsend_mutex;
	ExampleFrameListener *mefl;
	char terrainName[255];
	bool requestTerrainName();
	Ogre::String nickname;
	int rconauthed;
	bool shutdown;
	SoundScriptManager* ssm;
	Ogre::String getUserChatName(client_t *c);
	void calcSpeed();
public:

	Network(Beam **btrucks, std::string servername, long sport, ExampleFrameListener *efl);
	int sendmessage(SWInetSocket *socket, int type, unsigned int len, char* content);
	int receivemessage(SWInetSocket *socket, int *type, int *source, unsigned int *wrotelen, char* content, unsigned int bufferlen);
	void sendthreadstart();
	void receivethreadstart();
	//external call to check if a vehicle is to be spawned
	bool vehicle_to_spawn(char* name, unsigned int *uid, unsigned int *label);
	int vehicle_spawned(unsigned int uid, int trucknum, client_t &client);
	//external call to set vehicle type
	void sendVehicleType(char* name, int numnodes);
	//external call to send vehicle data
	void sendData(Beam* truck);
	void sendChat(char* line);
	void netFatalError(String error, bool exit=true);
	~Network();
	bool connect();
	void disconnect();
	int rconlogin(char* rconpasswd);
	int rconcommand(char* rconcmd);

	int getConnectedClientCount();

	char *getTerrainName() { return terrainName; };
	Ogre::String getNickname(bool colour=false);
	int getRConState() { return rconauthed; };
	int downloadMod(char* modname);


	int getSpeedUp();
	int getSpeedDown();
};


#endif

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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of August 2009

#ifndef STREAMABLE_H__
#define STREAMABLE_H__

#include "OgrePrerequisites.h"
#include "OgreSingleton.h"
#include "pthread.h"
#include "SocketW.h"

#include <map>

#define MAX_PACKET_LEN 8192

class Network;

typedef struct _bufferedPacket
{
	char packetBuffer[MAX_PACKET_LEN];
	unsigned int size;
} bufferedPacket_t;

/**
 * This class defines a standart interface and a buffer between the actual network code and the class that handles it.
 * The buffer must be decoupled from the separatly running network thread.
 */
class Streamable
{
	friend class NetworkStreamManager;
public:
	Streamable();
protected:
	static const int packetBufferSize = 10;
	static const int maxPacketLen = MAX_PACKET_LEN;
	
	void sendStreamData();
	virtual void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len) = 0;

	void addPacket(int type, int uid, unsigned int streamid, unsigned int len, char* content);
	std::queue < bufferedPacket_t > *getPacketQueue();

	std::queue < bufferedPacket_t > packets;

	unsigned int streamid;
};


class NetworkStreamManager : public Ogre::Singleton< NetworkStreamManager >
{
	friend class Network;
public:
	NetworkStreamManager();
	~NetworkStreamManager();
	static NetworkStreamManager& getSingleton(void);
	static NetworkStreamManager* getSingletonPtr(void);
	
	void addStream(Streamable *stream, int source=-1, int streamid=-1);
	void removeStream(Streamable *stream);
	void pauseStream(Streamable *stream);
	void resumeStream(Streamable *stream);
	
	void triggerSend();
	void sendStreams(Network *net, SWInetSocket *socket);

protected:
	pthread_mutex_t send_work_mutex;
	pthread_cond_t send_work_cv;
	Network *net;

	std::map < int, std::map < unsigned int, Streamable *> > streams;
	unsigned int streamid;

	void pushReceivedStreamMessage(unsigned int &type, int &source, unsigned int &streamid, unsigned int &wrotelen, char *buffer);
};



#endif //STREAMABLE_H__

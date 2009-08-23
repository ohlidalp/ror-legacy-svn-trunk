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
#include <map>

class Network;
class NetworkStreamManager;

/**
 * This class defines a standart interface and a buffer between the actual network code and the class that handles it.
 * The buffer must be decoupled from the separatly running network thread.
 */
class Streamable
{
	friend class NetworkStreamManager;
protected:
	// constructor/destructor are protected, so you cannot create instances without using the factory
	Streamable();
	~Streamable();

	// static const members
	static const unsigned int packetBufferSize = 10;
	static const unsigned int maxPacketLen     = 8192;

	// custom types
	typedef struct _bufferedPacket
	{
		char packetBuffer[maxPacketLen];
		unsigned int size;
	} bufferedPacket_t;

	// normal members
	std::queue < bufferedPacket_t > packets;
	unsigned int streamid;

	// virtual interface methods
	virtual void sendStreamData() = 0;
	virtual void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len) = 0;

	// base class methods
	void addPacket(int type, int uid, unsigned int streamid, unsigned int len, char* content);
	std::queue < bufferedPacket_t > *getPacketQueue();
};

#endif //STREAMABLE_H__

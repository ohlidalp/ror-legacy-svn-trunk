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

#include "Streamable.h"
#include "NetworkStreamManager.h"

#include "Ogre.h"
#include "network.h"
#include "utils.h"
#include "sha1.h"
#include "pthread.h"

using namespace Ogre;

Streamable::Streamable()
{
	//NetworkStreamManager::getSingleton().addStream(this);
}

Streamable::~Streamable()
{
}

std::queue < Streamable::bufferedPacket_t > *Streamable::getPacketQueue()
{
	return &packets;
}

void Streamable::addPacket(int type, int uid, unsigned int streamid, unsigned int len, char* content)
{
	if(packets.size() > packetBufferSize)
		// buffer full, packet discarded
		return;

	bufferedPacket_t packet;
	memset(&packet, 0, sizeof(bufferedPacket_t));

	// allocate buffer
	//packet.packetBuffer = (char*)calloc(len, sizeof(char));

	char *buffer = (char*)(packet.packetBuffer);

	// write header in buffer
	header_t *head = (header_t *)buffer;
	head->command  = type;
	head->source   = uid;
	head->size     = len;
	head->streamid = streamid;

	// then copy the contents
	char *bufferContent = (char *)(buffer + sizeof(header_t));
	memcpy(bufferContent, content, len);

	// record the packet size
	packet.size = len + sizeof(header_t);

	/*
	String header_hex = hexdump(buffer, sizeof(header_t));
	String content_hex = hexdump((buffer + sizeof(header_t)), len);

	LogManager::getSingleton().logMessage("header:  " + header_hex);
	LogManager::getSingleton().logMessage("content: " + content_hex);
	*/

	/*
	char hash[255] = "";
	RoR::CSHA1 sha1;
	sha1.UpdateHash((uint8_t *)bufferContent, len);
	sha1.Final();
	sha1.ReportHash(hash, RoR::CSHA1::REPORT_HEX_SHORT);
	LogManager::getSingleton().logMessage("S|HASH: " + String(hash));
	*/

	packets.push(packet);

	// trigger buffer clearing
	NetworkStreamManager::getSingleton().triggerSend();
}


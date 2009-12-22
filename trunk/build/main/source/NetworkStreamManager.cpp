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

#include "NetworkStreamManager.h"
#include "Streamable.h"

#include "Ogre.h"
#include "network.h"
#include "utils.h"
#include "sha1.h"

using namespace Ogre;

NetworkStreamManager::NetworkStreamManager()
{
	streamid=10;
	pthread_mutex_init(&send_work_mutex, NULL);
	pthread_cond_init(&send_work_cv, NULL);
}

NetworkStreamManager::~NetworkStreamManager()
{
}

template<> NetworkStreamManager * Singleton< NetworkStreamManager >::ms_Singleton = 0;
NetworkStreamManager* NetworkStreamManager::getSingletonPtr(void)
{
	return ms_Singleton;
}
NetworkStreamManager& NetworkStreamManager::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

void NetworkStreamManager::addLocalStream(Streamable *stream, stream_register_t *reg, unsigned int size)
{
	// for own streams: count stream id up ...
	int mysourceid = net->getUserID();
	
	// use counting streamid
	stream->setStreamID(streamid);
	
	// add new stream map to the streams map
	if(streams.find(mysourceid) == streams.end())
		streams[mysourceid] = std::map < unsigned int, Streamable *>();
	// map the stream
	streams[mysourceid][streamid] = stream;
	LogManager::getSingleton().logMessage("adding local stream: " + StringConverter::toString(mysourceid) + ":"+ StringConverter::toString(streamid) + ", type: " + StringConverter::toString(reg->type));
	// send stream setup notice to server
	if(size == 0) size = sizeof(stream_register_t);
	stream->addPacket(MSG2_STREAM_REGISTER, size, (char*)reg);

	// increase stream counter
	streamid++;
}

void NetworkStreamManager::addRemoteStream(Streamable *stream, int rsource, int rstreamid)
{
	stream->streamid = rstreamid;
	streams[rsource][rstreamid] = stream;
	LogManager::getSingleton().logMessage("adding remote stream: " + StringConverter::toString(rsource) + ":"+ StringConverter::toString(rstreamid));
}

void NetworkStreamManager::removeStream(Streamable *stream)
{
}

void NetworkStreamManager::pauseStream(Streamable *stream)
{
}

void NetworkStreamManager::resumeStream(Streamable *stream)
{
}

void NetworkStreamManager::removeUser(int sourceID)
{
	if(streams.find(sourceID) == streams.end())
		// no such stream?!
		return;
	// found and deleted
	streams.erase(streams.find(sourceID));

	// now iterate over all factories and remove their instances
	std::vector < StreamableFactoryInterface * >::iterator it;
	for(it=factories.begin(); it!=factories.end(); it++)
	{
		(*it)->deleteRemote(sourceID, -1); // -1 = all streams
	}
}

void NetworkStreamManager::pushReceivedStreamMessage(header_t header, char *buffer)
{
	if(streams.find(header.source) == streams.end())
		// no such stream?!
		return;
	if(streams.find(header.source)->second.find(header.streamid) == streams.find(header.source)->second.end())
		// no such stream?!
		return;

	streams[header.source][header.streamid]->addReceivedPacket(header, buffer);
}

void NetworkStreamManager::triggerSend()
{
	pthread_mutex_lock(&send_work_mutex);
	pthread_cond_broadcast(&send_work_cv);
	pthread_mutex_unlock(&send_work_mutex);
}

void NetworkStreamManager::sendStreams(Network *net, SWInetSocket *socket)
{
	pthread_mutex_lock(&send_work_mutex);
	pthread_cond_wait(&send_work_cv, &send_work_mutex);
	pthread_mutex_unlock(&send_work_mutex);

	char *buffer = 0;
	int bufferSize=0;

	std::map < int, std::map < unsigned int, Streamable *> >::iterator it;
	for(it=streams.begin(); it!=streams.end(); it++)
	{
		std::map<unsigned int,Streamable *>::iterator it2;
		for(it2=it->second.begin(); it2!=it->second.end(); it2++)
		{
			std::deque <Streamable::bufferedPacket_t> *packets = it2->second->getPacketQueue();

			if(packets->empty())
				continue;

			// remove oldest packet in queue
			Streamable::bufferedPacket_t packet = packets->front();

			// handle always one packet
			int etype = net->sendMessageRaw(socket, packet.packetBuffer, packet.size);
		
			packets->pop_front();

			if (etype)
			{
				char emsg[256];
				sprintf(emsg, "Error %i while sending data packet", etype);
				net->netFatalError(emsg);
				return;
			}
		}
	}
}

void NetworkStreamManager::update()
{
	syncRemoteStreams();
	receiveStreams();
}

void NetworkStreamManager::syncRemoteStreams()
{
	// iterate over all factories
	std::vector < StreamableFactoryInterface * >::iterator it;
	for(it=factories.begin(); it!=factories.end(); it++)
	{
		(*it)->syncRemoteStreams();
	}
}

void NetworkStreamManager::receiveStreams()
{
	char *buffer = 0;
	int bufferSize=0;

	std::map < int, std::map < unsigned int, Streamable *> >::iterator it;
	for(it=streams.begin(); it!=streams.end(); it++)
	{
		std::map<unsigned int,Streamable *>::iterator it2;
		for(it2=it->second.begin(); it2!=it->second.end(); it2++)
		{
			it2->second->lockReceiveQueue();
			std::deque <recvPacket_t> *packets = it2->second->getReceivePacketQueue();

			if(packets->empty())
			{
				it2->second->unlockReceiveQueue();
				continue;
			}

			// remove oldest packet in queue
			recvPacket_t packet = packets->front();

			Network::debugPacket("receive-2", &packet.header, (char *)packet.buffer);

			// handle always one packet
			it2->second->receiveStreamData(packet.header.command, packet.header.source, packet.header.streamid, (char*)packet.buffer, packet.header.size);
		
			packets->pop_front();
			it2->second->unlockReceiveQueue();
		}
	}
}

void NetworkStreamManager::addFactory(StreamableFactoryInterface *factory)
{
	this->factories.push_back(factory);
}


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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 12th of August 2009

#ifndef STREAMABLEFACTORY_H__
#define STREAMABLEFACTORY_H__

#include "OgrePrerequisites.h"
#include "pthread.h"
#include "SocketW.h"
#include "rornet.h"
#include "NetworkStreamManager.h"
#include "StreamableFactoryInterface.h"
#include <map>

class Network;
class Streamable;


template<class T, class X> class StreamableFactory : public StreamableFactoryInterface
{
	friend class Network;
public:
	// constructor, destructor and singleton
	StreamableFactory( void )
	{
		assert( !ms_Singleton );
		ms_Singleton = static_cast< T* >( this );
		pthread_mutex_init(&stream_reg_mutex, NULL);

		// add self to factory list
		NetworkStreamManager::getSingleton().addFactory(this);
	}

	~StreamableFactory( void )
	{
		assert( ms_Singleton );
		ms_Singleton = 0;
	}

	static T& getSingleton( void )
	{
		assert( ms_Singleton );
		return ( *ms_Singleton );
	}

	static T* getSingletonPtr( void )
	{
		return ms_Singleton;
	}

	// useful functions
	virtual X *createLocal(int colour) = 0;
	virtual void netUserAttributesChanged(int source, int streamid) = 0;
	virtual void createRemoteInstance(stream_reg_t *reg) = 0;

	// common functions
	void createRemote(int sourceid, int streamid, stream_register_t *reg, int colour)
	{
		lockStreams();

		stream_reg_t registration;
		registration.sourceid = sourceid;
		registration.streamid = streamid;
		registration.reg      = *reg; // really store the data
		registration.colour   = colour;
		stream_registrations.push_back(registration);

		unlockStreams();
	}

	void deleteRemote(int sourceid, int streamid)
	{
		lockStreams();

		stream_del_t deletion;
		deletion.sourceid = sourceid;
		deletion.streamid = streamid;
		stream_deletions.push_back(deletion);

		unlockStreams();
	}

	virtual void syncRemoteStreams()
	{
		lockStreams();
		// first registrations
		while (!stream_registrations.empty())
		{
			stream_reg_t reg = stream_registrations.front();
			createRemoteInstance(&reg);
			stream_registrations.pop_front();
		}
		// then deletions:
		// first registrations
		while (!stream_deletions.empty())
		{
			stream_del_t del = stream_deletions.front();
			removeInstance(&del);
			stream_deletions.pop_front();
		}
		unlockStreams();
	}

	void removeInstance(stream_del_t *del)
	{
		std::map < int, std::map < unsigned int, X *> >::iterator it1;
		std::map < unsigned int, X *>::iterator it2;

		for(it1=streamables.begin(); it1!=streamables.end();it1++)
		{
			if(it1->first != del->sourceid) continue;

			for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
			{
				if(del->streamid == -1 || del->streamid == it2->first)
				{
					// deletes the stream
					delete it2->second;
					it2->second = 0;
				}
			}
			break;
		}
	}

protected:
	static T* ms_Singleton;
	pthread_mutex_t stream_reg_mutex;

	std::map < int, std::map < unsigned int, X *> > streamables;
	std::deque < stream_reg_t > stream_registrations;
	std::deque < stream_del_t > stream_deletions;

	void lockStreams()
	{
		pthread_mutex_lock(&stream_reg_mutex);
	}

	void unlockStreams()
	{
		pthread_mutex_unlock(&stream_reg_mutex);
	}
};



#endif //STREAMABLEFACTORY_H__

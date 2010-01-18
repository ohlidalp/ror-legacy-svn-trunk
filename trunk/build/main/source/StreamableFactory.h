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

// this is the master swith to debug the stream locking/unlocking
//#define DEBUGSTREAMFACTORIES

#define OGREFUNCTIONSTRING  String(__FUNCTION__)+" @ "+String(__FILE__)+":"+StringConverter::toString(__LINE__)

#ifdef DEBUGSTREAMFACTORIES
# define LOCKSTREAMS()       do { LogManager::getSingleton().logMessage("***LOCK:   "+OGREFUNCTIONSTRING); lockStreams();   } while(0)
# define UNLOCKSTREAMS()     do { LogManager::getSingleton().logMessage("***UNLOCK: "+OGREFUNCTIONSTRING); unlockStreams(); } while(0)
# ifdef WIN32
// __debugbreak will break into the debugger in visual studio
#  define MYASSERT(x)       do { if(!x) { LogManager::getSingleton().logMessage("***ASSERT FAILED: "+OGREFUNCTIONSTRING); __debugbreak(); }; } while(0)
# else //!WIN32
#  define MYASSERT(x)       assert(x)
# endif //WIN32
#else //!DEBUGSTREAMFACTORIES
# define LOCKSTREAMS()       lockStreams();
# define UNLOCKSTREAMS()     unlockStreams();
# define MYASSERT(x)         ((void)0)
#endif //DEBUGSTREAMFACTORIES

template<class T, class X> class StreamableFactory : public StreamableFactoryInterface
{
	friend class Network;
public:
	// constructor, destructor and singleton
	StreamableFactory( void ) : locked(false)
	{
		MYASSERT( !ms_Singleton );
		ms_Singleton = static_cast< T* >( this );
		pthread_mutex_init(&stream_reg_mutex, NULL);

		// add self to factory list
		NetworkStreamManager::getSingleton().addFactory(this);
	}

	~StreamableFactory( void )
	{
		MYASSERT( ms_Singleton );
		ms_Singleton = 0;
	}

	static T& getSingleton( void )
	{
		MYASSERT( ms_Singleton );
		return ( *ms_Singleton );
	}

	static T* getSingletonPtr( void )
	{
		return ms_Singleton;
	}

	// useful functions
	virtual X *createLocal(int colour) = 0;
	virtual void netUserAttributesChanged(int source, int streamid) = 0;
	virtual X *createRemoteInstance(stream_reg_t *reg) = 0;

	// common functions
	void createRemote(int sourceid, int streamid, stream_register_t *reg, int colour)
	{
		LOCKSTREAMS();

		stream_reg_t registration;
		registration.sourceid = sourceid;
		registration.streamid = streamid;
		registration.reg      = *reg; // really store the data
		registration.colour   = colour;
		stream_registrations.push_back(registration);

		UNLOCKSTREAMS();
	}

	void deleteRemote(int sourceid, int streamid)
	{
		LOCKSTREAMS();

		stream_del_t deletion;
		deletion.sourceid = sourceid;
		deletion.streamid = streamid;
		stream_deletions.push_back(deletion);

		UNLOCKSTREAMS();
	}

	virtual void syncRemoteStreams()
	{
		LOCKSTREAMS();
		// first registrations
		while (!stream_registrations.empty())
		{
			stream_reg_t reg = stream_registrations.front();
			Streamable *s = createRemoteInstance(&reg);
			if(s)
			{
				// add it to the streams list
				NetworkStreamManager::getSingleton().addRemoteStream(s, reg.sourceid, reg.streamid);
			}
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
		UNLOCKSTREAMS();
	}

	void removeInstance(stream_del_t *del)
	{
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

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
	bool locked;

	std::deque < stream_reg_t > stream_registrations;
	std::deque < stream_del_t > stream_deletions;

	std::map < int, std::map < unsigned int, X *> > &getStreams()
	{
		// ensure we only access the map when we locked it before
		MYASSERT(locked);
		return mStreamables;
	}

	void lockStreams()
	{
		// double locking is not healty!
		//MYASSERT(!this->locked);
		pthread_mutex_lock(&stream_reg_mutex);
		this->locked=true;
	}

	void unlockStreams()
	{
		pthread_mutex_unlock(&stream_reg_mutex);
		this->locked=false;
	}

private:
	// no direct access to it, helps with locking it before using it
	std::map < int, std::map < unsigned int, X *> > mStreamables;

};



#endif //STREAMABLEFACTORY_H__

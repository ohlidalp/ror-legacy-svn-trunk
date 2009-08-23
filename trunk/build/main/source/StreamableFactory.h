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
#include <map>

class Network;
class Streamable;

template<class T, class X> class StreamableFactory
{
	friend class Network;
public:
	// constructor, destructor and singleton
	StreamableFactory( void )
	{
		assert( !ms_Singleton );
		ms_Singleton = static_cast< T* >( this );
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
	virtual X *createLocal() = 0;
	virtual X *createRemote(int sourceid, stream_register_t *reg, int slotid) = 0;
	virtual void netUserAttributesChanged(int source, int streamid) = 0;
	virtual void remove(X *stream) = 0;
	virtual void removeUser(int userid) = 0;

protected:
	static T* ms_Singleton;
	std::map < int, std::map < unsigned int, X *> > streamables;
};



#endif //STREAMABLEFACTORY_H__

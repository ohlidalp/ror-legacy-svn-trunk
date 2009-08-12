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
#include "OgreSingleton.h"
#include "pthread.h"
#include "SocketW.h"
#include "rornet.h"
#include <map>

class Network;
class Streamable;

class StreamableFactory : public Ogre::Singleton< StreamableFactory >
{
	friend class Network;
public:
	StreamableFactory();
	~StreamableFactory();
	static StreamableFactory& getSingleton(void);
	static StreamableFactory* getSingletonPtr(void);
	
	Streamable *createLocal();
	Streamable *createRemote(stream_register_t *reg);
	
	void remove(Streamable *stream);
	void removeUser(int userid);

protected:
	std::map < int, std::map < unsigned int, Streamable *> > streamables;
};



#endif //STREAMABLEFACTORY_H__

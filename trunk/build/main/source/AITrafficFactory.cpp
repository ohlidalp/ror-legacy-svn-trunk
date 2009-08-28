/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer
Copyright 2009 Imre, Nagy (mulder@nebulon.hu)

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

#ifdef AITRAFFIC
#include "AITrafficFactory.h"
#include "Ogre.h"

using namespace Ogre;

template<> AITrafficFactory *StreamableFactory < AITrafficFactory, AITraffic >::ms_Singleton = 0;

AITrafficFactory::AITrafficFactory(Network *net, Ogre::SceneManager *scm) : net(net), scm(scm)
{
	traffic = NULL;
}

AITrafficFactory::~AITrafficFactory()
{}

void AITrafficFactory::netUserAttributesChanged(int source, int streamid)
{}

void AITrafficFactory::localUserAttributesChanged(int newid)
{}

AITraffic *AITrafficFactory::createLocal()
{
	assert(false);
	return NULL;
}

AITraffic *AITrafficFactory::createRemote(int sourceid, stream_register_t *reg, int slotid)
{
	Ogre::LogManager::getSingleton().logMessage(" new Traffic management for " + Ogre::StringConverter::toString(sourceid) + ":" + Ogre::StringConverter::toString(reg->sid));
	traffic = new AITraffic(this->net, sourceid, reg->sid, slotid);
	NetworkStreamManager::getSingleton().addStream(traffic, sourceid, reg->sid);
	this->streamables[sourceid][reg->sid] = traffic;
	Ogre::LogManager::getSingleton().logMessage(" Traffic registration is done");
	return traffic;
}

void AITrafficFactory::removeUser(int userid)
{
}

void AITrafficFactory::remove(AITraffic *t)
{
}

AITraffic* AITrafficFactory::getTraffic()
{
	return traffic;
}

#endif //AITRAFFIC

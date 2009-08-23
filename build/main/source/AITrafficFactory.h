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

#ifdef AITRAFFIC

#ifndef AITRAFFICFACTORY_H
#define AITRAFFICFACTORY_H

#include "OgrePrerequisites.h"
#include "StreamableFactory.h"
#include "AITraffic.h"
#include "NetworkStreamManager.h"
class Network;


class AITrafficFactory  : public StreamableFactory
{
	public:
		AITrafficFactory(Network *net, Ogre::SceneManager *scm);
		~AITrafficFactory();

		static AITrafficFactory& getSingleton(void);

//		AITraffic *createLocal();	// RoR player cannot create traffic on its own, only traffic client is allowed to do that
		AITraffic *createRemote(int sourceid, stream_register_t *reg, int slotid);

	protected:
		Network *net;
		Ogre::SceneManager *scm;

		// functions used by friends
		void netUserAttributesChanged(int source, int streamid);
		void localUserAttributesChanged(int newid);
};

#endif //AITRAFFICFACTORY_H
#endif //AITRAFFIC

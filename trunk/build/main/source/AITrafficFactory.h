#ifdef AITRAFFIC
#ifndef AITRAFFICFACTORY_H
#define AITRAFFICFACTORY_H

#include "OgrePrerequisites.h"
#include "StreamableFactory.h"
#include "AITraffic.h"

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

#endif
#endif
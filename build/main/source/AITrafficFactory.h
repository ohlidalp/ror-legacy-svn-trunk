#ifdef AITRAFFIC
#ifndef AITRAFFICFACTORY_H
#define AITRAFFICFACTORY_H

#include "StreamableFactory.h"

class AITrafficFactory  : public StreamableFactory
{
	public:
		AITrafficFactory& AITrafficFactory::getSingleton(void);
		AITrafficFactory(Network *net, Ogre::SceneManager *scm);
		~AITrafficFactory();

	protected:
		Network *net;
		Ogre::SceneManager *scm;
	
	// functions used by friends
	void netUserAttributesChanged(int source, int streamid);
	void localUserAttributesChanged(int newid);
};

#endif
#endif
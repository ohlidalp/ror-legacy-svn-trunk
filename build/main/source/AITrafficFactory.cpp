#ifdef AITRAFFIC
#include "AITrafficFactory.h"

AITrafficFactory& AITrafficFactory::getSingleton(void)
{
	assert( ms_Singleton );  return ( *(dynamic_cast<AITrafficFactory*>(ms_Singleton)) );
}


AITrafficFactory::AITrafficFactory(Network *net, Ogre::SceneManager *scm) : net(net), scm(scm), StreamableFactory()
{
}


AITrafficFactory::~AITrafficFactory()
{}

void AITrafficFactory::netUserAttributesChanged(int source, int streamid)
{}

void AITrafficFactory::localUserAttributesChanged(int newid)
{}

#endif AITRAFFIC
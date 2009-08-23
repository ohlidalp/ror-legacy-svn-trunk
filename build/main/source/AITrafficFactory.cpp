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

AITraffic *AITrafficFactory::createRemote(int sourceid, stream_register_t *reg, int slotid)
{
	Ogre::LogManager::getSingleton().logMessage(" new Traffic management for " + Ogre::StringConverter::toString(sourceid) + ":" + Ogre::StringConverter::toString(reg->sid));
	AITraffic *traffic = new AITraffic(sourceid, reg->sid, slotid);
	NetworkStreamManager::getSingleton().addStream(traffic, sourceid, reg->sid);

	Ogre::LogManager::getSingleton().logMessage(" CP-1");
	streamables[sourceid][reg->sid] = traffic;
	Ogre::LogManager::getSingleton().logMessage(" Traffic registration is done");
	return traffic;
}


#endif AITRAFFIC
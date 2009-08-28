#ifdef AITRAFFIC
#include "network.h"
#include "AITraffic.h"
#include "ExampleFrameListener.h"

AITraffic::AITraffic(Network *net, int sourceid, int id, int slotid)
{
	Ogre::LogManager::getSingleton().logMessage("TRAFFIC - Created");
	this->source	= source;
	this->streamid	= streamid;
	this->slotid	= slotid;
	this->remote	= true;
	nettraffic.num_of_objs = 100;

	for (int i=0;i<100;i++)
		{
			nettraffic.objs[i].pos = Ogre::Vector3(0,0,0);
			nettraffic.objs[i].dir = Ogre::Quaternion(1,0,0,0);
		}
}

AITraffic::~AITraffic()
{
}

void AITraffic::sendStreamData()
{
/*
	int t = netTimer.getMilliseconds();
	if (t-last_net_time < 100)
		return;

	last_net_time = t;
*/

//	LogManager::getSingleton().logMessage("sending character stream data: " + StringConverter::toString(net->getUserID()) + ":"+ StringConverter::toString(streamid));
	this->addPacket(MSG2_STREAM_DATA, 123, 2, sizeof(nettraffic_t), (char *)&nettraffic);
}

void AITraffic::receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len)
{
	memcpy(&nettraffic, buffer, sizeof(nettraffic_t));
}

void AITraffic::sendStreamSetup()
{
}


#endif //AITRAFFIC
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

void AITraffic::sendStreamSetup()
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
//	memcpy(&nettraffic, buffer, sizeof(nettraffic_t));
	int a;
	memcpy(&a, buffer, sizeof(int));
	switch (a)
		{
			case AIMSG_PING:				ping(buffer, len)				;		break;
			case AIMSG_SETUPVEHICLES:		setupVehicles(buffer, len)		;		break;
			case AIMSG_SETPOSITIONDATA:		setPositionData(buffer, len)	;		break;
			case AIMSG_SETUPLAMPS:			setupLamps(buffer, len)			;		break;
			case AIMSG_SETUPSIGNS:			setupSigns(buffer, len)			;		break;
			case AIMSG_SETUPZONES:			setupZones(buffer, len)			;		break;
			case AIMSG_SETUPPORTALS:		setupPortals(buffer, len)		;		break;
			case AIMSG_UPDATELAMPPROGRAMS:	updateLampPrograms(buffer, len)	;		break;	
		}
}

/* --- AITraffic related executors -- */

void AITraffic::ping(char *buffer, unsigned int &len)
{}

void AITraffic::setupVehicles(char *buffer, unsigned int &len)
{}

void AITraffic::setPositionData(char *buffer, unsigned int &len)
{}

void AITraffic::setupLamps(char *buffer, unsigned int &len)
{}

void AITraffic::setupSigns(char *buffer, unsigned int &len)
{}

void AITraffic::setupZones(char *buffer, unsigned int &len)
{}

void AITraffic::setupPortals(char *buffer, unsigned int &len)
{}

void AITraffic::updateLampPrograms(char *buffer, unsigned int &len)
{}



#endif //AITRAFFIC
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

		sendStreamSetup();
}

AITraffic::~AITraffic()
{
}

void AITraffic::sendStreamData()
{
	Ogre::LogManager::getSingleton().logMessage("TRAFFIC - SendStreamData");
/*
	int t = netTimer.getMilliseconds();
	if (t-last_net_time < 100)
		return;

	last_net_time = t;
*/
//	netdata_t data;
//	data.pos = personode->getPosition();
//	data.rot = personode->getOrientation();

	nettraffic_t nettraffic;
	nettraffic.num_of_objs = 3;
	nettraffic.objs[0].pos.x = 987.0f;
	nettraffic.objs[1].pos.x = 654.0f;
	nettraffic.objs[2].pos.x = 321.0f;

	
//	LogManager::getSingleton().logMessage("sending character stream data: " + StringConverter::toString(net->getUserID()) + ":"+ StringConverter::toString(streamid));
	this->addPacket(MSG2_STREAM_DATA, 123, 2, sizeof(nettraffic_t), (char *)&nettraffic);
//	this->addPacket(MSG2_STREAM_DATA, 11, 2, 0, NULL);
}

void AITraffic::receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len)
{
	Ogre::LogManager::getSingleton().logMessage("TRAFFIC - receiveStreamData");
	sendStreamData();

	nettraffic_t nettraffic;
	memcpy(&nettraffic, buffer, sizeof(nettraffic_t));

	int a;
	a++;

/*
	if(type == MSG2_STREAM_DATA && this->source == source && this->streamid == streamid)
	{
		netdata_t *data = (netdata_t *)buffer;
		//LogManager::getSingleton().logMessage("character stream data correct: " + StringConverter::toString(source) + ":"+ StringConverter::toString(streamid) + ": "+ StringConverter::toString(data->pos));
		personode->setPosition(data->pos);
		personode->setOrientation(data->rot);
	}
	//else
	//	LogManager::getSingleton().logMessage("character stream data wrong: " + StringConverter::toString(source) + ":"+ StringConverter::toString(streamid));
*/
}

void AITraffic::sendStreamSetup()
{
/*
	if(remote)
	{
		LogManager::getSingleton().logMessage("new remote Traffic: " + StringConverter::toString(source) + ":"+ StringConverter::toString(streamid));
		NetworkStreamManager::getSingleton().addStream(this, source, streamid);
	}else
	{
		NetworkStreamManager::getSingleton().addStream(this);

		stream_register_t reg;
		reg.sid = this->streamid;
		reg.status = 1;
		strcpy(reg.name, "default");
		reg.type = 1;
		this->addPacket(MSG2_STREAM_REGISTER, net->getUserID(), streamid, sizeof(stream_register_t), (char*)&reg);
	}
*/
}


#endif //AITRAFFIC
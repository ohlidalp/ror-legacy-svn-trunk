#ifdef AITRAFFIC
#include "network.h"
#include "AITraffic.h"
#include "ExampleFrameListener.h"

AITraffic::AITraffic(ExampleFrameListener *efl, Network *net, int sourceid, int id, int slotid)
{
	mefl = efl;
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
	int a = 0;
	setupLamps(NULL);
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
			case AIMSG_SETUPLAMPS:			setupLamps(buffer)			;		break;
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

void AITraffic::setupLamps(char *buffer)
{
	spawnObject("trafficlight3", "trafficlight-1-1", Ogre::Vector3(0,0,0), Ogre::Vector3(0,0,0), true);
	spawnObject("trafficlight3", "trafficlight-1-2", Ogre::Vector3(10,0,10), Ogre::Vector3(0,0,0), true);
	spawnObject("trafficlight3", "trafficlight-1-3", Ogre::Vector3(20,0,20), Ogre::Vector3(0,0,0), true);
	spawnObject("trafficlight3", "trafficlight-1-4", Ogre::Vector3(30,0,30), Ogre::Vector3(0,0,0), true);
	spawnObject("trafficlight3", "trafficlight-1-5", Ogre::Vector3(40,0,40), Ogre::Vector3(0,0,0), true);
	spawnObject("trafficlight3", "trafficlight-1-6", Ogre::Vector3(50,0,50), Ogre::Vector3(0,0,0), true);
}

void AITraffic::setupSigns(char *buffer, unsigned int &len)
{}

void AITraffic::setupZones(char *buffer, unsigned int &len)
{}

void AITraffic::setupPortals(char *buffer, unsigned int &len)
{}

void AITraffic::updateLampPrograms(char *buffer, unsigned int &len)
{}

void AITraffic::spawnObject(const std::string &objectName, const std::string &instanceName, Ogre::Vector3 pos, Ogre::Vector3 rot, bool uniquifyMaterials)
{
	// trying to create the new object
//	ExampleFrameListener *mefl = (ExampleFrameListener *)scriptengine;
	SceneNode *bakeNode = mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode();
	mefl->loadObject(const_cast<char*>(objectName.c_str()), pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, bakeNode, const_cast<char*>(instanceName.c_str()), true, 0, const_cast<char*>(objectName.c_str()), uniquifyMaterials);
}

void AITraffic::setSignalState(Ogre::String instance, int state)
{
	int red		= state & 1;
	int yellow	= state & 2;
	int green	= state & 4;
	// states:
	// 0 = red
	// 1 = yellow
	// 2 = green


	setMaterialAmbient("trafficlight3/top"+instance,	red,	0,	0);
	setMaterialDiffuse("trafficlight3/top"+instance,	red,	0,	0, 1);
	setMaterialEmissive("trafficlight3/top"+instance,	(red? 1:0),	0,	0);

	setMaterialAmbient("trafficlight3/middle"+instance, yellow, yellow, 0);
	setMaterialDiffuse("trafficlight3/middle"+instance, yellow, yellow, 0, 1);
	setMaterialEmissive("trafficlight3/middle"+instance, (yellow? 1:0), (yellow? 1:0), 0);

	setMaterialAmbient("trafficlight3/bottom"+instance, 0, green, 0);
	setMaterialDiffuse("trafficlight3/bottom"+instance, 0, green, 0, 1);
	setMaterialEmissive("trafficlight3/bottom"+instance, 0, (green? 1:0), 0);
}


int AITraffic::setMaterialAmbient(const std::string &materialName, float red, float green, float blue)
{
	try
	{
		Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setAmbient(red, green, blue);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

int AITraffic::setMaterialDiffuse(const std::string &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setDiffuse(red, green, blue, alpha);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

int AITraffic::setMaterialSpecular(const std::string &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setSpecular(red, green, blue, alpha);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

int AITraffic::setMaterialEmissive(const std::string &materialName, float red, float green, float blue)
{
	try
	{
		Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setSelfIllumination(red, green, blue);
	} catch(...)
	{
		return 0;
	}
	return 1;
}


#endif //AITRAFFIC
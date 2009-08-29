#ifdef AITRAFFIC
#pragma once
#ifndef AITraffic_H
#define AITraffic_H

#include "Ogre.h"
#include "OgreVector3.h"
#include "OgreMaterial.h"

#include "AITraffic_Common.h"
#include "Streamable.h"
#include "rornet.h"

class Network;
class ExampleFrameListener;

class AITraffic : public Streamable
{

	friend class AITrafficFactory;
	friend class Network;

	public:
		AITraffic(ExampleFrameListener *efl, Network *net, int sourceid, int id, int slotid);
		~AITraffic();

		void sendStreamData();
		void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);

		nettraffic_t nettraffic;

	protected:
		void sendStreamSetup();

		void ping				(char *buffer, unsigned int &len);
		void setupVehicles		(char *buffer, unsigned int &len);
		void setPositionData	(char *buffer, unsigned int &len);
		void setupLamps			(char *buffer);
		void setupSigns			(char *buffer, unsigned int &len);
		void setupZones			(char *buffer, unsigned int &len);
		void setupPortals		(char *buffer, unsigned int &len);
		void updateLampPrograms	(char *buffer, unsigned int &len);

		void AITraffic::spawnObject(const std::string &objectName, const std::string &instanceName, Ogre::Vector3 pos, Ogre::Vector3 rot, bool uniquifyMaterials);
		void AITraffic::setSignalState(Ogre::String instance, int state);

		// duplicated from AS for performance issues
		int setMaterialAmbient(const std::string &materialName, float red, float green, float blue);
		int setMaterialDiffuse(const std::string &materialName, float red, float green, float blue, float alpha);
		int setMaterialSpecular(const std::string &materialName, float red, float green, float blue, float alpha);
		int setMaterialEmissive(const std::string &materialName, float red, float green, float blue);

	private:
		bool remote;
		int source;
		unsigned int streamid;
		int slotid;
		Network *net;
		ExampleFrameListener *mefl;

};

#endif
#endif //AITRAFFIC

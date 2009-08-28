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

class AITraffic : public Streamable
{

	friend class AITrafficFactory;
	friend class Network;

	public:
		AITraffic(Network *net, int sourceid, int id, int slotid);
		~AITraffic();

		void sendStreamData();
		void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);

		nettraffic_t nettraffic;

	protected:
		void sendStreamSetup();

		void ping				(char *buffer, unsigned int &len);
		void setupVehicles		(char *buffer, unsigned int &len);
		void setPositionData	(char *buffer, unsigned int &len);
		void setupLamps			(char *buffer, unsigned int &len);
		void setupSigns			(char *buffer, unsigned int &len);
		void setupZones			(char *buffer, unsigned int &len);
		void setupPortals		(char *buffer, unsigned int &len);
		void updateLampPrograms	(char *buffer, unsigned int &len);

	private:
		bool remote;
		int source;
		unsigned int streamid;
		int slotid;
		Network *net;

};

#endif
#endif //AITRAFFIC

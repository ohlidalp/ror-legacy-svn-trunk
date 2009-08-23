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

class ExampleFrameListener;

class AITraffic : public Streamable
{

	friend class AITrafficFactory;
	friend class Network;

	public:
		AITraffic(int sourceid, int id, int slotid);
		~AITraffic();

		void AITraffic::sendStreamData();
		void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);
};

#endif
#endif //AITRAFFIC

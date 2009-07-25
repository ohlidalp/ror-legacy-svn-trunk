#ifndef AITraffic_H
#define AITraffic_H

#ifdef OPENSTEER
#include "Ogre.h"
#include "OgreVector3.h"

#include "AITraffic_Common.h"
#include "AITraffic_Vehicle.h"

class AITraffic
{
	public:
		AITraffic();
		~AITraffic();

		void load();
		void initialize();
		void frameStep(Ogre::Real deltat);

		trafficgrid_t trafficgrid;

		
	private:
		void processOneCar(int idx, float delta);
		Ogre::Vector3 waypoints[20];
		int wpi;
		int max_wpi;
		Ogre::Real mTotalElapsedTime;

		int num_of_vehicles;
		AITraffic_Vehicle *vehicles[NUM_OF_TRAFFICED_CARS];
};

#endif //OPENSTEER

#endif

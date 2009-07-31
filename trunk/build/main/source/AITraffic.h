#ifdef OPENSTEER
#pragma once
#ifndef AITraffic_H
#define AITraffic_H

#include "Ogre.h"
#include "OgreVector3.h"

#include "OpenSteer/Pathway.h"
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

		Ogre::Vector3		playerpos;		// we store here the player's position and rotation
		Ogre::Quaternion	playerrot;		// used for creating interactivity layer in traffic

		
	private:
		void checkForZones();							// checking if
		void processOneCar(int idx, float delta);		// update a vehicle position (by OpenSteer)
		
		
		
		Ogre::Vector3 waypoints[200];
		Ogre::Vector3 turnpoints[200];
		int wpi;
		int max_wpi;
		Ogre::Real mTotalElapsedTime;

		int num_of_vehicles;
		int num_of_waypoints;
		AITraffic_Vehicle *vehicles[NUM_OF_TRAFFICED_CARS];
		float rs;
};

#endif
#endif //OPENSTEER

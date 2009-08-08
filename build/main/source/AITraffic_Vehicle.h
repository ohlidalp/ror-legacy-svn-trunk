#ifdef OPENSTEER
#pragma once
#ifndef AITRAFFIC_VEHICLE_H
#define AITRAFFIC_VEHICLE_H

#include "Ogre.h"
#include "OpenSteer/AbstractVehicle.h"
#include "OpenSteer/LocalSpace.h"
#include "OpenSteer/SteerLibrary.h"
#include "OpenSteer/Utilities.h"
#include "OpenSteer/Pathway.h"
#include "AITraffic_Route.h"
#include "AITraffic_TerrainMap.h"
#include "AITraffic_Common.h"

using namespace OpenSteer;

class AITraffic_Vehicle
{

	public:

		AITraffic_Vehicle(); 
		~AITraffic_Vehicle();

		void reset (void);

		void	updateSimple (const float currentTime, const float elapsedTime);	// the main updater function
		Ogre::Vector3 getPosition();
		void setPosition(Ogre::Vector3 newPos);
		Ogre::Quaternion getOrientation();
		AITraffic_Matrix *aimatrix;
		int serial;

		int path_id;
		int path_direction;
		int ps_idx;					// current path segment idx
		bool active;				// true if vehicle needs traffic calculation
		float speed;				// in m/s

	private:
		int		closestWayPoint();													// finds the closest waypoint for our position
		int		getHeadedWayPoint();												// returns the waypoint we are heading to
		int		getLeftWayPoint();													// returns the waypoint we are coming from
		int		advanceToNextWayPoint();
		bool	closeToWayPoint(int idx, float r);									// returns  true if we are within r range from the waypoint idx
		void	advance(float deltat);												// move the vehicle
		float	objectsOnTravelPath();												// returns the distance of the nearest obstacle in travel path
		float	calculateSafeFollowDistance();									
		float   calculateBrakeDistance();
	
	private:
		int wp_idx;					// which waypoint we are heading to
		int wp_prev_idx;
		Ogre::Vector3 position;		// the current position
		Ogre::Vector3 forward;		// where we are heading to


		



};

#endif
#endif //OPENSTEER
    
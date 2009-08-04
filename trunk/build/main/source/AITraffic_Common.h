#ifdef OPENSTEER
#pragma once
#ifndef AITraffic_Common_H
#define AITraffic_Common_H

#define NUM_OF_TRAFFICED_CARS			10
#define NUM_OF_TRAFFICLIGHTS			1000
#define NUM_OF_INTERSECTIONS			100
#define NUM_OF_LAMPS_PER_INTERSECTION	32			// we use one bit for a lamp
#define MAX_PROGRAM_LENGTH				16

#include "Ogre.h"

typedef struct _trafficlightnode
{
	int state;				// bitwise  1-green, 2-yellow, 4-red, 128-blinking (this is current state)
	int nextstate;			// as above, but the next state is stored here (so no red-yellow-red cycle)
	int intersection_id;	// id of intersection this lamp is a part of
	int group_id;			// lane group id of the lamp
	Ogre::String name;

} trafficlightnode_t;

typedef struct _trafficintersection
{
	int num_of_lamps;
	int length_of_program;
	trafficlightnode_t lamps[NUM_OF_LAMPS_PER_INTERSECTION];
	int program[MAX_PROGRAM_LENGTH];

	bool inchange;
	int prg_idx;
} trafficintersection_t;

typedef struct _trafficnode
{
	Ogre::Vector3		position;		// current position of vehicle
	Ogre::Quaternion	rotation;		// current orientation of vehicle
	Ogre::Vector3		aabb;			// axis aligned bounding box for the vehicle
	Ogre::Vector3		dimensions;		// width (x), height (y), length (z) of the vehicle

} trafficnode_t;



typedef struct _trafficgrid
{
	 trafficnode_t trafficnodes[NUM_OF_TRAFFICED_CARS];

} trafficgrid_t;


typedef trafficlightnode_t		trafficlightgrid_t[NUM_OF_TRAFFICLIGHTS];
typedef trafficintersection_t	trafficintersectiongrid_t[NUM_OF_INTERSECTIONS];

//typedef trafficnode_t			trafficgrid_t[NUM_OF_TRAFFICED_CARS];

class AITraffic_Matrix
{
	public:
//		AITraffic_Matrix() { trafficgrid = (trafficgrid_t*) malloc(NUM_OF_TRAFFICED_CARS *sizeof(trafficnode_t));}
//		AITraffic_Matrix() { trafficgrid = (trafficgrid_t*) malloc(NUM_OF_TRAFFICED_CARS *sizeof(_trafficnode));}
		AITraffic_Matrix() 
			{ 
				Ogre::LogManager::getSingleton().logMessage("SIZEOF trafficgrid_t: "+Ogre::StringConverter::toString(sizeof(trafficgrid_t)));
				Ogre::LogManager::getSingleton().logMessage("SIZEOF trafficnode_t: "+Ogre::StringConverter::toString(sizeof(trafficnode_t)));
				Ogre::LogManager::getSingleton().logMessage("SIZEOF _trafficnode: "+Ogre::StringConverter::toString(sizeof(_trafficnode)));
//				trafficgrid = (trafficgrid_t*) malloc(100000);
				trafficgrid = (trafficgrid_t*) malloc(sizeof(trafficgrid_t));
//				trafficnodes = (trafficnode_t *) malloc(sizeof(trafficnode_t)*NUM_OF_TRAFFICED_CARS);
		
			}
//		AITraffic_Matrix() { trafficgrid = (trafficgrid_t*) malloc(sizeof(trafficgrid_t));}
		~AITraffic_Matrix() {};
		trafficgrid_t *trafficgrid;

//		trafficnode_t *trafficnodes[NUM_OF_TRAFFICED_CARS];

};

#endif
#endif //OPENSTEER

#ifdef AITRAFFIC
#pragma once
#ifndef AITraffic_Common_H
#define AITraffic_Common_H

#include "Ogre.h"

#define AITRAFFIC_MAX_ZONES				32
#define AITRAFFIC_MAX_NUM_LAMPS			256
#define AITRAFFIC_MAX_PORTALS			64

#define AIMSG_PING						0
#define AIMSG_SETUPVEHICLES				1
#define AIMSG_SETPOSITIONDATA			2
#define AIMSG_SETUPLAMPS				3
#define AIMSG_SETUPSIGNS				4
#define AIMSG_SETUPZONES				5
#define AIMSG_SETUPPORTALS				6
#define AIMSG_UPDATELAMPPROGRAMS		7

typedef struct _ping
{
	int command;
} ping_t;

typedef struct _setupvehicles
{
	int command;
} setupvehicles_t;

typedef struct _setupposition
{
	int command;
} setupposition_t;

typedef struct _setuplamps
{
	int command;
} setuplamps_t;


typedef struct _trafficlightnode
{
	int index;
	int state;
	Ogre::String name;
} trafficlightnode_t;

typedef struct _netobj
{
	Ogre::Vector3	 pos;
	Ogre::Quaternion dir;
} netobj_t;

typedef struct _nettraffic
{
	int num_of_objs;
	Ogre::Vector3		playerpos;
	Ogre::Quaternion	playerdir;
	netobj_t objs[100];
} nettraffic_t;

//---------------- Traffic Vehicle related types


/* Internal structrures */

typedef struct _zone
{
	Ogre::Vector3 p1;
	Ogre::Vector3 p2;
} zone_t;


typedef struct _trafficnode
{
	bool				direct_control;	// if true direct (pos+rot is given), if false indirect (steer, brake, throttle)

	Ogre::Vector3		position;		// current position of vehicle
	Ogre::Quaternion	rotation;		// current orientation of vehicle
	Ogre::Vector3		aabb;			// axis aligned bounding box for the vehicle
	Ogre::Vector3		dimensions;		// width (x), height (y), length (z) of the vehicle
	int					type;			// register object type here (1 - vehicle, 2 - pedestrian)
	bool				active;			// if false, it is not updated
	int					zone;			// what zone it is current in (-1 if no zone)
	bool				inzone;			// used to flag procession of entrance to a zone
	float				wait;			// set this to >0 if node should wait before progressing further
	float				steer;			// steering hydro command
	float				accel;			// acceleration hydro command
	float				brake;			// brake hydro command

} trafficnode_t;

//typedef trafficlightnode_t		trafficlightgrid_t[NUM_OF_TRAFFICLIGHTS];

#endif
#endif //AITRAFFIC

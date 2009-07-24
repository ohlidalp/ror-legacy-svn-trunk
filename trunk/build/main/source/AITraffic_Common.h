#pragma once
#ifndef AITraffic_Common_H
#define AITraffic_Common_H

#define NUM_OF_TRAFFICED_CARS 1000

#include "Ogre.h"

typedef struct _trafficnode
{
	float speed;	// in m/s
	Ogre::Vector3 lastpos;

	float x1;
	float y1;
	float z1;

	float x2;
	float y2;
	float z2;

} trafficnode_t;

typedef trafficnode_t trafficgrid_t[NUM_OF_TRAFFICED_CARS];

#endif

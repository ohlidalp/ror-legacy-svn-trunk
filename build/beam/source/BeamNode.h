/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __BeamNode_H__
#define __BeamNode_H__

#include <BeamConfig.h>

typedef struct _node
{
	Real mass;
	Real inverted_mass;
	Vector3 iPosition; // initial position, absolute
	Real    iDistance; // initial distance from node0 during loading - used to check for loose parts
	Vector3 AbsPosition; //absolute position in the world (shaky)
	Vector3 RelPosition; //relative to the local physics origin (one origin per truck) (shaky)
	Vector3 smoothpos; //absolute, per-frame smooth, must be used for visual effects only
	Vector3 Velocity;
	Vector3 Forces;
	Vector3 lastNormal;
	int locked;
	int iswheel; //0=no, 1, 2=wheel1  3,4=wheel2, etc...
	int wheelid;
	int masstype;
	int contactless;
	int contacted;
	Real friction;
	Real buoyancy;
	Vector3 lastdrag;
	Vector3 gravimass;
	int lockednode;
	Vector3 lockedPosition; //absolute
	Vector3 lockedVelocity;
	Vector3 lockedForces;
	int wetstate;
	float wettime;
	bool isHot;
	bool overrideMass;
	bool iIsSkin;
	bool isSkin;
	Vector3 buoyanceForce;
	int id;
	float colltesttimer;
//	Vector3 tsmooth;
} node_t;

#endif

/*
This source file is part of Rigs of Rods
Copyright 2005-2011 Pierre-Michel Ricordel
Copyright 2007-2011 Thomas Fischer

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
#ifndef AEROENGINE_H__
#define AEROENGINE_H__

#include "RoRPrerequisites.h"

class AeroEngine
{
public:
	enum {AEROENGINE_TYPE_TURBOPROP, AEROENGINE_TYPE_TURBOJET};

	virtual ~AeroEngine() {};

	virtual void updateVisuals()=0;
	virtual void updateForces(float dt, int doUpdate)=0;

	virtual void setThrotle(float val)=0;

	virtual float getThrotle()=0;

	virtual void reset()=0;

	virtual void toggleReverse()=0;

	virtual void flipStart()=0;

	virtual float getRPMpc()=0;
	virtual float getRPM()=0;
	virtual void setRPM(float _rpm)=0;
	virtual float getpropwash()=0;
	virtual Ogre::Vector3 getAxis()=0;
	virtual bool isFailed()=0;
	virtual int getType()=0;
	virtual bool getIgnition()=0;
	virtual int getNoderef()=0;
	virtual bool getWarmup()=0;
	virtual float getRadius()=0;
};

#endif //AEROENGINE_H__

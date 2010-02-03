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
// thomas, 11th of March 2008

#ifndef __BeamStats_H__
#define __BeamStats_H__

#include <Ogre.h>
#include <map>
#include "rormemory.h"

class Beam;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define BEAMSTATS
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#undef BEAMSTATS
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#undef BEAMSTATS
#endif

#ifdef BEAMSTATS

#define MAX_TIMINGS 20
#define BES BeamEngineStats::getInstance()
class BeamThreadStats : public MemoryAllocatedObject
{
public:
	BeamThreadStats();
	~BeamThreadStats();
	void queryStart(int type);
	void queryStop(int type);
	void frameStep(float ds);

	unsigned int getFramecount();
	unsigned int getPhysFrameCount();
	double getTiming(int type);

	
	static enum types {WholeTruckCalc=0, Beams, Nodes, TruckEngine, Rigidifiers, Ropes, Turboprop, Screwprop, Wing, FuseDrag, Airbrakes, Buoyance, Contacters, CollisionCab, Wheels, Shocks, Hydros, Commands};
	static Ogre::String typeDescriptions[MAX_TIMINGS];
private:
	LARGE_INTEGER  timings_start[MAX_TIMINGS];
	double timings[MAX_TIMINGS];
	double savedTimings[MAX_TIMINGS];
	Ogre::String stattext;
	unsigned int framecounter;
	unsigned int physcounter;
};

class BeamEngineStats : public MemoryAllocatedObject
{
public:
	bool updateGUI(float dt, int current_truck, Beam **trucks);
	void setup(bool enabled);
	BeamThreadStats *getClient(int number);
	static BeamEngineStats & getInstance();
	~BeamEngineStats();
protected:
	BeamEngineStats();
	BeamEngineStats(const BeamEngineStats&);
	BeamEngineStats& operator= (const BeamEngineStats&);

private:
	bool enabled;
	static BeamEngineStats *myInstance;
	std::map<int, BeamThreadStats*> statClients;
};

#endif

#endif



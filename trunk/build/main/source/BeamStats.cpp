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
#include "BeamStats.h"

#ifdef BEAMSTATS

// thomas, 11th of March 2008

#include "Beam.h"

using namespace std;
using namespace Ogre;

BeamEngineStats *BeamEngineStats::myInstance = 0;

BeamEngineStats::BeamEngineStats()
{
	enabled=true;
}

BeamEngineStats::~BeamEngineStats()
{
}

BeamEngineStats &BeamEngineStats::getInstance()
{
	if(myInstance == 0) {
		myInstance = new BeamEngineStats();
	}
	return *myInstance;
}

BeamThreadStats *BeamEngineStats::getClient(int number)
{
	if(enabled)
	{
		BeamThreadStats *bts = new BeamThreadStats();
		if(statClients[number]!=0)
			delete statClients[number];
		statClients[number] = bts;
		return bts;
	} else
		return 0;
}

void BeamEngineStats::setup(bool enabled)
{
	this->enabled = enabled;
}

bool BeamEngineStats::updateGUI(float dt, int current_truck, Beam **trucks)
{
	static OverlayElement *stats = 0;
	if(!stats)
		stats = OverlayManager::getSingleton().getOverlayElement("tracks/DebugBeamTiming/Text");
	
	if(statClients.size() == 0)
		return true;

	if(!stats->isVisible())
		return true;

	std::map<int, BeamThreadStats*>::iterator it;
	String msg = "";
	for(it=statClients.begin(); it!=statClients.end(); it++)
	{
		//if(it->first != current_truck || trucks[it->first]->state == SLEEPING)
		if(trucks[it->first]->state == SLEEPING)
			continue;

		double sum=0, t=0, perc=0;

		msg += "\n";
		msg += "Truck "+StringConverter::toString(it->first) + "\n";
		msg += "  Graphic Frames: " + StringConverter::toString(it->second->getFramecount()) + "\n";
		msg += "  Physic Frames: " + StringConverter::toString(it->second->getPhysFrameCount()) + "\n";
		
		sum = it->second->getTiming(BeamThreadStats::WholeTruckCalc);
		if(sum==0)
			continue;
		
		msg += "  sum: 100% ("+StringConverter::toString((Real)sum)+"s\n";
		
		for(int i=1;i<MAX_TIMINGS;i++)
		{
			t = it->second->getTiming(i);
			perc=(int)(100.0 * t / sum);
			if(perc>=2)
				msg += "  " + BeamThreadStats::typeDescriptions[i] + ": "+StringConverter::toString((int)perc)+"% ("+StringConverter::toString((Real)t)+"s)\n";
		}
	}
	stats->setCaption(msg);
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


Ogre::String BeamThreadStats::typeDescriptions[MAX_TIMINGS];

BeamThreadStats::BeamThreadStats()
{
	for (int i=0; i<MAX_TIMINGS; i++) 
	{
		timings[i]=0;
		savedTimings[i]=0;
		timings_start[i].QuadPart=0;
	}
	framecounter=0;
	physcounter=0;

	// setup descriptions
	BeamThreadStats::typeDescriptions[WholeTruckCalc] = "Sum";
	BeamThreadStats::typeDescriptions[Beams] = "Beams";
	BeamThreadStats::typeDescriptions[Nodes] = "Nodes";
	BeamThreadStats::typeDescriptions[TruckEngine] = "TruckEngine";
	BeamThreadStats::typeDescriptions[Rigidifiers] = "Rigidifiers";
	BeamThreadStats::typeDescriptions[Ropes] = "Ropes";
	BeamThreadStats::typeDescriptions[Turboprop] = "Turboprop";
	BeamThreadStats::typeDescriptions[Screwprop] = "Screwprop";
	BeamThreadStats::typeDescriptions[Wing] = "Wing";
	BeamThreadStats::typeDescriptions[FuseDrag] = "FuseDrag";
	BeamThreadStats::typeDescriptions[Airbrakes] = "Airbrakes";
	BeamThreadStats::typeDescriptions[Buoyance] = "Buoyance";
	BeamThreadStats::typeDescriptions[Contacters] = "Contacters";
	BeamThreadStats::typeDescriptions[CollisionCab] = "CollisionCab";
	BeamThreadStats::typeDescriptions[Wheels] = "Wheels";
	BeamThreadStats::typeDescriptions[Shocks] = "Shocks";
	BeamThreadStats::typeDescriptions[Hydros] = "Hydros";
	BeamThreadStats::typeDescriptions[Commands] = "Commands";
}

BeamThreadStats::~BeamThreadStats()
{
}

void BeamThreadStats::frameStep(float ds)
{
	for (int i=0; i<MAX_TIMINGS; i++) 
		timings[i]=0;
	framecounter++;
}


void BeamThreadStats::queryStart(int type)
{
	QueryPerformanceCounter(&timings_start[type]);
}

void BeamThreadStats::queryStop(int type)
{
	LARGE_INTEGER tick, ticksPerSecond;
	QueryPerformanceFrequency(&ticksPerSecond);
	QueryPerformanceCounter(&tick);
	
	if (timings_start[type].QuadPart != 0)
		timings[type] += ((double)tick.QuadPart - (double)timings_start[type].QuadPart) / (double)ticksPerSecond.QuadPart;
	
	if(type == WholeTruckCalc)
	{
		// secure timings
		physcounter++;
		memcpy(savedTimings, timings, sizeof(double) * MAX_TIMINGS);
	}
}

unsigned int BeamThreadStats::getPhysFrameCount()
{
	return physcounter;
}

unsigned int BeamThreadStats::getFramecount()
{
	return framecounter;
}

double BeamThreadStats::getTiming(int type)
{
	return savedTimings[type];
}


#endif


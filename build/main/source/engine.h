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
#ifndef __Engine_H__
#define __Engine_H__

#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif

class SoundScriptManager;
#include <stdio.h>
#include <stdlib.h>

//#include "Ogre.h"
//using namespace Ogre;

#define CLUTCH_TIME 0.2f
#define SHIFT_TIME 0.5f
#define POST_SHIFT_TIME 0.2f

#define AUTOMATIC 0
#define SEMIAUTO 1
#define MANUAL 2

enum autoswitch {REAR=0, NEUTRAL=1, DRIVE=2, TWO=3, ONE=4, MANUALMODE=5};

class BeamEngine
{
protected:
	float reverseRatio;
	int numGears;
	float *gearsRatio;
	float inertia;
	float clutchForce;

	int curGear;
	float curEngineRPM;
	float curGearboxRPM;
	float curClutch;
	float curAcc;
	float curClutchTorque;
	//shifting
	int shifting;
	int shiftval;
	float shiftclock;
	int postshifting;
	float postshiftclock;
	//auto
	float autocurAcc;
	int starter;
	autoswitch autoselect;
	//turbo
	float curTurboRPM;
	//air pressure
	float apressure;
	int automode;
	int trucknum;
	SoundScriptManager *ssm;

public:
	float iddleRPM;
	float maxRPM;
	float stallRPM;
	float engineTorque;
	float brakingTorque;

	bool hasturbo;
	bool hasair;
	char type;
	int running;
	int contact;
	char status[256];
	float hydropump;
	int prime;

	BeamEngine(float iddle, float max, float torque, float rear, int numgears, float *gears, float diff, int trucknum);
	void setOptions(float einertia, char etype);
	void update(float dt, int doUpdate);
	float getRPM();
	void toggleAutoMode();
	int getAutoMode();
	void setAcc(float val);
	float getTurboPSI();
	float getAcc();
	void netForceSettings(float rpm, float force);
	float getSmoke();
	float getTorque();
	void setSpin(float rpm);
	//for hydros acceleration
	float getCrankFactor();
	void setClutch(float clutch);
	float getClutch();
	void toggleContact();
	//quick start
	void start();
	void offstart();
	void setstarter(int v);
	//low level gear changing
	int getGear();
	void setGear(int v);
	//stalling engine
	void stop();
	//high level controls
	void autoSetAcc(float val);
	void shift(int val);
	void shiftTo(int val);
	void updateShifts();
	void autoShiftUp();
	void autoShiftDown();
	int getAutoShift();
	void setManualClutch(float val);
	int getNumGears() { return numGears; };
	float getMaxRPM() { return maxRPM; };
	~BeamEngine();
};

#endif

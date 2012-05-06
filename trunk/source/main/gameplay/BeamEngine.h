/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#ifndef _Engine_H__
#define _Engine_H__

#include "RoRPrerequisites.h"

class BeamEngine
{
public:

	enum shiftmodes {AUTOMATIC, SEMIAUTO, MANUAL, MANUAL_STICK, MANUAL_RANGES};
	enum autoswitch {REAR, NEUTRAL, DRIVE, TWO, ONE, MANUALMODE};

	float iddleRPM;
	float maxRPM;
	float stallRPM;
	float brakingTorque;
	float engineTorque;

	bool hasturbo;
	bool hasair;
	char type;
	int running;
	int contact;
	//char status[256];
	float hydropump;
	int prime;

	BeamEngine(float iddle, float max, float torque, std::vector<float> gears, float diff, int trucknum);
	~BeamEngine();
	void setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime);
	void update(float dt, int doUpdate);
	void updateAudio(int doUpdate);
	float getRPM();
	void setRPM(float value);
	
	void toggleAutoMode();
	int getAutoMode();
	void setAutoMode(int mode);
	
	void setAcc(float val);
	float getTurboPSI();
	float getAcc();
	void netForceSettings(float rpm, float force, float clutch, int gear, bool running, bool contact, char automode);
	float getSmoke();
	float getTorque();
	void setSpin(float rpm);
	//for hydros acceleration
	float getCrankFactor();
	void setClutch(float clutch);
	float getClutch();
	float getClutchForce();
	void toggleContact();
	//quick start
	void start();
	void offstart();
	void setstarter(int v);
	//low level gear changing
	int getGear();
	int getGearRange();
	void setGear(int v);
	void setGearRange(int v);
	//stalling engine
	void stop();
	//high level controls
	void autoSetAcc(float val);
	void shift(int val);
	void shiftTo(int val);
	void updateShifts();
	void autoShiftUp();
	void autoShiftDown();
	void autoShiftSet(int mode);
	int getAutoShift();
	void setManualClutch(float val);
	size_t getNumGears() { return gearsRatio.size() - 2; };
	size_t getNumGearsRanges() {return getNumGears()/6+1; }
	float getMaxRPM() { return maxRPM; };
	float getIdleRPM() { return iddleRPM; };
	char getType() { return type; };
	TorqueCurve *getTorqueCurve() { return torqueCurve; };
	float getEngineTorque() { return engineTorque; };

protected:

	float clutch_time;
	float shift_time;
	float post_shift_time;

	int numGears;
	std::vector<float> gearsRatio;
	float inertia;
	float clutchForce;

	int curGear;
	int curGearRange;
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
	TorqueCurve *torqueCurve;
};

#endif // _Engine_H__

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
#ifndef __BeamEngine_H_
#define __BeamEngine_H_

#include "RoRPrerequisites.h"

class BeamEngine
{
	friend class Beam;

public:

	BeamEngine(float iddle, float max, float torque, std::vector<float> gears, float diff, int trucknum);
	~BeamEngine();

	float getRPM();
	void setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime);
	void setRPM(float value);
	void update(float dt, int doUpdate);
	void updateAudio(int doUpdate);
	
	int getAutoMode();
	void setAutoMode(int mode);
	void toggleAutoMode();
	
	float getAcc();
	float getSmoke();
	float getTorque();
	float getTurboPSI();
	void netForceSettings(float rpm, float force, float clutch, int gear, bool running, bool contact, char automode);
	void setAcc(float val);
	void setSpin(float rpm);

	// for hydros acceleration
	float getClutch();
	float getClutchForce();
	float getCrankFactor();
	void setClutch(float clutch);
	void toggleContact();

	//quick start
	void offstart();
	void setstarter(int v);
	void start();

	//low level gear changing
	int getGear();
	int getGearRange();
	void setGear(int v);
	void setGearRange(int v);
	
	// stall engine
	void stop();

	// high level controls
	bool hasContact() { return contact != 0; };
	bool hasTurbo() { return hasturbo; };
	bool isRunning() { return running != 0; };
	char getType() { return type; };
	float getEngineTorque() { return engineTorque; };
	float getIdleRPM() { return iddleRPM; };
	float getMaxRPM() { return maxRPM; };
	int getAutoShift();
	size_t getNumGears() { return gearsRatio.size() - 2; };
	size_t getNumGearsRanges() {return getNumGears()/6+1; }
	TorqueCurve *getTorqueCurve() { return torqueCurve; };
	void autoSetAcc(float val);
	void autoShiftDown();
	void autoShiftSet(int mode);
	void autoShiftUp();
	void setManualClutch(float val);
	void shift(int val);
	void shiftTo(int val);
	void updateShifts();

	enum shiftmodes {AUTOMATIC, SEMIAUTO, MANUAL, MANUAL_STICK, MANUAL_RANGES};
	enum autoswitch {REAR, NEUTRAL, DRIVE, TWO, ONE, MANUALMODE};

protected:

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
	float hydropump;
	int prime;
	

	// gear stuff
	float curGearboxRPM;
	int curGear;
	int curGearRange;
	int numGears;
	std::vector<float> gearsRatio;

	// clutch
	float clutchForce;
	float clutch_time;
	float curClutch;
	float curClutchTorque;

	// engine stuff
	float curAcc;
	float curEngineRPM;
	float inertia;

	// shifting
	float post_shift_time;
	float postshiftclock;
	float shift_time;
	float shiftclock;
	int postshifting;
	int shifting;
	int shiftval;

	// auto
	autoswitch autoselect;
	float autocurAcc;
	int starter;

	// turbo
	float curTurboRPM;

	// air pressure
	TorqueCurve *torqueCurve;
	float apressure;
	int automode;
	int trucknum;
};

#endif // __BeamEngine_H_

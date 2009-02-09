/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#ifndef __BeamJoystick_H__
#define __BeamJoystick_H__

#include "Ogre.h"
using namespace Ogre;
#include "OISEvents.h"
#include "OISInputManager.h"
#include "OISJoyStick.h"

//axis type definition
#define AXIS_VEHICLE_STEERING		0
#define AXIS_VEHICLE_COMBO			1
#define AXIS_VEHICLE_THROTLE		2
#define AXIS_VEHICLE_BRAKE			3
#define AXIS_VEHICLE_CLUTCH			4
#define AXIS_VEHICLE_SHIFT			5
#define AXIS_AIRCRAFT_AILERON		6
#define AXIS_AIRCRAFT_ELEVATOR		7
#define AXIS_AIRCRAFT_RUDDER		8
#define AXIS_AIRCRAFT_THROTLE		9	
#define AXIS_BOAT_RUDDER			10
#define AXIS_BOAT_THROTLE			11
#define AXIS_BOAT_BOW				12
#define AXIS_VEHICLE_HYDRO_1_2		13
#define AXIS_VEHICLE_HYDRO_3_4		14
#define AXIS_VEHICLE_HYDRO_5_6		15
#define AXIS_VEHICLE_HYDRO_7_8		16
#define AXIS_VEHICLE_HYDRO_9_10		17
#define AXIS_VEHICLE_HYDRO_11_12	18
#define AXIS_AIRCRAFT_HYDRO_1_2		19
#define AXIS_AIRCRAFT_HYDRO_3_4		20
#define AXIS_AIRCRAFT_HYDRO_5_6		21
#define AXIS_AIRCRAFT_HYDRO_7_8		22
#define AXIS_AIRCRAFT_HYDRO_9_10	23
#define AXIS_AIRCRAFT_HYDRO_11_12	24
#define AXIS_BOAT_HYDRO_1_2			25
#define AXIS_BOAT_HYDRO_3_4			26
#define AXIS_BOAT_HYDRO_5_6			27
#define AXIS_BOAT_HYDRO_7_8			28
#define AXIS_BOAT_HYDRO_9_10		29
#define AXIS_BOAT_HYDRO_11_12		30
#define AXIS_VEHICLE_STICK			31
#define NUM_ROLES					32

static const char *axisname[] = {
        "Vehicle steering",
        "Vehicle throtle+brake",
		"Vehicle throtle only",
		"Vehicle brake only",
		"Vehicle manual clutch",
		"Vehicle manual shift",
		"Aircraft ailerons",
		"Aircraft elevators",
		"Aircraft rudder",
		"Aircraft throtle",
		"Boat rudder",
		"Boat engine",
		"Boat bow thruster",
		"Hydro F1/F2",
		"Hydro F3/F4",
		"Hydro F5/F6",
		"Hydro F7/F8",
		"Hydro F9/F10",
		"Hydro F11/F12",
		"Hydro F1/F2",
		"Hydro F3/F4",
		"Hydro F5/F6",
		"Hydro F7/F8",
		"Hydro F9/F10",
		"Hydro F11/F12",
		"Hydro F1/F2",
		"Hydro F3/F4",
		"Hydro F5/F6",
		"Hydro F7/F8",
		"Hydro F9/F10",
		"Hydro F11/F12",
		"FF Stick shift",
        NULL
}; 

static const char *axisprefix[] = {
        "Car",
        "Car",
		"Car",
		"Car",
		"Car",
		"Car",
		"Pln",
		"Pln",
		"Pln",
		"Pln",
		"Bot",
		"Bot",
		"Bot",
        "Car",
        "Car",
		"Car",
		"Car",
		"Car",
		"Car",
		"Pln",
		"Pln",
		"Pln",
		"Pln",
		"Pln",
		"Pln",
		"Bot",
		"Bot",
		"Bot",
		"Bot",
		"Bot",
		"Bot",
		"Car",
        NULL
}; 

typedef struct
{
	bool used;
	int jnum;
	int axis;
	int value;
	bool half;
	bool linear;
	bool reverse;
} axis_t;


class BeamJoystick
{
private:

	int numjoy;
	OIS::JoyStick* joys[64];

	bool usefeedback;
	bool noforce;
	int dz;

	axis_t axes[256];
	int free_axe;
	int roles[NUM_ROLES];

	int sticknum;
	int povjoy;
	int pov;

public:
	BeamJoystick(OIS::InputManager *im, float deadzone, bool feedback, ConfigFile *cfg);
	~BeamJoystick();

void UpdateInputState();

 float dead(int axis);

 float logval(float val);

 float getAxis(int type);

 int getHPov();
 int getVPov();
 bool povLeft();
 bool povRight();
 bool povUp();
 bool povDown();
bool hasJoy(int type);
bool hasForce();
//stick
int updateStickShift(bool enabled, float clutch);
//FORCE FEEDBACK STUFF
void setForceFeedback(float fx, float fy);


};


#endif

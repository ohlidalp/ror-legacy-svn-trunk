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
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 19th of October 2011

#ifndef DASHBOARDMANAGER_H__
#define DASHBOARDMANAGER_H__

#include "RoRPrerequisites.h"
#include <OgreSingleton.h>

#define DD_MAXCHAR 255
#define DD_MAX_SCREWPROP  6
#define DD_MAX_AEROENGINE 6
#define DD_MAX_WING       6

typedef union dataContainer_t
{
	bool  value_bool;
	int   value_int;
	float value_float;
	char  value_char[DD_MAXCHAR];
} dataContainer_t;

enum {DC_BOOL, DC_INT, DC_FLOAT, DC_CHAR, DC_INVALID};

typedef struct dashData_t
{
	char type; // DC_*
	dataContainer_t data;
	char *name; // char string of name

	dashData_t() : type(DC_INVALID)
	{
	}
	dashData_t(char type, char *name) : type(type), name(name)
	{
	};

} dashData_t;

// DashData enum definition
// important: if you add things here, also change the initialization in the constructor
enum {
	DD_ENGINE_RPM,         // engine RPM
	DD_ENGINE_SPEEDO_KPH,  // speedo in kilometer per hour
	DD_ENGINE_SPEEDO_MPH,  // speedo in miles per hour
	DD_ENGINE_TURBO,       // turbo gauge
	DD_ENGINE_IGNITION,    // ignition on engine
	DD_ENGINE_BATTERY,     // battery lamp
	DD_ENGINE_CLUTCH_WARNING, // clutch warning lamp

	DD_ENGINE_GEAR,        // current gear
	DD_ENGINE_NUM_GEAR,    // amount of gears
	DD_ENGINE_GEAR_STRING, // string like "<current gear>/<max gear>"
	DD_ENGINE_AUTO_GEAR,   // automatic gear

	DD_ENGINE_CLUTCH,      // the engines clutch
	
	DD_BRAKE,              // the brake application in % 0-1
	DD_ACCELERATOR,        // accelerator pedal in %, 0-1

	DD_ROLL,               // roll of the chassis
	DD_ROLL_CORR,          // correction roll of the chassis
	DD_ROLL_CORR_ACTIVE,   // correction rolling active

	DD_PITCH,              // chassis pitch

	DD_PARKINGBRAKE,       // parking brake status
	DD_LOCKED,             // locked lamp

	DD_LOW_PRESSURE,       // low pressure
	DD_LIGHTS,             // lights on

	DD_TRACTIONCONTROL_MODE, // TC
	DD_ANTILOCKBRAKE_MODE, // ALB
	DD_TIES_MODE,          // ties locked

	// water things
	DD_SCREW_THROTTLE_0,   // throttle for screwprop 0
	DD_SCREW_THROTTLE_1,
	DD_SCREW_THROTTLE_2,
	DD_SCREW_THROTTLE_3,
	DD_SCREW_THROTTLE_4,
	DD_SCREW_THROTTLE_5,   // if you add some, change DD_MAX_SCREWPROP and DD_SCREW_STEER_*

	DD_SCREW_STEER_0,      // steering direction of screw 0
	DD_SCREW_STEER_1,      // steering direction of screw 1
	DD_SCREW_STEER_2,      // steering direction of screw 2
	DD_SCREW_STEER_3,      // steering direction of screw 3
	DD_SCREW_STEER_4,      // steering direction of screw 4
	DD_SCREW_STEER_5,      // steering direction of screw 4,  if you add some, change DD_MAX_SCREWPROP and DD_SCREW_THROTTLE_*

	DD_WATER_DEPTH,        // how much water is under the boat
	DD_WATER_SPEED,        // speed in knots
	
	// airplane things
	DD_AEROENGINE_THROTTLE_0,
	DD_AEROENGINE_THROTTLE_1,
	DD_AEROENGINE_THROTTLE_2,
	DD_AEROENGINE_THROTTLE_3,
	DD_AEROENGINE_THROTTLE_4,
	DD_AEROENGINE_THROTTLE_5,

	DD_AEROENGINE_FAILED_0,
	DD_AEROENGINE_FAILED_1,
	DD_AEROENGINE_FAILED_2,
	DD_AEROENGINE_FAILED_3,
	DD_AEROENGINE_FAILED_4,
	DD_AEROENGINE_FAILED_5,

	DD_AEROENGINE_RPM_0,
	DD_AEROENGINE_RPM_1,
	DD_AEROENGINE_RPM_2,
	DD_AEROENGINE_RPM_3,
	DD_AEROENGINE_RPM_4,
	DD_AEROENGINE_RPM_5,

	DD_AIRSPEED,           // speed in air

	DD_WING_AOA_0,
	DD_WING_AOA_1,
	DD_WING_AOA_2,
	DD_WING_AOA_3,
	DD_WING_AOA_4,
	DD_WING_AOA_5,

	DD_ALTITUDE,
	DD_ALTITUDE_STRING,

	DD_EDITOR_NODE_INFO,   // editor node info string
	DD_MAX
};

// this class is NOT intended to be thread safe - performance is required
class DashBoardManager
{
public:
	DashBoardManager(void);
	~DashBoardManager(void);

	// Getter / Setter
	inline bool  getBool(size_t key)  { return data[key].data.value_bool; };
	inline int   getInt(size_t key)   { return data[key].data.value_int; };
	inline float getFloat(size_t key) { return data[key].data.value_float; };
	inline char *getChar(size_t key)  { return data[key].data.value_char; };

	inline void setBool(size_t key, bool &val)   { data[key].data.value_bool  = val; };
	inline void setInt(size_t key, int &val)     { data[key].data.value_int   = val; };
	inline void setFloat(size_t key, float &val) { data[key].data.value_float = val; };
	inline void setChar(size_t key, const char *val)  { strncpy(data[key].data.value_char, val, DD_MAXCHAR); };

protected:
	dashData_t data[DD_MAX];

};


#endif //DASHBOARDMANAGER_H__

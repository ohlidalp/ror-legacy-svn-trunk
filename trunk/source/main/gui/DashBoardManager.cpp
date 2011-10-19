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

#include "DashBoardManager.h"
#include "Settings.h"
#include "language.h"
#include "DashBoardManager.h"

#include <Ogre.h>
using namespace Ogre;

#define INITDATA(key, type, name) data[key] = dashData_t(type, name)

DashBoardManager::DashBoardManager(void)
{
	// init data
	INITDATA(DD_ENGINE_RPM, DC_FLOAT, "rpm");
	INITDATA(DD_ENGINE_SPEEDO_KPH, DC_FLOAT, "speedo_kph");
	INITDATA(DD_ENGINE_SPEEDO_MPH, DC_FLOAT, "speedo_mph");
	INITDATA(DD_ENGINE_TURBO, DC_FLOAT, "engine_turbo");
	INITDATA(DD_ENGINE_IGNITION, DC_BOOL, "engine_ignition");
	INITDATA(DD_ENGINE_BATTERY, DC_BOOL, "engine_battery");
	INITDATA(DD_ENGINE_CLUTCH_WARNING, DC_BOOL, "engine_clutch_warning");
	
	
	INITDATA(DD_ENGINE_GEAR, DC_INT, "engine_gear");
	INITDATA(DD_ENGINE_NUM_GEAR, DC_INT, "engine_num_gear");
	INITDATA(DD_ENGINE_GEAR_STRING, DC_CHAR, "engine_gear_string");
	INITDATA(DD_ENGINE_AUTO_GEAR, DC_INT, "engine_auto_gear");
	
	INITDATA(DD_ENGINE_CLUTCH, DC_FLOAT, "engine_clutch");

	INITDATA(DD_BRAKE, DC_FLOAT, "brake");
	INITDATA(DD_ACCELERATOR, DC_FLOAT, "accelerator");
	
	INITDATA(DD_ROLL, DC_FLOAT, "roll");
	INITDATA(DD_ROLL_CORR, DC_FLOAT, "roll_corr");
	INITDATA(DD_ROLL_CORR_ACTIVE, DC_BOOL, "roll_corr_active");
	INITDATA(DD_PITCH, DC_FLOAT, "pitch");

	
	INITDATA(DD_PARKINGBRAKE, DC_BOOL, "parkingbrake");
	INITDATA(DD_LOCKED, DC_BOOL, "locked");
	INITDATA(DD_LOW_PRESSURE, DC_BOOL, "low_pressure");
	INITDATA(DD_LIGHTS, DC_BOOL, "lights");
	
	INITDATA(DD_TRACTIONCONTROL_MODE, DC_INT, "tractioncontrol_mode");
	INITDATA(DD_ANTILOCKBRAKE_MODE, DC_INT, "antilockbrake_mode");
	
	INITDATA(DD_TIES_MODE, DC_INT, "ties_mode");
	
	INITDATA(DD_SCREW_THROTTLE_0, DC_FLOAT, "screw_throttle_0");
	INITDATA(DD_SCREW_THROTTLE_1, DC_FLOAT, "screw_throttle_1");
	INITDATA(DD_SCREW_THROTTLE_2, DC_FLOAT, "screw_throttle_2");
	INITDATA(DD_SCREW_THROTTLE_3, DC_FLOAT, "screw_throttle_3");
	INITDATA(DD_SCREW_THROTTLE_4, DC_FLOAT, "screw_throttle_4");
	INITDATA(DD_SCREW_THROTTLE_5, DC_FLOAT, "screw_throttle_5");
	
	INITDATA(DD_SCREW_STEER_0, DC_FLOAT, "screw_steer_0");
	INITDATA(DD_SCREW_STEER_1, DC_FLOAT, "screw_steer_1");
	INITDATA(DD_SCREW_STEER_2, DC_FLOAT, "screw_steer_2");
	INITDATA(DD_SCREW_STEER_3, DC_FLOAT, "screw_steer_3");
	INITDATA(DD_SCREW_STEER_4, DC_FLOAT, "screw_steer_4");
	INITDATA(DD_SCREW_STEER_5, DC_FLOAT, "screw_steer_5");

	INITDATA(DD_WATER_DEPTH, DC_FLOAT, "water_depth");
	INITDATA(DD_WATER_SPEED, DC_FLOAT, "water_speed");
	
	INITDATA(DD_AEROENGINE_THROTTLE_0, DC_FLOAT, "aeroengine_throttle_0");
	INITDATA(DD_AEROENGINE_THROTTLE_1, DC_FLOAT, "aeroengine_throttle_1");
	INITDATA(DD_AEROENGINE_THROTTLE_2, DC_FLOAT, "aeroengine_throttle_2");
	INITDATA(DD_AEROENGINE_THROTTLE_3, DC_FLOAT, "aeroengine_throttle_3");
	INITDATA(DD_AEROENGINE_THROTTLE_4, DC_FLOAT, "aeroengine_throttle_4");
	INITDATA(DD_AEROENGINE_THROTTLE_5, DC_FLOAT, "aeroengine_throttle_5");
	
	INITDATA(DD_AEROENGINE_FAILED_0, DC_BOOL, "aeroengine_onfire_0");
	INITDATA(DD_AEROENGINE_FAILED_1, DC_BOOL, "aeroengine_onfire_1");
	INITDATA(DD_AEROENGINE_FAILED_2, DC_BOOL, "aeroengine_onfire_2");
	INITDATA(DD_AEROENGINE_FAILED_3, DC_BOOL, "aeroengine_onfire_3");
	INITDATA(DD_AEROENGINE_FAILED_4, DC_BOOL, "aeroengine_onfire_4");
	INITDATA(DD_AEROENGINE_FAILED_5, DC_BOOL, "aeroengine_onfire_5");

	INITDATA(DD_AEROENGINE_RPM_0, DC_FLOAT, "aeroengine_rpm_0");
	INITDATA(DD_AEROENGINE_RPM_1, DC_FLOAT, "aeroengine_rpm_1");
	INITDATA(DD_AEROENGINE_RPM_2, DC_FLOAT, "aeroengine_rpm_2");
	INITDATA(DD_AEROENGINE_RPM_3, DC_FLOAT, "aeroengine_rpm_3");
	INITDATA(DD_AEROENGINE_RPM_4, DC_FLOAT, "aeroengine_rpm_4");
	INITDATA(DD_AEROENGINE_RPM_5, DC_FLOAT, "aeroengine_rpm_5");
	
	INITDATA(DD_AIRSPEED, DC_FLOAT, "airspeed");
	
	INITDATA(DD_WING_AOA_0, DC_FLOAT, "wing_aoa_0");
	INITDATA(DD_WING_AOA_1, DC_FLOAT, "wing_aoa_1");
	INITDATA(DD_WING_AOA_2, DC_FLOAT, "wing_aoa_2");
	INITDATA(DD_WING_AOA_3, DC_FLOAT, "wing_aoa_3");
	INITDATA(DD_WING_AOA_4, DC_FLOAT, "wing_aoa_4");
	INITDATA(DD_WING_AOA_5, DC_FLOAT, "wing_aoa_5");

	INITDATA(DD_ALTITUDE, DC_FLOAT, "altitude");
	INITDATA(DD_ALTITUDE_STRING, DC_CHAR, "altitude_string");

	INITDATA(DD_EDITOR_NODE_INFO, DC_CHAR, "editor_node_info");
	
	

}


DashBoardManager::~DashBoardManager(void)
{
}

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
// created on 24th of February 2009 by Thomas Fischer
#ifdef ANGELSCRIPT

#ifndef SCRIPTENGINE_H__
#define SCRIPTENGINE_H__

#include <string>
#include "angelscript.h"
#include "Ogre.h"

class ExampleFrameListener;
class GameScript;
class Beam;

#define AS_INTERFACE_VERSION "0.1.0" //!< versioning for the scripting interface

/**
 * @file ScriptEngine.h
 * @version 0.1.0
 * @brief AngelScript interface to the game
 * @authors Thomas Fischer (thomas{AT}rigsofrods{DOT}com)
 */

/**
 *  @brief This class represents the angelscript scripting interface. It can load and execute scripts.
 */
class ScriptEngine
{
public:
	ScriptEngine(ExampleFrameListener *efl);
	~ScriptEngine();

	/**
	 * Loads a script bound to a terrain
	 * @param terrainName name of the loaded terrain. I.e. 'nhelens.terrn'
	 * @return 0 on success, everything else on error
	 */
	int loadTerrainScript(Ogre::String terrainName);

	/**
	 * Calls the script's framestep function to be able to use timed things inside the script
	 * @param dt time passed since the last call to this function in seconds
	 * @return 0 on success, everything else on error
	 */
	int framestep(Ogre::Real dt);

	/**
	 * This enum describes what events are existing. The script can register to receive events.
	 */
	enum scriptEvents
	{
		SE_COLLISION_BOX_ENTER,             //!< called when truck or person enters a previous registered collision box
		SE_COLLISION_BOX_LEAVE,             //!< called when truck or person leaves a previous registered and entered collision box

		SE_TRUCK_ENTER,                     //!< called when switching from person mode to truck mode
		SE_TRUCK_EXIT,                      //!< called when switching from truck mode to person mode

		SE_TRUCK_ENGINE_DIED,               //!< called when the trucks engine dies (from underrev, water, etc)
		SE_TRUCK_ENGINE_FIRE,               //!< called when the planes engines start to get on fire
		SE_TRUCK_TOUCHED_WATER,             //!< called when any part of the truck touches water
		SE_TRUCK_BEAM_BROKE,                //!< called when a beam breaks
		SE_TRUCK_LOCKED,                    //!< called when the truck got lock to another truck
		SE_TRUCK_UNLOCKED,                  //!< called when the truck unlocks again
		SE_TRUCK_LIGHT_TOGGLE,              //!< called when the main light is toggled
		SE_TRUCK_SKELETON_TOGGLE,           //!< called when the user enters or exits skeleton mode
		SE_TRUCK_TIE_TOGGLE,                //!< called when the user toggles ties
		SE_TRUCK_PARKINGBREAK_TOGGLE,       //!< called when the user toggles the parking break
		SE_TRUCK_BEACONS_TOGGLE,            //!< called when the user toggles beacons
		SE_TRUCK_CPARTICLES_TOGGLE,         //!< called when the user toggles custom particles
		SE_TRUCK_GROUND_CONTACT_CHANGED,    //!< called when the trucks ground contact changed (no contact, different ground models, etc)

		SE_GENERIC_NEW_TRUCK,               //!< called when the user spawns a new truck
		SE_GENERIC_DELETED_TRUCK,           //!< called when the user deletes a truck

		SE_GENERIC_INPUT_EVENT,             //!< called when an input event bound to the scripting engine is toggled
		SE_GENERIC_MOUSE_BEAM_INTERACTION,  //!< called when the user uses the mouse to interact with the truck

	};
protected:
    ExampleFrameListener *mefl;             //!< local Exampleframelistener instance, used as proxy for many functions
    asIScriptEngine *engine;                //!< instance of the scripting engine
	asIScriptContext *context;              //!< context in which all scripting happens
	int frameStepFunctionPtr;               //!< script function pointer to the frameStep function
	int eventCallbackFunctionPtr;           //!< script function pointer to the event callback function

	/**
	 * This function initialzies the engine and registeres all types
	 */
    void init();
    
	/**
	 * This is the callback function that gets called when script error occur.
	 * When the script crashes, this function will provide you with more detail
	 * @param msg arguments that contain details about the crash
	 * @param param unkown?
	 */
    void msgCallback(const asSMessageInfo *msg, void *param);

	/**
	 * This function reads a file into the provided string.
	 * @param filename filename of the file that should be loaded into the script string
	 * @param script reference to a string where the contents of the file is written to
	 * @return 0 on success, everything else on error
	 */
	int loadScriptFile(const char *fileName, std::string &script);

	// undocumented debugging functions below, not working.
	void ExceptionCallback(asIScriptContext *ctx, void *param);
	void PrintVariables(asIScriptContext *ctx, int stackLevel);
	void LineCallback(asIScriptContext *ctx, void *param);
};


/**
 *  @brief Proxy class that can be called by script functions
 */
class GameScript
{
protected:
	ScriptEngine *mse;              //!< local script engine pointer, used as proxy mostly
	ExampleFrameListener *mefl;     //!< local pointer to the main ExampleFrameListener, used as proxy mostly

public:
	/**
	 * constructor
	 * @param se pointer to the ScriptEngine instance
	 * @param efl pointer to the ExampleFrameListener instance
	 */
	GameScript(ScriptEngine *se, ExampleFrameListener *efl);

	/**
	 * destructor
	 */
	~GameScript();

	/**
	 * writes a message to the games log (RoR.log)
	 * @param msg string to log
	 */
	void log(std::string &msg);

	/**
	 * returns the time in seconds since the game was started
	 * @return time in seconds
	 */
	double getTime();

	/**
	 * sets the character position
	 * @param x X position on the terrain
	 * @param y Y position on the terrain
	 * @param z Z position on the terrain
	 */
	void setPersonPosition(float x, float y, float z);

	/**
	 * moves the person relative
	 * @param x X translation
	 * @param y Y translation
	 * @param z Z translation
	 */
	void movePerson(float x, float y, float z);

	/**
	 * gets the time of the day in seconds
	 * @return day time in seconds
	 */
	float getCaelumTime();
	
	/**
	 * sets the time of the day in seconds
	 * @param value day time in seconds
	 */
	void setCaelumTime(float value);
	
	/**
	 * returns the current base water level (without waves)
	 * @return water height in meters
	 */
	float getWaterHeight();

	/**
	 * sets the base water height
	 * @param value base height in meters
	 */
	void setWaterHeight(float value);

	/**
	 * returns the current selected truck, 0 if in person mode
	 * @return reference to Beam object that is currently in use
	 */
	Beam *getCurrentTruck();

	/**
	 * returns a truck by index, get max index by calling getNumTrucks
	 * @return reference to Beam object that the selected slot
	 */
	Beam *getTruckByNum(int num);

	/**
	 * returns the current amount of loaded trucks
	 * @return integer value representing the amount of loaded trucks
	 */
	int getNumTrucks();

	/**
	 * returns the current truck number. >=0 when using a truck, -1 when in person mode
	 * @return integer truck number
	 */
	int getCurrentTruckNumber();
	
	/**
	 * returns the currently set upo gravity
	 * @return float number describing gravity terrain wide.
	 */
	float getGravity();
	
	/**
	 * sets the gravity terrain wide. This is an expensive call, since the masses of all trucks are recalculated.
	 * @param value new gravity terrain wide (default is -9.81)
	 */
	void setGravity(float value);
};

#endif

#endif //SCRIPTENGINE_H__

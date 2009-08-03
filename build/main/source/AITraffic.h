#ifdef OPENSTEER
#pragma once
#ifndef AITraffic_H
#define AITraffic_H

#ifdef ANGELSCRIPT
#include "ScriptEngine.h"
#include "angelscript.h"
#endif

#include "Ogre.h"
#include "OgreVector3.h"
#include "OgreMaterial.h"

#include "OpenSteer/Pathway.h"
#include "AITraffic_Common.h"
#include "AITraffic_Vehicle.h"

class ExampleFrameListener;

class AITraffic
{
	public:
		AITraffic(ScriptEngine *engine);
		~AITraffic();

		void load();
		void initialize();
		void frameStep(Ogre::Real deltat);

		trafficgrid_t trafficgrid;

		Ogre::Vector3		playerpos;		// we store here the player's position and rotation
		Ogre::Quaternion	playerrot;		// used for creating interactivity layer in traffic

// Traffic lights management
		void registerTrafficLamp(int id, int group_id, int role, int name);	// register traffic lamp to the system
		void setTrafficLampGroupServiceMode(int mode);						// mode: 0 - normal, 1 - blinking, 2 - out of order			

		
	private:
		void checkForZones();							// checking if event should be triggered
		void updateLamps();
		void processOneCar(int idx, float delta);		// update a vehicle position (by OpenSteer)

		// duplicated from AS for performance issues
		int setMaterialAmbient(const std::string &materialName, float red, float green, float blue);
		int setMaterialDiffuse(const std::string &materialName, float red, float green, float blue, float alpha);
		int setMaterialSpecular(const std::string &materialName, float red, float green, float blue, float alpha);
		int setMaterialEmissive(const std::string &materialName, float red, float green, float blue);
		void spawnObject(const std::string &objectName, const std::string &instanceName, float px, float py, float pz, float rx, float ry, float rz, const std::string &eventhandler, bool uniquifyMaterials);
		void setSignalState(Ogre::String instance, int state);

		Ogre::Vector3 waypoints[200];
		Ogre::Vector3 turnpoints[200];
		int wpi;
		int max_wpi;
		Ogre::Real mTotalElapsedTime;
		Ogre::Real mLampTimer;

		int num_of_vehicles;
		int num_of_waypoints;
		AITraffic_Vehicle *vehicles[NUM_OF_TRAFFICED_CARS];
		float rs;
		ScriptEngine *scriptengine;
		int cnt;

		// intersection management
		int num_of_intersections;
		trafficintersection_t intersection[NUM_OF_INTERSECTIONS];
};

#endif
#endif //OPENSTEER

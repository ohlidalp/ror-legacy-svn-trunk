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
#ifndef __TerrainManager_H_
#define __TerrainManager_H_

#include "RoRPrerequisites.h"

#include <OgreConfigFile.h>

class TerrainManager
{
public:

	TerrainManager();
	~TerrainManager();

	void loadTerrain(Ogre::String filename);
	
	void update(float dt) {};

	void setGravity(float value);
	float getGravity() { return gravity; };

	Ogre::Vector3 getMax() { return Ogre::Vector3::ZERO; };

	Collisions *getCollisions() { return collisions; };
	Envmap *getEnvmap() { return envmap; };
	IHeightFinder *getHeightFinder() { return reinterpret_cast<IHeightFinder *>(geometry_manager); };
	SkyManager *getSkyManager() { return sky_manager; };
	Water *getWater() { return water; };

protected:
	// members
	Ogre::ConfigFile mTerrainConfig;

	// subsystems
	Character *character;
	Collisions *collisions;
	Dashboard *dashboard;
	Envmap *envmap;
	HDRListener *hdr_listener;
	MapControl *survey_map;
	ShadowManager *shadow_manager;
	SkyManager *sky_manager;
	TerrainGeometryManager *geometry_manager;
	TerrainObjectManager *object_manager;
	Water *water;	

	// properties
	Ogre::ColourValue ambient_color;
	Ogre::ColourValue fade_color;
	Ogre::Light *main_light;
	Ogre::String fileHash;
	Ogre::String ogre_terrain_config_filename;
	Ogre::String terrain_name;
	Ogre::Vector3 start_position;
	bool use_caelum;
	float gravity;
	float water_line;
	int farclip;
	int loading_state;

	// internal methods
	void initCamera();
	void initCollisions();
	void initDashboards();
	void initEnvironmentMap();
	void initFog();
	void initGlow();
	void initHDR();
	void initLight();
	void initMotionBlur();
	void initScripting();
	void initShadows();
	void initSkySubSystem();
	void initSubSystems();
	void initSunburn();
	void initVegetation();
	void initWater();
	void initSurveyMap();

	void fixCompositorClearColor();

	void loadTerrainObjects();
};

#endif // __TerrainManager_H_

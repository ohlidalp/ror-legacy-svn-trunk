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

	TerrainManager(Ogre::SceneManager *smgr, Ogre::RenderWindow *window, Ogre::Camera *camera, Character *character);
	~TerrainManager();

	void loadTerrain(Ogre::String filename);
	
	inline Collisions *getCollisions() { return collisions; };
	inline Water *getWater() { return water; };
	inline Envmap *getEnvmap() { return envmap; };

	void setGravity(float value);
	float getGravity() { return gravity; };

protected:
	// members
	Ogre::Camera *mCamera;
	Ogre::ConfigFile mTerrainConfig;
	Ogre::RenderWindow *mWindow;
	Ogre::SceneManager *mSceneMgr;
	Character *mCharacter;

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
	TerrainHeightFinder *height_finder;
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
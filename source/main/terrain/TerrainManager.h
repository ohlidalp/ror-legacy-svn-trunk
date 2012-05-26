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
#ifndef TERRAINMANAGER_H__
#define TERRAINMANAGER_H__

#include "RoRPrerequisites.h"
#include <OgreConfigFile.h>

class TerrainManager
{
public:
	TerrainManager(Ogre::SceneManager *smgr, Ogre::RenderWindow *window, Ogre::Camera *camera);
	~TerrainManager();

	void loadTerrain(Ogre::String filename);

	void setGravity(float value);
	float getGravity();


	inline Collisions *getCollisions() { return collisions; };
	inline Water *getWater() { return water; };
	inline Envmap *getEnvmap() { return envmap; };

protected:
	// members
	Ogre::SceneManager *mSceneMgr;
	Ogre::RenderWindow *mWindow;
	Ogre::Camera *mCamera;
	Ogre::ConfigFile terrainConfig;

	// subsystems
	TerrainObjectManager *objectManager;
	TerrainGeometryManager *geometryManager;
	TerrainHeightFinder *heightFinder;
	ShadowManager *shadowManager;
	SkyManager *skyManager;
	Envmap *envmap;
	Dashboard *dashboard;
	Collisions *collisions;
	Water *water;
	MapControl *surveyMap;

	// subsystem properties
	HDRListener *hdrListener;


	// properties
	Ogre::String fileHash;
	Ogre::String terrainName;
	Ogre::String ogreTerrainConfigFilename;

	float waterLine;
	Ogre::ColourValue ambientColor;
	Ogre::Vector3 startPosition;
	bool useCaelum;
	int farclip;
	float gravity;


	Ogre::Light *mainLight;


	// internal methods
	void initSubSystems();
	void initCamera();
	void initSkySubSystem();
	void initLight();
	void initFog();
	void initVegetation();
	void initHDR();
	void initGlow();
	void initMotionBlur();
	void initSunburn();
	void initWater();
	void initEnvironmentMap();
	void initDashboards();
	void initHeightFinder();
	void initShadows();
	void initCollisions();
	void initScripting();
	void initSurveyMap();

	void fixCompositorClearColor();

	void loadTerrainObjects();
};
#endif // TERRAINMANAGER_H__


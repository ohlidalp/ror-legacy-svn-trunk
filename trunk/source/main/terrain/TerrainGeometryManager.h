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
#ifndef TerrainGeometryManager_H__
#define TerrainGeometryManager_H__

#include "RoRPrerequisites.h"

#include <OgreTerrain.h>
#include <OgreTerrainMaterialGeneratorA.h>
#include <OgreTerrainPaging.h>
#include <OgreTerrainQuadTreeNode.h>
#include <OgreTerrainGroup.h>

#include <OgreConfigFile.h>

// this class handles all interactions with the Ogre Terrain system
class TerrainGeometryManager
{
public:
	TerrainGeometryManager(Ogre::SceneManager *smgr, TerrainManager *terrainManager);
	~TerrainGeometryManager();

	void loadOgreTerrainConfig(Ogre::String filename);

	inline Ogre::TerrainGroup *getTerrainGroup() { return mTerrainGroup; };


	inline float getHeightAt(float x, float z)
	{
		return mTerrainGroup->getHeightAtWorldPosition(x, 1000, z);
	}

	inline Ogre::Vector3 getNormalAt(float x, float y, float z, float precision = 0.1f)
	{
		Ogre::Vector3 left(-precision, getHeightAt(x - precision, z) - y, 0.0f);
		Ogre::Vector3 down(0.0f, getHeightAt(x, z + precision) - y, precision);
		down = left.crossProduct(down);
		down.normalise();
		return down;
	}

protected:
	bool disableCaching;
	bool mTerrainsImported;
	Ogre::SceneManager *mSceneMgr;
	TerrainManager *terrainManager;
	int mapsizex, mapsizey, mapsizez, pageSize, terrainSize, worldSize;
	Ogre::String baseName;
	Ogre::ConfigFile terrainConfig;
	int pageMinX, pageMaxX, pageMinY, pageMaxY;
	int terrainLayers;
	TerrainObjectManager *objectManager;

	// terrain engine specific
	Ogre::TerrainGroup *mTerrainGroup;
	Ogre::TerrainPaging* mTerrainPaging;
	Ogre::PageManager* mPageManager;

	typedef struct blendLayerInfo_t {
		Ogre::String blendMapTextureFilename;
		char blendMode;
		float alpha;
	} blendLayerInfo_t;

	std::vector<blendLayerInfo_t> blendInfo;

	void initTerrain();
	bool loadTerrainConfig(Ogre::String filename);
	void configureTerrainDefaults();
	void defineTerrain(int x, int y, bool flat=false);
	void initBlendMaps( Ogre::Terrain* t );
	void getTerrainImage(int x, int y, Ogre::Image& img);
};
#endif // TerrainGeometryManager_H__



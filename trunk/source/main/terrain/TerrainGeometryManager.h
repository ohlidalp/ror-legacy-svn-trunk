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
#ifndef __TerrainGeometryManager_H_
#define __TerrainGeometryManager_H_

#include "RoRPrerequisites.h"

#include "IHeightFinder.h"

#include <OgreTerrain.h>
#include <OgreTerrainMaterialGeneratorA.h>
#include <OgreTerrainPaging.h>
#include <OgreTerrainQuadTreeNode.h>
#include <OgreTerrainGroup.h>

#include <OgreConfigFile.h>

// this class handles all interactions with the Ogre Terrain system
class TerrainGeometryManager : public IHeightFinder, public ZeroedMemoryAllocator
{
public:
	TerrainGeometryManager(TerrainManager *terrainManager);
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

	Ogre::ConfigFile terrainConfig;
	Ogre::String baseName;
	TerrainManager *terrainManager;
	TerrainObjectManager *objectManager;
	bool disableCaching;
	bool mTerrainsImported;
	int mapsizex, mapsizey, mapsizez, pageSize, terrainSize, worldSize;
	int pageMinX, pageMaxX, pageMinY, pageMaxY;
	int terrainLayers;

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

	bool loadTerrainConfig(Ogre::String filename);
	void configureTerrainDefaults();
	void defineTerrain(int x, int y, bool flat=false);
	void getTerrainImage(int x, int y, Ogre::Image& img);
	void initBlendMaps( Ogre::Terrain* t );
	void initTerrain();
};

#endif // __TerrainGeometryManager_H_

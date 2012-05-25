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

#include <OgreTerrain.h>
#include <OgreTerrainMaterialGeneratorA.h>
#include <OgreTerrainPaging.h>
#include <OgreTerrainQuadTreeNode.h>
#include <OgreTerrainGroup.h>

#include <OgreConfigFile.h>

class TerrainManager
{
public:
	TerrainManager(Ogre::SceneManager *smgr);
	~TerrainManager();

	void loadTerrain(Ogre::String filename);

	inline Ogre::TerrainGroup *getTerrainGroup() { return mTerrainGroup; };

protected:
	bool disableCaching;
	bool mTerrainsImported;
	Ogre::SceneManager *mSceneMgr;
	int mapsizex, mapsizey, mapsizez, pageSize, terrainSize, worldSize;
	Ogre::String baseName;
	Ogre::ConfigFile terrainConfig;
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

	void initTerrain();
	bool loadTerrainConfig(Ogre::String filename);
	void configureTerrainDefaults();
	void defineTerrain(int x, int y, bool flat=false);
	void initBlendMaps( Ogre::Terrain* t );
	void getTerrainImage(int x, int y, Ogre::Image& img);
};
#endif // TERRAINMANAGER_H__



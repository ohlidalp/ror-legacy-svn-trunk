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
#include "TerrainManager.h"

#include "BeamFactory.h"
#include "CameraManager.h"
#include "RoRFrameListener.h"
#include "SkyManager.h"

using namespace Ogre;

TerrainManager::TerrainManager(Ogre::SceneManager *smgr) :
	  mSceneMgr(smgr)
	, disableCaching(false)
	, mTerrainsImported(false)
{
}

TerrainManager::~TerrainManager()
{

}

void TerrainManager::loadTerrain(String filename)
{
	String ext;
	Ogre::StringUtil::splitBaseFilename(filename, baseName, ext);

	loadTerrainConfig(filename);

	if (!terrainConfig.getSetting("disableCaching").empty())
		disableCaching = StringConverter::parseBool(terrainConfig.getSetting("disableCaching"));

	initTerrain();
}


bool TerrainManager::loadTerrainConfig(String filename)
{
	try
	{
		DataStreamPtr ds_config = ResourceGroupManager::getSingleton().openResource(filename, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		terrainConfig.load(ds_config, "\t:=", false);
	}catch(...)
	{
		terrainConfig.clear();
	}
	return true;
}


void TerrainManager::initTerrain()
{
	// X, Y and Z scale
	mapsizex = PARSEINT(terrainConfig.getSetting("PageWorldX"));
	mapsizey = PARSEINT(terrainConfig.getSetting("MaxHeight"));
	mapsizez = PARSEINT(terrainConfig.getSetting("PageWorldZ"));
	pageSize = PARSEINT(terrainConfig.getSetting("PageSize"));
	terrainSize = pageSize;
	worldSize = std::max(mapsizex, mapsizez);

	String terrainFileSuffix = "mapbin";
	pageMinX = 0;
	pageMaxX = 0;
	pageMinY = 0;
	pageMaxY = 0;

	pageMaxX = PARSEINT(terrainConfig.getSetting("Pages_X"));
	pageMaxY = PARSEINT(terrainConfig.getSetting("Pages_Y"));


	Vector3 mTerrainPos(mapsizex*0.5f, 0, mapsizez * 0.5f);

	mTerrainGroup = OGRE_NEW TerrainGroup(mSceneMgr, Terrain::ALIGN_X_Z, terrainSize, worldSize);
	mTerrainGroup->setFilenameConvention(baseName, terrainFileSuffix);
	mTerrainGroup->setOrigin(mTerrainPos);
	mTerrainGroup->setResourceGroup("cache");

	OGRE_NEW TerrainGlobalOptions();
	// Configure global

	configureTerrainDefaults();

	String filename = mTerrainGroup->generateFilename(0, 0);
	bool is_cached  = ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), filename);
	if (disableCaching || !is_cached)
	{
		for (long x = pageMinX; x <= pageMaxX; ++x)
			for (long y = pageMinY; y <= pageMaxY; ++y)
				defineTerrain(x, y);

		// sync load since we want everything in place when we start
		mTerrainGroup->loadAllTerrains(true);

		if (mTerrainsImported)
		{
			TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
			while(ti.hasMoreElements())
			{
				Terrain *terrain = ti.getNext()->instance;
				//ShadowManager::getSingleton().updatePSSM(terrain);
				//initBlendMaps(terrain);
			}
		}

		// always save the results
		//mTerrainGroup->saveAllTerrains(false);
	}

	//mTerrainGroup->freeTemporaryResources();
}


void TerrainManager::configureTerrainDefaults()
{
	Light *light = SkyManager::getSingleton().getMainLight();
	TerrainGlobalOptions *terrainOptions = TerrainGlobalOptions::getSingletonPtr();
	// Configure global
	terrainOptions->setMaxPixelError(PARSEINT(terrainConfig.getSetting("MaxPixelError")));
	// testing composite map
	terrainOptions->setCompositeMapDistance(PARSEINT(terrainConfig.getSetting("CompositeDistance")));

	// Important to set these so that the terrain knows what to use for derived (non-realtime) data
	if(light)
	{
		terrainOptions->setLightMapDirection(light->getDerivedDirection());
		terrainOptions->setCompositeMapDiffuse(light->getDiffuseColour());
	}
	terrainOptions->setCompositeMapAmbient(mSceneMgr->getAmbientLight());

	// Configure default import settings for if we use imported image
	Ogre::Terrain::ImportData& defaultimp = mTerrainGroup->getDefaultImportSettings();
	defaultimp.terrainSize  = terrainSize; // the heightmap size
	defaultimp.worldSize    = pageSize; // this is the scaled up size, like 12km
	defaultimp.inputScale   = mapsizey;
	defaultimp.minBatchSize = 33;
	defaultimp.maxBatchSize = 65;

	if (!terrainConfig.getSetting("minBatchSize").empty())
		defaultimp.minBatchSize = PARSEINT(terrainConfig.getSetting("minBatchSize"));

	if (!terrainConfig.getSetting("maxBatchSize").empty())
		defaultimp.maxBatchSize = PARSEINT(terrainConfig.getSetting("maxBatchSize"));

	/*
	// testing composite map




	// Configure default import settings for if we use imported image
	Terrain::ImportData& defaultimp = mTerrainGroup->getDefaultImportSettings();
	defaultimp.terrainSize  = pageSize;
	defaultimp.worldSize    = mapsizex;

	//TerrainGlobalOptions::getSingleton().setDefaultGlobalColourMapSize(pageSize);

	if (mCamera->getFarClipDistance() == 0)
		TerrainGlobalOptions::getSingleton().setCompositeMapDistance(1000.0f);
	else
		TerrainGlobalOptions::getSingleton().setCompositeMapDistance(std::min(1000.0f, mCamera->getFarClipDistance()));
	
	//terrainOptions->setUseRayBoxDistanceCalculation(true);

	// adds strange colours for debug purposes
	//TerrainGlobalOptions::getSingleton().getDefaultMaterialGenerator()->setDebugLevel(1);

	// TBD: optimizations
	TerrainMaterialGeneratorA::SM2Profile* matProfile = static_cast<TerrainMaterialGeneratorA::SM2Profile*>(TerrainGlobalOptions::getSingleton().getDefaultMaterialGenerator()->getActiveProfile());
	matProfile->setLightmapEnabled(true);
	//matProfile->setLayerNormalMappingEnabled(false);
	//matProfile->setLayerSpecularMappingEnabled(false);
	//matProfile->setLayerParallaxMappingEnabled(false);

	matProfile->setGlobalColourMapEnabled(false);
	matProfile->setReceiveDynamicShadowsDepth(true);


	TerrainGlobalOptions::getSingleton().setCompositeMapSize(1024);
	//TerrainGlobalOptions::getSingleton().setCompositeMapDistance(100);
	TerrainGlobalOptions::getSingleton().setSkirtSize(1);
	TerrainGlobalOptions::getSingleton().setLightMapSize(256);
	TerrainGlobalOptions::getSingleton().setCastsDynamicShadows(true);

	// Important to set these so that the terrain knows what to use for derived (non-realtime) data
	Light *mainLight = SkyManager::getSingleton().getMainLight();
	if (mainLight) TerrainGlobalOptions::getSingleton().setLightMapDirection(mainLight->getDerivedDirection());
	TerrainGlobalOptions::getSingleton().setCompositeMapAmbient(mSceneMgr->getAmbientLight());
	//terrainOptions->setCompositeMapAmbient(ColourValue::Red);
	if (mainLight) TerrainGlobalOptions::getSingleton().setCompositeMapDiffuse(mainLight->getDiffuseColour());
	*/


	// load the textures and blendmaps into our data structures
	blendMaps.clear();
	blendMode.clear();
	terrainLayers = StringConverter::parseInt(terrainConfig.getSetting("Layers.count"));
	if (terrainLayers > 0)
	{
		defaultimp.layerList.resize(terrainLayers);
		blendMaps.resize(terrainLayers);
		blendMode.resize(terrainLayers);
		for(int i = 0; i < terrainLayers; i++)
		{
			defaultimp.layerList[i].worldSize = PARSEINT(terrainConfig.getSetting("Layers."+TOSTRING(i)+".size"));
			defaultimp.layerList[i].textureNames.push_back(terrainConfig.getSetting("Layers."+TOSTRING(i)+".diffusespecular"));
			defaultimp.layerList[i].textureNames.push_back(terrainConfig.getSetting("Layers."+TOSTRING(i)+".normalheight"));
			blendMaps[i] = terrainConfig.getSetting("Layers."+TOSTRING(i)+".blendmap");
			blendMode[i] = terrainConfig.getSetting("Layers."+TOSTRING(i)+".blendmapmode");
		}
	}
}

void TerrainManager::initBlendMaps( Ogre::Terrain* terrain )
{
	for(int i = 1; i < terrain->getLayerCount(); i++)
	{
		Ogre::Image img;
		try
		{
			img.load(blendMaps[i-1], ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		} catch(Exception &e)
		{
			LOG("Error loading blendmap: " + blendMaps[i] + " : " + e.getFullDescription());
			continue;
		}

		TerrainLayerBlendMap *blendmap = terrain->getLayerBlendMap(i); // starting with 1, very strange ...

		// resize that blending map so it will fit
		Ogre::uint32 blendmapSize = terrain->getLayerBlendMapSize();
		if(img.getWidth() != blendmapSize)
			img.resize(blendmapSize, blendmapSize);

		// now to the ugly part
		float* ptr = blendmap->getBlendPointer();
		for (Ogre::uint32 x = 0; x != blendmapSize; x++)
		{
			for (Ogre::uint32 y = 0; y != blendmapSize; y++)
			{
				Ogre::ColourValue c = img.getColourAt(x, y, 0);
				float alpha = 1;//(1/b);
				if (blendMode[i] == "R")
					*ptr++ = c.r * alpha;
				else if (blendMode[i] == "G")
					*ptr++ = c.g * alpha;
				else if (blendMode[i] == "B")
					*ptr++ = c.b * alpha;
				else if (blendMode[i] == "A")
					*ptr++ = c.a * alpha;
			}
		}
		blendmap->dirty();
		blendmap->update();
	}
}

void TerrainManager::defineTerrain( int x, int y, bool flat )
{
	if (flat)
	{
		// very simple, no height data to load at all
		mTerrainGroup->defineTerrain(x, y, 0.0f);
		return;
	}
	String filename = mTerrainGroup->generateFilename(x, y);
	if (!disableCaching && ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), filename))
	{
		// load from cache
		mTerrainGroup->defineTerrain(x, y);
	}
	else
	{
		// create new from image
		String heightmapString = "Heightmap.image." + TOSTRING(x) + "." + TOSTRING(y);
		String heightmapFilename = terrainConfig.getSetting(heightmapString);
		if (heightmapFilename.empty())
		{
			// try loading the old non-paged name
			heightmapString = "Heightmap.image";
			heightmapFilename = terrainConfig.getSetting(heightmapString);
		}
		Image img;
		if (heightmapFilename.find(".raw") != String::npos)
		{
			int rawSize = StringConverter::parseInt(terrainConfig.getSetting("Heightmap.raw.size"));
			int bpp = StringConverter::parseInt(terrainConfig.getSetting("Heightmap.raw.bpp"));

			// load raw data
			DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(heightmapFilename);
			LOG(" loading RAW image: " + TOSTRING(stream->size()) + " / " + TOSTRING(rawSize*rawSize*bpp));
			PixelFormat pformat = PF_L8;
			if (bpp == 2)
				pformat = PF_L16;
			img.loadRawData(stream, rawSize, rawSize, 1, pformat);
		} else
		{
			img.load(heightmapFilename, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		}

		if (!terrainConfig.getSetting("Heightmap.flip").empty()  && StringConverter::parseBool(terrainConfig.getSetting("Heightmap.flip")))
			img.flipAroundX();


		//if (x % 2 != 0)
		//	img.flipAroundY();
		//if (y % 2 != 0)
		//	img.flipAroundX();

		mTerrainGroup->defineTerrain(x, y, &img);
		mTerrainsImported = true;
	}
}

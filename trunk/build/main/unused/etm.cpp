#include "etm.h"
#include "ResourceBuffer.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace Ogre;
using namespace ET;

EditableTerrain *EditableTerrain::myInstance = 0;

EditableTerrain::EditableTerrain()
{
	width=0;
	height=0;
	scaleX=0;
	scaleZ=0;
	maxHeight=0;
	heighmapname = "";
	worldtext = "";
}

EditableTerrain::~EditableTerrain()
{
}

EditableTerrain &EditableTerrain::getInstance()
{
	if(myInstance == 0) {
		myInstance = new EditableTerrain();
	}
	return *myInstance;
}

bool EditableTerrain::loadEditBrush(Ogre::String brushname)
{
	Image image;
	image.load(brushname, "ET");
	image.resize(16, 16);
	mEditBrush = ET::loadBrushFromImage(image);
	return true;
}

void EditableTerrain::setup(Ogre::SceneManager *mgr)
{
	mSceneMgr = mgr;
	mTerrainMgr = new ET::TerrainManager(mSceneMgr);
	loadEditBrush("brush.png");
}

bool EditableTerrain::deform(Ogre::Vector3 pos, float amount)
{
	mTerrainMgr->deform(terrainInfo->posToVertexX(pos.x), terrainInfo->posToVertexZ(pos.z), mEditBrush, amount);
	//updateLightmap();
	return true;
}

bool EditableTerrain::smooth(Ogre::Vector3 pos, SmoothSampleType type)
{
	int x = terrainInfo->posToVertexX(pos.x);
	int z = terrainInfo->posToVertexZ(pos.z);
	Brush smooth = boxFilterPatch(x, z, 16, 16, type, mEditBrush);
	mTerrainMgr->setHeights(x, z, smooth);
	return true;
}

bool EditableTerrain::paint(Ogre::Vector3 paintPos, int pattern, float pressure)
{
	float brushIntensity = 2;
	// retrieve edit points
	int x = terrainInfo->posToVertexX(paintPos.x);
	int z = terrainInfo->posToVertexZ(paintPos.z);
	LogManager::getSingleton().logMessage("painting to " + StringConverter::toString(x) + ", " + StringConverter::toString(z) + " with pattern no. " + StringConverter::toString(pattern));
	mSplatMgr->paint(pattern, x, z, mEditBrush, brushIntensity);
	return true;
}

void EditableTerrain::updateLightmap(Vector3 sunDirection, ColourValue lightColour, ColourValue ambientColour)
{
	static Vector3 lastDirection = Vector3::ZERO;
	static ColourValue lastLightColour, lastAmbientColour;

	if(sunDirection == Vector3::ZERO)
	{
		if(lastDirection == Vector3::ZERO)
			return;
		// use last values
		sunDirection = lastDirection;
		lightColour = lastLightColour;
		ambientColour = lastAmbientColour;
	}
	 else
	{
		// update values
		lastDirection = sunDirection ;
		lastLightColour = lightColour;
		lastAmbientColour = ambientColour;
	}
	Image lightmap;
	ET::createTerrainLightmap(*terrainInfo, lightmap, 128, 128, sunDirection, lightColour, ambientColour);

	// get our dynamic texture and update its contents
	TexturePtr tex = TextureManager::getSingleton().getByName("ETLightmap");
	tex->getBuffer(0, 0)->blitFromMemory(lightmap.getPixelBox(0, 0));
}

void EditableTerrain::loadTerrain(String geom, Camera *cam)
{
	terrainConfigFile = geom;
	mCamera = cam;
	// load configuration from STM
	ConfigFile cfg;
	String group="";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(String(geom));
	}catch(...)
	{
	}
	if(group == "")
		return;
	
	DataStreamPtr ds_config = ResourceGroupManager::getSingleton().openResource(String(geom), group);
	cfg.load(ds_config, "\t:=", false);

	// the image size
	String tmpSize = cfg.getSetting("Heightmap.raw.size");
	if (tmpSize != String(""))
		width = height = atof(tmpSize.c_str());

	// the image size
	tmpSize = cfg.getSetting("MaxHeight");
	if (tmpSize != String(""))
		maxHeight = atof(tmpSize.c_str());

	// X and Z scale
	tmpSize = cfg.getSetting("PageWorldX");
	if (tmpSize != String(""))
		scaleX = atof(tmpSize.c_str());

	tmpSize = cfg.getSetting("PageWorldZ");
	if (tmpSize != String(""))
		scaleZ = atof(tmpSize.c_str());

	heighmapname = cfg.getSetting("Heightmap.image");

	worldtext = cfg.getSetting("WorldTexture");

	mTerrainMgr->setLODErrorMargin(2, mCamera->getViewport()->getActualHeight());
	mTerrainMgr->setUseLODMorphing(true, 0.2, "morphFactor");

	// create a fresh, mid-high terrain for editing
	ET::TerrainInfo loadTerrainInfo(width, height, std::vector<float>(width*height, 0.0f));

	Image *image = new Image();
	//image->load("aspen.png","General");
	//ET::loadHeightmapFromImage(terrainInfo, *image);

	String group2="";
	try
	{
		group2 = ResourceGroupManager::getSingleton().findGroupContainingResource(heighmapname);
	}catch(...)
	{
	}
	if(group2 == "")
		return;
	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(heighmapname, group2);
	ET::loadHeightmapFromRawData(loadTerrainInfo, *ds.get(), width, height);

	// set position and size of the terrain
	loadTerrainInfo.setExtents(AxisAlignedBox(0, 0, 0, scaleX, maxHeight, scaleZ));
	
	mTerrainMgr->createTerrain(loadTerrainInfo);
	terrainInfo = const_cast<ET::TerrainInfo *>(&mTerrainMgr->getTerrainInfo());

	// create the splatting manager
	mSplatMgr = new ET::SplattingManager("ETSplatting", "ET", 128, 128);
	// specify number of splatting textures we need to handle
	mSplatMgr->setNumTextures(6);

	// load splatting
	image->load("ETterrain.png", "ET");
	// now load the splatting maps
	for (unsigned int i = 0; i < mSplatMgr->getNumMaps(); ++i)
	{
		String filename = "ETcoverage."+StringConverter::toString(i)+".png";
		if(ResourceGroupManager::getSingleton().resourceExists("ET", filename))
		{
			LogManager::getSingleton().logMessage("ET: loading: ETcoverage."+StringConverter::toString(i)+".png");
			image->load(filename, "ET");
			mSplatMgr->loadMapFromImage(i, *image);
		}
	}

	// load the terrain material and assign it
	MaterialPtr material (MaterialManager::getSingleton().getByName("ETTerrainMaterial"));

	//load the default texture:
	//MaterialPtr material = MaterialManager::getSingleton().create("myTerrainMat", "General", false);
	//material->createTechnique();
	//material->getTechnique(0)->createPass();
	
	material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(worldtext);
	
	mTerrainMgr->setMaterial(material);

	LogManager::getSingleton().logMessage("terrain texture: " + worldtext);

	// create a manual lightmap texture
	TexturePtr lightmapTex = TextureManager::getSingleton().createManual("ETLightmap", "ET", TEX_TYPE_2D, 128, 128, 1, PF_BYTE_RGB,Ogre::TU_DEFAULT, new ResourceBuffer());
	/*
	Image lightmap;
	ET::createTerrainLightmap(*terrainInfo, lightmap, 128, 128, Vector3(1, -1, 1), ColourValue::White,
	ColourValue(0.3, 0.3, 0.3));
	lightmapTex->getBuffer(0, 0)->blitFromMemory(lightmap.getPixelBox(0, 0));
	*/
}

bool EditableTerrain::saveTerrain()
{
	// try to find out the real filename
	String group="";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(heighmapname);
	}catch(...)
	{
	}
	if(group == "")
		return false;
	FileInfoListPtr list = ResourceGroupManager::getSingleton().findResourceFileInfo(group, heighmapname);
	if(list.getPointer() == 0 || list->size() == 0)
		return false;
	FileInfoList f = *list.get();
	String filename = f[0].archive->getName() + "/" + f[0].filename;
	// ugly ostream following
	try
	{
		std::ifstream is;
		std::filebuf *fb = is.rdbuf();
		if(!fb || !fb->open(filename.c_str(), ios::out | ios::binary))
			return false;
		if(!fb->is_open())
			return false;

		std::ostream os(fb);
		ET::saveHeightmapToRawData(*terrainInfo, os);
		fb->close();
		return true;
	} catch(...)
	{
		return false;
	}
	return false;
}

Ogre::Material *EditableTerrain::getTerrainMaterial()
{
	return mTerrainMgr->getMaterial().get();
}

ET::TerrainInfo *EditableTerrain::getTerrainInfo()
{
	return terrainInfo;
}

float EditableTerrain::buildFactor (Brush heights, int x, int y, float &factor)
{
	if(x >= 0 && x < (int)heights.getWidth() && y >= 0 && y < (int)heights.getHeight())
	{
		factor += 1.0;
		return heights.at(x, y);
	}

	return 0.0f;
}

Brush EditableTerrain::boxFilterPatch(int x, int z, int w, int h, enum SmoothSampleType type, Brush intensity)
{
	std::vector<float> retBuf;
	retBuf.resize(w*h);
	std::vector<float> heightsBuf;
	heightsBuf.resize(w*h);
	Brush ret = Brush(retBuf, w, h);
	Brush heights = Brush(heightsBuf, w, h);
	
	mTerrainMgr->getHeights(x, z, heights);
	for (unsigned int ix = 0; ix < heights.getWidth(); ix++ )
	{
		for(unsigned int iy = 0; iy < heights.getHeight(); iy++)
		{
			int px1 = (int)ix - 1, px2 = (int)ix + 1;
			int py1 = (int)iy - 1, py2 = (int)iy + 1;

			float height = heights.at(ix, iy);

			float final = 0.0f;
			float factor = 0.0f;                   

			// Sample grid
			// 1 4 7
			// 2 5 8
			// 3 6 9

			if(type == SST_Large) {
				final += buildFactor (heights, px1, py1, factor); // 1
				final += buildFactor (heights, px1, py2, factor); // 3
				final += buildFactor (heights, px2, py1, factor); // 7
				final += buildFactor (heights, px2, py2, factor); // 9
			}

			final += buildFactor (heights, (int)ix, (int)iy, factor); // 5

			final += buildFactor (heights, px1, (int)iy, factor); // 2
			final += buildFactor (heights, (int)ix, py1, factor); // 4
			final += buildFactor (heights, (int)ix, py2, factor); // 6                   
			final += buildFactor (heights, px2, (int)iy, factor); // 8

			final /= factor;
			float delta = height - final;
			float intens = intensity.at(ix, iy);
			if(intens > 0.0f) {
				delta *= intens;
			}

			final = height - delta;
			ret.at(ix, iy) = final;
		}
	}
   
	return ret;
}


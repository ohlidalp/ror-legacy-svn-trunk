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
#include "TerrainObjectManager.h"
#include "TerrainGeometryManager.h"
#include "TerrainHeightFinder.h"

#include "ScriptEngine.h"

#include "ShadowManager.h"
#include "dashboard.h"

#include "hdrlistener.h"

#include "utils.h"

using namespace Ogre;

TerrainManager::TerrainManager(Ogre::SceneManager *smgr, Ogre::RenderWindow *window, Ogre::Camera *camera) :
	  mSceneMgr(smgr)
	, mWindow(window)
	, mCamera(camera)
	, waterLine(-9999)
{

}

TerrainManager::~TerrainManager()
{

}

void TerrainManager::loadTerrain( Ogre::String filename )
{
	char line[1024] = "";
	DataStreamPtr ds;
	try
	{
		ds = ResourceGroupManager::getSingleton().openResource(filename, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
	} catch(...)
	{
		LOG("Terrain not found: " + String(filename));
		showError(_L("Terrain loading error"), _L("Terrain not found: ") + filename);
		exit(125);
	}

	// now generate the hash of it
	fileHash = generateHashFromDataStream(ds);

	terrainConfig.load(ds, "\t:=", false);

	// read in the settings
	terrainName = terrainConfig.getSetting("Name");
	ogreTerrainConfigFilename = terrainConfig.getSetting("GeometryConfig");
	// otc = ogre terrain config
	if (ogreTerrainConfigFilename.find(".otc") == String::npos)
	{
		showError(_L("Terrain loading error"), _L("the new terrain mode only supports .otc configurations"));
		exit(125);
	}

	if(!terrainConfig.getSetting("WaterLine").empty())
		waterLine = StringConverter::parseReal(terrainConfig.getSetting("WaterLine"));

	ambientColor = StringConverter::parseColourValue(terrainConfig.getSetting("AmbientColor"));
	startPosition = StringConverter::parseVector3(terrainConfig.getSetting("StartPosition"));

	// then, init the subsystems, order is important :)
	initSubSystems();

	fixCompositorClearColor();


	// fix the person starting position
	/*
	if (persostart.isZeroLength() && !spl.pos.isZeroLength())
	{
		if (hfinder)
			persostart = Vector3(spl.pos.x, hfinder->getHeightAt(spl.pos.x, spl.pos.z), spl.pos.z);
		else
			persostart = spl.pos;
	}
	*/

	loadTerrainObjects();

	collisions->printStats();

	loading_state=TERRAIN_LOADED;



	if (debugCollisions)
		collisions->createCollisionDebugVisualization();

	// bake the decals
	//finishTerrainDecal();

	collisions->finishLoadingTerrain();
}

void TerrainManager::initSubSystems()
{
	// objects  - .odef support
	objectManager   = new TerrainObjectManager(mSceneMgr, this);
	
	// geometry - ogre terrain things
	geometryManager = new TerrainGeometryManager(mSceneMgr, this);

	// collision integration
	initHeightFinder();
	
	// shadows
	initShadows();

	// sky
	initSkySubSystem();

	initCamera();

	initLight();

	initFog();

	initVegetation();

	if (BSETTING("HDR", false))
		initHDR();

	if (BSETTING("Glow", false))
		initGlow();

	if (BSETTING("Motion blur", false))
		initMotionBlur();
	

	if (BSETTING("Sunburn", false))
		initSunburn();

	if(waterLine != -9999)
		initWater();

	//environment map
	if(!BSETTING("Envmapdisable", false))
		initEnvironmentMap();

	initDashboards();
}

void TerrainManager::initCamera()
{
	mCamera->getViewport()->setBackgroundColour(ambientColor);

	// still required?
	farclip = ISETTING("SightRange", 2000);
	bool inifite_farclip = false;
	if (farclip == 5000 && mRoot->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_INFINITE_FAR_PLANE))
	{
		mCamera->setFarClipDistance(0);   // enable infinite far clip distance if we can
		inifite_farclip = true;
	} else
	{
		farclip = std::min((float)farclip, terrainzsize * 1.8f);
		mCamera->setFarClipDistance(farclip);
	}

	mCamera->setPosition(startPosition);
}

void TerrainManager::initSkySubSystem()
{
#ifdef USE_CAELUM
	//Caelum skies
	useCaelum = SSETTING("Sky effects", "Caelum (best looking, slower)")=="Caelum (best looking, slower)";
	if (useCaelum)
	{
		skyManager = new SkyManager(mSceneMgr, mWindow, mCamera);

		// try to load caelum config
		String caelumConfig = terrainConfig.getSetting("CaelumConfigFile");
		if(!caelumConfig.empty() && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(caelumConfig))
		{
			// config provided and existing, use it :)
			skyManager->loadScript(caelumConfig);
		} else
		{
			// no config provided, fall back to the default one
			skyManager->loadScript("ror_default_sky");
		}
#endif //USE_CAELUM
	} else
#endif //CAELUM
	{
		String sandStormConfig = terrainConfig.getSetting("SandStormCubeMap");
		if (!sandStormConfig.empty())
		{
			// use custom
			mSceneMgr->setSkyBox(true, sandStormConfig, 100, true);
		} else
		{
			// use default
			mSceneMgr->setSkyBox(true, "tracks/skyboxcol", 100, true);
		}
	}
}

void TerrainManager::initLight()
{
	if(useCaelum)
	{
		mainLight = skyManager->getMainLight();
	} else
	{
		// screw caelum, we will roll our own light

		// Create a light
		mainLight = mSceneMgr->createLight("MainLight");
		//directional light for shadow
		mainLight->setType(Light::LT_DIRECTIONAL);
		mainLight->setDirection(0.785, -0.423, 0.453);

		mainLight->setDiffuseColour(ambientColor);
		mainLight->setSpecularColour(ambientColor);
	}
}

void TerrainManager::initFog()
{
	mSceneMgr->setFog(FOG_LINEAR, ambientColor,  0, farclip * 0.7, farclip * 0.9);
}

void TerrainManager::initVegetation()
{
	// get vegetation mode
	int pagedMode = 0; //None
	float pagedDetailFactor = 0;
	String vegetationMode = SSETTING("Vegetation", "None (fastest)");
	if     (vegetationMode == "None (fastest)")
	{
		pagedMode = 0;
		pagedDetailFactor = 0.001;
	}
	else if (vegetationMode == "20%")
	{
		pagedMode = 1;
		pagedDetailFactor = 0.2;
	}
	else if (vegetationMode == "50%")
	{
		pagedMode = 2;
		pagedDetailFactor = 0.5;
	}
	else if (vegetationMode == "Full (best looking, slower)")
	{
		pagedMode = 3;
		pagedDetailFactor = 1;
	}
}

void TerrainManager::initHDR()
{
	Viewport *vp = mCamera->getViewport();
	Ogre::CompositorInstance *instance = Ogre::CompositorManager::getSingleton().addCompositor(vp, "HDR", 0);
	Ogre::CompositorManager::getSingleton().setCompositorEnabled(vp, "HDR", true);

	// HDR needs a special listener
	hdrListener = new HDRListener();
	instance->addListener(hdrListener);
	hdrListener->notifyViewportSize(vp->getActualWidth(), vp->getActualHeight());
	hdrListener->notifyCompositor(instance);
}

void TerrainManager::initGlow()
{
	CompositorManager::getSingleton().addCompositor(mCamera->getViewport(), "Glow");
	CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Glow", true);
	GlowMaterialListener *gml = new GlowMaterialListener();
	Ogre::MaterialManager::getSingleton().addListener(gml);
}

void TerrainManager::initMotionBlur()
{
	/// Motion blur effect
	CompositorPtr comp3 = CompositorManager::getSingleton().create(
		"MotionBlur", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
		);
	{
		CompositionTechnique *t = comp3->createTechnique();
		{
			CompositionTechnique::TextureDefinition *def = t->createTextureDefinition("scene");
			def->width = 0;
			def->height = 0;
#if OGRE_VERSION>0x010602
			def->formatList.push_back(PF_R8G8B8);
#else
			def->format = PF_R8G8B8;
#endif //OGRE_VERSION
		}
		{
			CompositionTechnique::TextureDefinition *def = t->createTextureDefinition("sum");
			def->width = 0;
			def->height = 0;
#if OGRE_VERSION>0x010602
			def->formatList.push_back(PF_R8G8B8);
#else
			def->format = PF_R8G8B8;
#endif //OGRE_VERSION
		}
		{
			CompositionTechnique::TextureDefinition *def = t->createTextureDefinition("temp");
			def->width = 0;
			def->height = 0;
#if OGRE_VERSION>0x010602
			def->formatList.push_back(PF_R8G8B8);
#else
			def->format = PF_R8G8B8;
#endif //OGRE_VERSION
		}
		/// Render scene
		{
			CompositionTargetPass *tp = t->createTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_PREVIOUS);
			tp->setOutputName("scene");
		}
		/// Initialisation pass for sum texture
		{
			CompositionTargetPass *tp = t->createTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_PREVIOUS);
			tp->setOutputName("sum");
			tp->setOnlyInitial(true);
		}
		/// Do the motion blur
		{
			CompositionTargetPass *tp = t->createTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_NONE);
			tp->setOutputName("temp");
			{ CompositionPass *pass = tp->createPass();
			pass->setType(CompositionPass::PT_RENDERQUAD);
			pass->setMaterialName("Ogre/Compositor/Combine");
			pass->setInput(0, "scene");
			pass->setInput(1, "sum");
			}
		}
		/// Copy back sum texture
		{
			CompositionTargetPass *tp = t->createTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_NONE);
			tp->setOutputName("sum");
			{ CompositionPass *pass = tp->createPass();
			pass->setType(CompositionPass::PT_RENDERQUAD);
			pass->setMaterialName("Ogre/Compositor/Copyback");
			pass->setInput(0, "temp");
			}
		}
		/// Display result
		{
			CompositionTargetPass *tp = t->getOutputTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_NONE);
			{ CompositionPass *pass = tp->createPass();
			pass->setType(CompositionPass::PT_RENDERQUAD);
			pass->setMaterialName("Ogre/Compositor/MotionBlur");
			pass->setInput(0, "sum");
			}
		}
	}
	CompositorManager::getSingleton().addCompositor(mCamera->getViewport(),"MotionBlur");
	CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "MotionBlur", true);
}

void TerrainManager::initSunburn()
{
	CompositorManager::getSingleton().addCompositor(mCamera->getViewport(),"Sunburn");
	CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Sunburn", true);
}

void TerrainManager::fixCompositorClearColor()
{
	//hack
	// now with extensive error checking
	if (CompositorManager::getSingleton().hasCompositorChain(mCamera->getViewport()))
	{
		//	//CompositorManager::getSingleton().getCompositorChain(mCamera->getViewport())->getCompositor(0)->getTechnique()->getOutputTargetPass()->getPass(0)->setClearColour(fadeColour);
		CompositorInstance *co = CompositorManager::getSingleton().getCompositorChain(mCamera->getViewport())->_getOriginalSceneCompositor();
		if (co)
		{
			CompositionTechnique *ct = co->getTechnique();
			if (ct)
			{
				CompositionTargetPass *ctp = ct->getOutputTargetPass();
				if (ctp)
				{
					CompositionPass *p = ctp->getPass(0);
					if (p)
						p->setClearColour(fadeColour);
				}
			}
		}
	}
}

void TerrainManager::initWater()
{
	//water!
	if (waterline != -9999)
	{
		bool usewaves=(BSETTING("Waves", false));

		// disable waves in multiplayer
		if (net)
			usewaves=false;

		String waterSettingsString = SSETTING("Water effects", "Reflection + refraction (speed optimized)");

		if      (waterSettingsString == "None")
			w = 0;
		else if (waterSettingsString == "Basic (fastest)")
			w = new WaterOld(WaterOld::WATER_BASIC, mCamera, mSceneMgr, mWindow, waterline, &mapsizex, &mapsizez, usewaves);
		else if (waterSettingsString == "Reflection")
			w = new WaterOld(WaterOld::WATER_REFLECT, mCamera, mSceneMgr, mWindow, waterline, &mapsizex, &mapsizez, usewaves);
		else if (waterSettingsString == "Reflection + refraction (speed optimized)")
			w = new WaterOld(WaterOld::WATER_FULL_SPEED, mCamera, mSceneMgr, mWindow, waterline, &mapsizex, &mapsizez, usewaves);
		else if (waterSettingsString == "Reflection + refraction (quality optimized)")
			w = new WaterOld(WaterOld::WATER_FULL_QUALITY, mCamera, mSceneMgr, mWindow, waterline, &mapsizex, &mapsizez, usewaves);
	}
	if (w) w->setFadeColour(fadeColour);
	if (person) person->setWater(w);
	BeamFactory::getSingleton().w = w;
	DustManager::getSingleton().setWater(w);
}

void TerrainManager::initEnvironmentMap()
{
	envmap = new Envmap(mSceneMgr, mWindow, mCamera, BSETTING("Envmap", false), ISETTING("EnvmapUpdateRate", 1));
}

void TerrainManager::initDashboards()
{
	dashboard = new Dashboard(mSceneMgr);
}

void TerrainManager::initHeightFinder()
{
	heightFinder = new TerrainHeightFinder(geometryManager);
	collisions->setHfinder(heightFinder);

	if (person)
		person->setHFinder(heightFinder);

	// update hfinder instance in factory
	BeamFactory::getSingleton().mfinder = heightFinder;
}

void TerrainManager::initShadows()
{
	shadowManager   = new ShadowManager(mSceneMgr, mWindow, mCamera);
	shadowManager->loadConfiguration();
}

void TerrainManager::loadTerrainObjects()
{
	Ogre::ConfigFile::SettingsIterator objectsIterator = terrainConfig.getSettingsIterator("Objects");
	Ogre::String svalue, sname;
	while (objectsIterator.hasMoreElements())
	{
		sname = objectsIterator.peekNextKey();
		Ogre::StringUtil::trim(sname);
		svalue = objectsIterator.getNext();
		Ogre::StringUtil::trim(svalue);

		objectManager->loadObjectConfigFile(sname);
	}
}

void TerrainManager::initCollisions()
{
	collisions = new Collisions(this, mSceneMgr, debugCollisions);

	// load AS
#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().setCollisions(collisions);

	// update icollisions instance in factory
	BeamFactory::getSingleton().icollisions = collisions;

	if (person) person->setCollisions(collisions);
#ifdef USE_MYGUI
	if (GUI_Friction::getSingletonPtr())
		GUI_Friction::getSingleton().setCollisions(collisions);
#endif //MYGUI

	// advanced camera collision tools
	mCollisionTools = new MOC::CollisionTools(mSceneMgr);
	// set how far we want the camera to be above ground
	mCollisionTools->setHeightAdjust(0.2f);
}

void TerrainManager::initScripting()
{
#ifdef USE_ANGELSCRIPT
	if (!netmode)
	{
		Ogre::ConfigFile::SettingsIterator objectsIterator = terrainConfig.getSettingsIterator("scripts");
		Ogre::String svalue, sname;
		int loaded = 0;
		while (objectsIterator.hasMoreElements())
		{
			sname = objectsIterator.peekNextKey();
			Ogre::StringUtil::trim(sname);
			svalue = objectsIterator.getNext();
			Ogre::StringUtil::trim(svalue);

			ScriptEngine::getSingleton().loadScript(sname);
			loaded++;
		}

		if(loaded == 0)
		{
			// load a default script that does the most basic things
			ScriptEngine::getSingleton().loadScript("default.as");
		}
	} else
	{
		// load the default stscriptuff so spawners will work in multiplayer
		ScriptEngine::getSingleton().loadScript("default.as");
	}

	// finally activate AS logging, so we dont spam the users screen with initialization messages
	ScriptEngine::getSingleton().activateLogging();
#endif
}

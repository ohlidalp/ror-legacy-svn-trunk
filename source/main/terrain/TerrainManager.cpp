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

#include "BeamData.h"
#include "BeamFactory.h"
#include "Character.h"
#include "Character.h"
#include "DustManager.h"
#include "GlowMaterialListener.h"
#include "ScriptEngine.h"
#include "Settings.h"
#include "ShadowManager.h"
#include "SkyManager.h"
#include "SoundScriptManager.h"
#include "TerrainGeometryManager.h"
#include "TerrainObjectManager.h"
#include "Dashboard.h"
#include "EnvironmentMap.h"
#include "ErrorUtils.h"
#include "GUIFriction.h"
#include "HDRListener.h"
#include "Language.h"
#include "Utils.h"
#include "Water.h"

using namespace Ogre;

TerrainManager::TerrainManager() :
	loading_state(NONE_LOADED)
{

}

TerrainManager::~TerrainManager()
{

}

void TerrainManager::loadTerrain(String filename)
{
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

	mTerrainConfig.load(ds, "\t:=", false);

	// read in the settings
	terrain_name = mTerrainConfig.getSetting("Name");

	ogre_terrain_config_filename = mTerrainConfig.getSetting("GeometryConfig");
	// otc = ogre terrain config
	if (ogre_terrain_config_filename.find(".otc") == String::npos)
	{
		showError(_L("Terrain loading error"), _L("the new terrain mode only supports .otc configurations"));
		exit(125);
	}

	ambient_color = StringConverter::parseColourValue(mTerrainConfig.getSetting("AmbientColor"));
	start_position = StringConverter::parseVector3(mTerrainConfig.getSetting("StartPosition"));

	// then, init the subsystems, order is important :)
	initSubSystems();

	fixCompositorClearColor();

	loadTerrainObjects();

	collisions->printStats();

	loading_state = TERRAIN_LOADED;



	//if (debugCollisions)
		//collisions->createCollisionDebugVisualization();

	// bake the decals
	//finishTerrainDecal();

	collisions->finishLoadingTerrain();
}

void TerrainManager::initSubSystems()
{
	// objects  - .odef support
	object_manager   = new TerrainObjectManager(this);
	
	// geometry - ogre terrain things
	geometry_manager = new TerrainGeometryManager(this);
	
	// shadows
	initShadows();

	// sky
	initSkySubSystem();

	initCamera();

	initLight();

	initFog();

	initVegetation();

	initWater();

	if (BSETTING("HDR", false))
	{
		initHDR();
	}
	if (BSETTING("Glow", false))
	{
		initGlow();
	}
	if (BSETTING("Motion blur", false))
	{
		initMotionBlur();
	}
	if (BSETTING("Sunburn", false))
	{
		initSunburn();
	}
	// environment map
	if (!BSETTING("Envmapdisable", false))
	{
		initEnvironmentMap();
	}
	// init the map
	if (!BSETTING("disableOverViewMap", false))
	{
		initSurveyMap();
	}
	initDashboards();
}

void TerrainManager::initCamera()
{
	globalEnvironment->ogreCamera->getViewport()->setBackgroundColour(ambient_color);

	//globalEnvironment->ogreCamera->setFarClipDistance(0);

	globalEnvironment->ogreCamera->setPosition(start_position);
}

void TerrainManager::initSkySubSystem()
{
#ifdef USE_CAELUM
	// Caelum skies
	bool useCaelum = SSETTING("Sky effects", "Caelum (best looking, slower)")=="Caelum (best looking, slower)";

	if (useCaelum)
	{
		sky_manager = new SkyManager();
		globalEnvironment->sky = sky_manager;

		// try to load caelum config
		String caelumConfig = mTerrainConfig.getSetting("CaelumConfigFile");

		if (!caelumConfig.empty() && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(caelumConfig))
		{
			// config provided and existing, use it :)
			sky_manager->loadScript(caelumConfig);
		} else
		{
			// no config provided, fall back to the default one
			sky_manager->loadScript("ror_default_sky");
		}

	} else
#endif //USE_CAELUM
	{
		String sandStormConfig = mTerrainConfig.getSetting("SandStormCubeMap");

		if (!sandStormConfig.empty())
		{
			// use custom
			globalEnvironment->ogreSceneManager->setSkyBox(true, sandStormConfig, 100, true);
		} else
		{
			// use default
			globalEnvironment->ogreSceneManager->setSkyBox(true, "tracks/skyboxcol", 100, true);
		}
	}
}

void TerrainManager::initLight()
{
	if (use_caelum)
	{
		main_light = sky_manager->getMainLight();
	} else
	{
		// screw caelum, we will roll our own light

		// Create a light
		main_light = globalEnvironment->ogreSceneManager->createLight("MainLight");
		//directional light for shadow
		main_light->setType(Light::LT_DIRECTIONAL);
		main_light->setDirection(0.785, -0.423, 0.453);

		main_light->setDiffuseColour(ambient_color);
		main_light->setSpecularColour(ambient_color);
	}
}

void TerrainManager::initFog()
{
	globalEnvironment->ogreSceneManager->setFog(FOG_LINEAR, ambient_color,  0, farclip * 0.7, farclip * 0.9);
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
	Viewport *vp = globalEnvironment->ogreCamera->getViewport();
	CompositorInstance *instance = CompositorManager::getSingleton().addCompositor(vp, "HDR", 0);
	CompositorManager::getSingleton().setCompositorEnabled(vp, "HDR", true);

	// HDR needs a special listener
	hdr_listener = new HDRListener();
	instance->addListener(hdr_listener);
	hdr_listener->notifyViewportSize(vp->getActualWidth(), vp->getActualHeight());
	hdr_listener->notifyCompositor(instance);
}

void TerrainManager::initGlow()
{
	CompositorManager::getSingleton().addCompositor(globalEnvironment->ogreCamera->getViewport(), "Glow");
	CompositorManager::getSingleton().setCompositorEnabled(globalEnvironment->ogreCamera->getViewport(), "Glow", true);
	GlowMaterialListener *gml = new GlowMaterialListener();
	MaterialManager::getSingleton().addListener(gml);
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
	CompositorManager::getSingleton().addCompositor(globalEnvironment->ogreCamera->getViewport(),"MotionBlur");
	CompositorManager::getSingleton().setCompositorEnabled(globalEnvironment->ogreCamera->getViewport(), "MotionBlur", true);
}

void TerrainManager::initSunburn()
{
	CompositorManager::getSingleton().addCompositor(globalEnvironment->ogreCamera->getViewport(),"Sunburn");
	CompositorManager::getSingleton().setCompositorEnabled(globalEnvironment->ogreCamera->getViewport(), "Sunburn", true);
}

void TerrainManager::fixCompositorClearColor()
{
	//hack
	// now with extensive error checking
	if (CompositorManager::getSingleton().hasCompositorChain(globalEnvironment->ogreCamera->getViewport()))
	{
		//	//CompositorManager::getSingleton().getCompositorChain(globalEnvironment->ogreCamera->getViewport())->getCompositor(0)->getTechnique()->getOutputTargetPass()->getPass(0)->setClearColour(fade_color);
		CompositorInstance *co = CompositorManager::getSingleton().getCompositorChain(globalEnvironment->ogreCamera->getViewport())->_getOriginalSceneCompositor();
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
					{
						p->setClearColour(fade_color);
					}
				}
			}
		}
	}
}

void TerrainManager::initWater()
{
	water = new Water(mTerrainConfig);
}

void TerrainManager::initEnvironmentMap()
{
	envmap = new Envmap();
}

void TerrainManager::initDashboards()
{
	dashboard = new Dashboard();
}

void TerrainManager::initShadows()
{
	shadow_manager   = new ShadowManager();
	shadow_manager->loadConfiguration();
}

void TerrainManager::loadTerrainObjects()
{
	ConfigFile::SettingsIterator objectsIterator = mTerrainConfig.getSettingsIterator("Objects");
	String svalue, sname;
	while (objectsIterator.hasMoreElements())
	{
		sname = objectsIterator.peekNextKey();
		StringUtil::trim(sname);
		svalue = objectsIterator.getNext();
		StringUtil::trim(svalue);

		object_manager->loadObjectConfigFile(sname);
	}
}

void TerrainManager::initCollisions()
{
	collisions = new Collisions();
	globalEnvironment->collisions = collisions;
}

void TerrainManager::initScripting()
{
#ifdef USE_ANGELSCRIPT
	if (!BSETTING("netmode", false))
	{
		ConfigFile::SettingsIterator objectsIterator = mTerrainConfig.getSettingsIterator("scripts");
		bool loaded = false;
		while (objectsIterator.hasMoreElements())
		{
			String sname = objectsIterator.peekNextKey();
			StringUtil::trim(sname);
			String svalue = objectsIterator.getNext();
			StringUtil::trim(svalue);

			ScriptEngine::getSingleton().loadScript(sname);
			loaded = true;
		}
		if (loaded)
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

void TerrainManager::setGravity(float value)
{
	gravity = value;
	BeamFactory::getSingleton().recalcGravityMasses();
}

void TerrainManager::initSurveyMap()
{
	//survey_map = new MapControl(mapsizex, mapsizey, mapsizez);
}

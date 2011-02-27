/*
This source file is part of Rigs of Rods
Copyright 2005-2011 Pierre-Michel Ricordel
Copyright 2007-2011 Thomas Fischer

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

#include "ContentManager.h"

#include <Ogre.h>

#include "Settings.h"
#include "ColoredTextAreaOverlayElementFactory.h"
#include "SoundScriptManager.h"
#include "skinmanager.h"
#include "language.h"

#include "CacheSystem.h"

#include "utils.h"

using namespace Ogre;
using namespace std;

template<> ContentManager* Ogre::Singleton<ContentManager>::ms_Singleton = 0;

ContentManager::ContentManager()
{
}

ContentManager::~ContentManager()
{
}

void ContentManager::loadMainResource(String name, String group)
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif

	String zipFilename = SETTINGS.getSetting("Resources Path")+name+".zip";
	if(fileExists(zipFilename))
	{
		ResourceGroupManager::getSingleton().addResourceLocation(zipFilename, "Zip", group, true);
	} else
	{
		String dirname = SETTINGS.getSetting("Resources Path")+dirsep+name;
		ResourceGroupManager::getSingleton().addResourceLocation(dirname, "FileSystem", group, true);
	}
}

bool ContentManager::init(void)
{

    //Set listener if none has already been set
    if (!Ogre::ResourceGroupManager::getSingleton().getLoadingListener())
        Ogre::ResourceGroupManager::getSingleton().setLoadingListener(this);

	//try to get correct paths
	//note: we don't have LogManager available yet!
	//FIRST: Get the "program path" and the user space path

	// note: this is now done in the settings class, so set it up
	// note: you need to set the buildmode correcty before you build the paths!

	// by default, display everything in the depth map
	Ogre::MovableObject::setDefaultVisibilityFlags(DEPTHMAP_ENABLED);

	CACHE.setLocation(SETTINGS.getSetting("Cache Path"), SETTINGS.getSetting("Config Root"));

	ColoredTextAreaOverlayElementFactory *cef = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(cef);



	//load bootstrap and main resources
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	//bootstrap
	LogManager::getSingleton().logMessage("Loading Bootstrap");
	loadMainResource("OgreCore", "Bootstrap");
	//main game resources
	LogManager::getSingleton().logMessage("Loading main resources");
	loadMainResource("airfoils");
	loadMainResource("materials");
	loadMainResource("meshes");
	loadMainResource("overlays");
	loadMainResource("particles");
	loadMainResource("mygui", "cache"); // HACK for mygui only supporting one resource group
	loadMainResource("scripts");
	loadMainResource("textures");
	loadMainResource("flags");
	loadMainResource("icons");

	// optional ones

	// sound iss a bit special as we mark the base sounds so we dont clear them accidentially later on
#ifdef USE_OPENAL
	LogManager::getSingleton().logMessage("Creating Sound Manager");
	SoundScriptManager *ssm = SoundScriptManager::getSingleton();
	if(ssm) ssm->setLoadingBaseSounds(true);
#endif // USE_OPENAL
	if (SETTINGS.getSetting("3D Sound renderer") != "No sound")
		loadMainResource("sounds");

	if (SETTINGS.getSetting("Sky effects") == "Caelum (best looking, slower)")
		loadMainResource("caelum");

	if(SETTINGS.getSetting("Hydrax") == "Yes")
		loadMainResource("hydrax", "Hydrax"); // special resourcegroup required!

	if(SETTINGS.getSetting("Vegetation") != "None (fastest)")
		loadMainResource("paged");

	if(SETTINGS.getSetting("HDR") == "Yes")
		loadMainResource("hdr");

	if(SETTINGS.getSetting("DOF") == "Yes")
		loadMainResource("dof");

	if(SETTINGS.getSetting("Glow") == "Yes")
		loadMainResource("glow");

	if(SETTINGS.getSetting("Motion blur") == "Yes")
		loadMainResource("blur");

	if(SETTINGS.getSetting("HeatHaze") == "Yes")
		loadMainResource("heathaze");

	if (SETTINGS.getSetting("Sunburn")!="Yes")
		loadMainResource("sunburn");

	if (SETTINGS.getSetting("Shadow technique")=="Parallel-split Shadow Maps")
		loadMainResource("pssm");

	//streams path, to be processed later by the cache system
	LogManager::getSingleton().logMessage("Loading filesystems");

	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"cache", "FileSystem", "cache");
	//config, flat
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"config", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"alwaysload", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	//packs, to be processed later by the cache system

	// add scripts folder
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"scripts", "FileSystem", "Scripts", true);

	// init skin manager, important to happen before trucks resource loading!
	LogManager::getSingleton().logMessage("registering Skin Manager");
	new SkinManager();

	LogManager::getSingleton().logMessage("registering colored text overlay factory");
	ColoredTextAreaOverlayElementFactory *pCT = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(pCT);

	// Set default mipmap level (NB some APIs ignore this)
	TextureManager::getSingleton().setDefaultNumMipmaps(5);
	String tft=SETTINGS.getSetting("Texture Filtering");
	TextureFilterOptions tfo=TFO_NONE;
	if (tft=="Bilinear") tfo=TFO_BILINEAR;
	if (tft=="Trilinear") tfo=TFO_TRILINEAR;
	if (tft=="Anisotropic (best looking)") tfo=TFO_ANISOTROPIC;
	MaterialManager::getSingleton().setDefaultAnisotropy(8);
	MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);

	// load all resources now, so the zip files are also initiated
	LogManager::getSingleton().logMessage("initialiseAllResourceGroups()");
	try
	{
		ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	} catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("catched error while initializing Resource groups: " + e.getFullDescription());
	}
#ifdef USE_OPENAL
	if(ssm) ssm->setLoadingBaseSounds(false);
#endif // USE_OPENAL


	// and the content
	//main synced streams
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Streams Path"),      "FileSystem", "Streams", true);
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"packs", "FileSystem", "Packs", true);
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"mods",  "FileSystem", "Packs", true);

	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"vehicles", "FileSystem", "VehicleFolders", true);
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"terrains", "FileSystem", "TerrainFolders", true);

	LogManager::getSingleton().logMessage("initialiseAllResourceGroups() - Content");
	try
	{
		ResourceGroupManager::getSingleton().initialiseResourceGroup("Streams");
		ResourceGroupManager::getSingleton().initialiseResourceGroup("Packs");
		ResourceGroupManager::getSingleton().initialiseResourceGroup("VehicleFolders");
		ResourceGroupManager::getSingleton().initialiseResourceGroup("TerrainFolders");
	} catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("catched error while initializing Content Resource groups: " + e.getFullDescription());
	}

#ifndef NOLANG
	// load language, must happen after initializing Settings class and Ogre Root!
	// also it must happen after loading all basic resources!
	LanguageEngine::Instance().setup();
#endif //NOLANG
	return true;
}

Ogre::DataStreamPtr ContentManager::resourceLoading(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource)
{
	return Ogre::DataStreamPtr();
}

void ContentManager::resourceStreamOpened(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource, Ogre::DataStreamPtr& dataStream)
{
}

bool ContentManager::resourceCollision(Ogre::Resource *resource, Ogre::ResourceManager *resourceManager)
{
	return false;
}

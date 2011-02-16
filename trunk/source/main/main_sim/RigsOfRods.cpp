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
#include "RigsOfRods.h"
#include "OgreLineStreamOverlayElement.h"
#include "language.h"
#include "errorutils.h"
#include "SoundScriptManager.h"

#include <OgreHeaderPrefix.h>

/** This class simply demonstrates basic usage of the CRTShader system.
It sub class the material manager listener class and when a target scheme callback
is invoked with the shader generator scheme it tries to create an equvialent shader
based technique based on the default technique of the given material.
*/
class ShaderGeneratorTechniqueResolverListener : public MaterialManager::Listener
{
public:

	ShaderGeneratorTechniqueResolverListener(RTShader::ShaderGenerator* pShaderGenerator)
	{
		mShaderGenerator = pShaderGenerator;
	}

	virtual Technique* handleSchemeNotFound(unsigned short schemeIndex,
		const String& schemeName, Material* originalMaterial, unsigned short lodIndex,
		const Renderable* rend)
	{
		// Case this is the default shader generator scheme.
		if (schemeName == RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME)
		{
			MaterialRegisterIterator itFind = mRegisteredMaterials.find(originalMaterial);
			bool techniqueCreated = false;

			// This material was not registered before.
			if (itFind == mRegisteredMaterials.end())
			{
				techniqueCreated = mShaderGenerator->createShaderBasedTechnique(
					originalMaterial->getName(),
					MaterialManager::DEFAULT_SCHEME_NAME,
					schemeName);
			}
			mRegisteredMaterials[originalMaterial] = techniqueCreated;
		}

		return NULL;
	}

protected:
	typedef std::map<Material*, bool>		MaterialRegisterMap;
	typedef MaterialRegisterMap::iterator	MaterialRegisterIterator;

protected:
	MaterialRegisterMap				mRegisteredMaterials;		// Registered material map.
	RTShader::ShaderGenerator*		mShaderGenerator;			// The shader generator instance.
};

RigsOfRods::RigsOfRods(Ogre::String name, unsigned int hwnd, unsigned int mainhwnd) :
	mRoot(0),
	mCamera(0),
	mSceneMgr(0),
	mFrameListener(0),
	mWindow(0),
	ssm(0),
	hwnd(hwnd),
	mainhwnd(mainhwnd),
	name(name)
{
	useogreconfig=false;
}

RigsOfRods::~RigsOfRods()
{
	// this is the end of all and everything
	if (mFrameListener)
		delete mFrameListener;
	if (mRoot)
	{
		if (mWindow)
			mRoot->detachRenderTarget(mWindow);
		try
		{
			mRoot->shutdown();
			// XXX : commented out code below crashes
			//
			//if(mRoot)
			//	delete mRoot;
		} catch(Ogre::Exception& e)
		{
			LogManager::getSingleton().logMessage("error while closing RoR: " + e.getFullDescription());
		}
	}
}

void RigsOfRods::go(void)
{
	if (!setup())
		return;

	mRoot->startRendering();

	// clean up
	//destroyScene(); we don't!
}

void RigsOfRods::loadMainResource(String name, String group)
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif

	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Resources Path")+dirsep+name, "FileSystem", group, true);
	try
	{
		ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Resources Path")+name+".zip", "Zip", group, true);
	} catch(...) {}
}

bool RigsOfRods::setup(void)
{
	//try to get correct paths
	//note: we don't have LogManager available yet!
	//FIRST: Get the "program path" and the user space path

	// note: this is now done in the settings class, so set it up
	// note: you need to set the buildmode correcty before you build the paths!
	SETTINGS.setSetting("BuildMode", buildmode?"Yes":"No");
	if(!SETTINGS.setupPaths())
		return false;

	// load RoR.cfg directly after setting up paths
	SETTINGS.loadSettings(SETTINGS.getSetting("Config Root")+"RoR.cfg");

	//CREATE OGRE ROOT
	String logFilename   = SETTINGS.getSetting("Log Path") + name + Ogre::String(".log");
	String pluginsConfig = SETTINGS.getSetting("plugins.cfg");
	String ogreConfig    = SETTINGS.getSetting("ogre.cfg");
	mRoot = new Root(pluginsConfig, ogreConfig, logFilename);

	// log verbosity change
	if(SETTINGS.getSetting("Logging Level") == "verbose")
		LogManager::getSingleton().setLogDetail(LL_BOREME);
	else if(SETTINGS.getSetting("Logging Level") == "normal")
		LogManager::getSingleton().setLogDetail(LL_NORMAL);
	else if(SETTINGS.getSetting("Logging Level") == "low")
		LogManager::getSingleton().setLogDetail(LL_LOW);
	//FROM NOW ON WE HAVE LOGMANAGER!

	// by default, display everything in the depth map
	Ogre::MovableObject::setDefaultVisibilityFlags(DEPTHMAP_ENABLED);


	CACHE.setLocation(SETTINGS.getSetting("Cache Path"), SETTINGS.getSetting("Config Root"));

	ColoredTextAreaOverlayElementFactory *cef = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(cef);

	// load factory to be able to create stream lines
	OverlayManager& overlayManager = OverlayManager::getSingleton();
	overlayManager.addOverlayElementFactory(new LineStreamOverlayElementFactory());

#ifdef HAS_EDITOR
	spinfact=new SpinControlOverlayElementFactory();
	Ogre::OverlayManager::getSingleton().addOverlayElementFactory(spinfact);
#endif

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

	LogManager::getSingleton().logMessage("Configuring");
	bool carryOn = configure();
	if (!carryOn) return false;

	//CREATE SCENE MANAGER
	LogManager::getSingleton().logMessage("Creating Scene Manager");
	if(SETTINGS.getSetting("new Terrain Mode") == "Yes")
	{
		mSceneMgr = mRoot->createSceneManager(ST_GENERIC);
	} else
	{
		mSceneMgr = mRoot->createSceneManager(ST_EXTERIOR_CLOSE);
	}

	//CREATE CAMERA
	LogManager::getSingleton().logMessage("Creating camera");
	// Create the camera
	mCamera = mSceneMgr->createCamera("PlayerCam");
	// Position it at 500 in Z direction
	mCamera->setPosition(Vector3(128,25,128));
	// Look back along -Z
	mCamera->lookAt(Vector3(0,0,-300));
	mCamera->setNearClipDistance( 0.5 );
	mCamera->setFarClipDistance( 1000.0*1.733 );
	mCamera->setFOVy(Degree(60));
	mCamera->setAutoAspectRatio(true);

	//CREATE VIEWPORT
	LogManager::getSingleton().logMessage("Creating Viewport");
	// Create one viewport, entire window
	vp = mWindow->addViewport(mCamera);
	vp->setBackgroundColour(ColourValue(0,0,0));

	// create RTShader System (after creating the viewport)
	if (SETTINGS.getSetting("Use RTShader System")=="Yes")
		initRTShaderSystem();

	// Set default mipmap level (NB some APIs ignore this)
	TextureManager::getSingleton().setDefaultNumMipmaps(5);
	//mRoot->setFrameSmoothingPeriod(2.0);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#ifndef _UNICODE
	if(hwnd == 0)
	{
		// only in non-embedded mode
		size_t hWnd = 0;
		mWindow->getCustomAttribute("WINDOW", &hWnd);

		char buf[MAX_PATH];
		::GetModuleFileNameA(0, (LPCH)&buf, MAX_PATH);

		HINSTANCE instance = ::GetModuleHandleA(buf);
		HICON hIcon = ::LoadIconA(instance, MAKEINTRESOURCE(1001));
		if (hIcon)
		{
			::SendMessageA((HWND)hWnd, WM_SETICON, 1, (LPARAM)hIcon);
			::SendMessageA((HWND)hWnd, WM_SETICON, 0, (LPARAM)hIcon);
		}
	}
#endif //_UNICODE
#endif //OGRE_PLATFORM_WIN32

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
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Streams Path"), "FileSystem", "Streams");
	exploreStreams(); //this will explore subdirs and register them as Packs dirs
	//cache, flat
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"packs", "FileSystem", "Packs", true);
	//user vehicles, to be processed later by the cache system
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"vehicles", "FileSystem", "VehicleFolders");
	exploreVehicles(); //this will add subdirs contents into General
	//user terrains, to be processed later by the cache system
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"terrains", "FileSystem", "TerrainFolders");
	exploreTerrains(); //this will add subdirs contents into General
	LogManager::getSingleton().logMessage("initialiseAllResourceGroups() - Content");
	try
	{
		ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	} catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("catched error while initializing Content Resource groups: " + e.getFullDescription());
	}

	//rgm.initialiseResourceGroup(ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

#ifndef NOLANG
	// load language, must happen after initializing Settings class and Ogre Root!
	// also it must happen after loading all basic resources!
	LanguageEngine::Instance().setup();
#endif //NOLANG

	// Create the scene

	LogManager::getSingleton().logMessage("createScene()");
	createScene();

	LogManager::getSingleton().logMessage("Adding Frame Listener");
	bool isEmbedded = (hwnd != 0);
	mFrameListener = new RoRFrameListener(mWindow, mCamera, mSceneMgr, mRoot, isEmbedded, mainhwnd);
	mRoot->addFrameListener(mFrameListener);

	LogManager::getSingleton().logMessage("Setup finished successfully");
	return true;
}

void RigsOfRods::initRTShaderSystem()
{
	if(!Ogre::RTShader::ShaderGenerator::initialize())
		return;

	// Add the shader libs resource location.
	loadMainResource("rtshader");

	// Grab the shader generator pointer.
	mShaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();

	// Set shader cache path.
	mShaderGenerator->setShaderCachePath(SETTINGS.getSetting("Cache Path"));

	// Set the scene manager.
	mShaderGenerator->addSceneManager(mSceneMgr);

	//ShaderGeneratorTechniqueResolverListener *mMaterialMgrListener = new ShaderGeneratorTechniqueResolverListener(mShaderGenerator);
	//MaterialManager::getSingleton().addListener(mMaterialMgrListener);

	// Apply the shader generated based techniques.
	vp->setMaterialScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
}

void RigsOfRods::createScene(void)
{
	String tft=SETTINGS.getSetting("Texture Filtering");
	TextureFilterOptions tfo=TFO_NONE;
	if (tft=="Bilinear") tfo=TFO_BILINEAR;
	if (tft=="Trilinear") tfo=TFO_TRILINEAR;
	if (tft=="Anisotropic (best looking)") tfo=TFO_ANISOTROPIC;

	// Set ambient light
	//        mSceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));
	mSceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));
	//lighten shadows
	//		mSceneMgr->setShadowColour(ColourValue(0.5, 0.5, 0.5));

	// Accept default settings: point light, white diffuse, just set position
	// NB I could attach the light to a SceneNode if I wanted it to move automatically with
	//  other objects, but I don't
	//        l->setPosition(-20000,80000,-50000);

	MaterialManager::getSingleton().setDefaultAnisotropy(8);
	MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);
}

bool RigsOfRods::configure(void)
{
	// Show the configuration dialog and initialise the system
	// You can skip this and use root.restoreConfig() to load configuration
	// settings if you were sure there are valid ones saved in ogre.cfg
	if(hwnd == 0)
	{
		//default mode
		bool ok = false;
		if(useogreconfig)
			ok = mRoot->showConfigDialog();
		else
			ok = mRoot->restoreConfig();
		if(ok)
		{
			// If returned true, user clicked OK so initialise
			// Here we choose to let the system create a default rendering window by passing 'true'
			mWindow = mRoot->initialise(true);
			return true;
		}
		else
		{
			showError(_L("Configuration error"), _L("Run the RoRconfig program first."));
			return false;
		}
	} else
	{
		// embedded mode
		if(!mRoot->restoreConfig())
		{
			showError(_L("Configuration error"), _L("Run the RoRconfig program first."));
			return false;
		}

		mRoot->initialise(false);

		Ogre::NameValuePairList param;
		param["externalWindowHandle"] = Ogre::StringConverter::toString(hwnd);
		mWindow = mRoot->createRenderWindow(name, 320, 240, false, &param);
		return true;
	}
	return false;
}

void RigsOfRods::exploreStreams()
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	//explore stream folders, add their content as packs
	FileInfoListPtr files= rgm.findResourceFileInfo("Streams", "*", true); //searching for dirs
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if (iterFiles->filename==String(".svn")) continue;
		String filename = SETTINGS.getSetting("Streams Path") + (*iterFiles).filename;
		rgm.addResourceLocation(filename, "FileSystem", "Packs");

		// HACK: add subfolders
		rgm.addResourceLocation(filename+dirsep+"vehicles", "FileSystem", "streams");
		rgm.addResourceLocation(filename+dirsep+"terrains", "FileSystem", "streams");
	}
	LogManager::getSingleton().logMessage("initialiseResourceGroups: streams");
	try
	{
		ResourceGroupManager::getSingleton().initialiseResourceGroup("streams");
	} catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("catched error while initializing streams Resource groups: " + e.getFullDescription());
	}
}

void RigsOfRods::exploreVehicles()
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	//explore vehicles folders, add their content as flat
	FileInfoListPtr files= rgm.findResourceFileInfo("VehicleFolders", "*", true); //searching for dirs
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if ((*iterFiles).filename==String(".svn")) continue;
		//trying to get the full path
		String fullpath=(*iterFiles).archive->getName()+dirsep;
		rgm.addResourceLocation(fullpath+(*iterFiles).filename, "FileSystem", "VehicleFolders");
		//add textures/temp for unpacked ddx support
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"textures"+dirsep+"temp", "FileSystem", "VehicleFolders");
	}
	LogManager::getSingleton().logMessage("initialiseResourceGroups: VehicleFolders");
	try
	{
		ResourceGroupManager::getSingleton().initialiseResourceGroup("VehicleFolders");
	} catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("catched error while initializing VehicleFolders Resource groups: " + e.getFullDescription());
	}
}

void RigsOfRods::exploreTerrains()
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	//explore vehicles folders, add their content as flat
	FileInfoListPtr files= rgm.findResourceFileInfo("TerrainFolders", "*", true); //searching for dirs
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if ((*iterFiles).filename==String(".svn")) continue;
		//trying to get the full path
		String fullpath=(*iterFiles).archive->getName()+dirsep;
		rgm.addResourceLocation(fullpath+(*iterFiles).filename, "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"textures"+dirsep+"temp", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"objects", "FileSystem", "TerrainFolders");
		// ogitor support below
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Caelum", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Hydrax", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Materials", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Models", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Scripts", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"SkyX", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Temp", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Templates", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Terrain", "FileSystem", "TerrainFolders");
	}
	LogManager::getSingleton().logMessage("initialiseResourceGroups: TerrainFolders");
	try
	{
		ResourceGroupManager::getSingleton().initialiseResourceGroup("TerrainFolders");
	} catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("catched error while initializing TerrainFolders Resource groups: " + e.getFullDescription());
	}
}

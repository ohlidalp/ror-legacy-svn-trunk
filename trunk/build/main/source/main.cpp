/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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
#include "main.h"
#include "OgreLineStreamOverlayElement.h"
#include "language.h"

RigsOfRods::RigsOfRods()
{
	mFrameListener = 0;
	mRoot = 0;
	mWindow=0;
	useogreconfig=false;
}

RigsOfRods::~RigsOfRods()
{
	if (mFrameListener)
		delete mFrameListener;
	if (mRoot)
	{
		if (mWindow) mRoot->detachRenderTarget(mWindow);
		try {
			mRoot->shutdown();
			if(mRoot)
				delete mRoot;
		} catch(...){}
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

bool RigsOfRods::fileExists(char* filename)
{
	FILE* f = fopen(filename, "rb");
	if(f != NULL) {
		fclose(f);
		return true;
	}
	return false;
}

void RigsOfRods::loadMainResource(String name, String group)
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	if (buildmode)
	{
		if (name==String("textures"))
			ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Resources Path")+name+dirsep+"temp", "FileSystem", group);
		else
			ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Resources Path")+name, "FileSystem", group);
	}
	else
		ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Resources Path")+name+".zip", "Zip", group);
}

bool RigsOfRods::setup(void)
{
	//try to get correct paths
	//note: we don't have LogManager available yet!
	//FIRST: Get the "program path" and the user space path

	// note: this is now done in the settigns class, so set it up
	// note: you need to set the buildmode correcty before you build the paths!
	SETTINGS.setSetting("BuildMode", buildmode?"Yes":"No");
	if(!SETTINGS.setupPaths())
		return false;

	// load RoR.cfg directly after setting up paths
	SETTINGS.loadSettings(SETTINGS.getSetting("Config Root")+"RoR.cfg");

	//CREATE OGRE ROOT
	mRoot = new Root(SETTINGS.getSetting("plugins.cfg"), SETTINGS.getSetting("ogre.cfg"), SETTINGS.getSetting("ogre.log"));

	//FROM NOW ON WE HAVE LOGMANAGER!

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
	loadMainResource("airfoils", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("materials", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("meshes", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("overlays", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("paged", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("particles", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("mygui", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("scripts", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("sounds", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("textures", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("hydrax", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	//streams path, to be processed later by the cache system
	LogManager::getSingleton().logMessage("Loading filesystems");

	//main synced streams
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Streams Path"), "FileSystem", "Streams");
	exploreStreams(); //this will explore subdirs and register them as Packs dirs
	//cache, flat
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"cache", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	//config, flat
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"config", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	//packs, to be processed later by the cache system
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"packs", "FileSystem", "Packs", true);
	//user vehicles, to be processed later by the cache system
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"vehicles", "FileSystem", "VehicleFolders");
	exploreVehicles(); //this will add subdirs contents into General
	//user terrains, to be processed later by the cache system
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"terrains", "FileSystem", "TerrainFolders");
	exploreTerrains(); //this will add subdirs contents into General

	// init skin manager, important to happen before trucks resource loading!
	LogManager::getSingleton().logMessage("registering Skin Manager");
	new SkinManager();

	LogManager::getSingleton().logMessage("registering colored text overlay factory");
	ColoredTextAreaOverlayElementFactory *pCT = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(pCT);

	LogManager::getSingleton().logMessage("Configuring");
	bool carryOn = configure();
	if (!carryOn) return false;

	LogManager::getSingleton().logMessage("Creating Sound Manager");
	//get instance of SoundScriptManager
	//this will create the singleton and register the parser!
	ssm=SoundScriptManager::getSingleton();

	//CREATE SCENE MANAGER
	LogManager::getSingleton().logMessage("Creating Scene Manager");
	mSceneMgr = mRoot->createSceneManager(ST_EXTERIOR_CLOSE);

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

	//CREATE VIEWPORT
	LogManager::getSingleton().logMessage("Creating Viewport");
	// Create one viewport, entire window
	Viewport* vp = mWindow->addViewport(mCamera);
	vp->setBackgroundColour(ColourValue(0,0,0));

	// Alter the camera aspect ratio to match the viewport
	mCamera->setAspectRatio(
		Real(vp->getActualWidth()) / Real(vp->getActualHeight()));

	// Set default mipmap level (NB some APIs ignore this)
	TextureManager::getSingleton().setDefaultNumMipmaps(5);
	//mRoot->setFrameSmoothingPeriod(2.0);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
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
#endif

	// load all resources now, so the zip files are also initiated
	LogManager::getSingleton().logMessage("initialiseAllResourceGroups()");
	try
	{
		ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	} catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("catched error while initializing Resource groups: " + e.getFullDescription());
	}

	//rgm.initialiseResourceGroup(ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	// load language, must happen after initializing Settings class and Ogre Root!
	// also it must happen after loading all basic resources!
	LanguageEngine::Instance().setup();

	// Create the scene
	LogManager::getSingleton().logMessage("createScene()");
	createScene();

	LogManager::getSingleton().logMessage("Adding Frame Listener");
	mFrameListener = new RoRFrameListener(mWindow, mCamera, mSceneMgr, mRoot);
	mRoot->addFrameListener(mFrameListener);

	LogManager::getSingleton().logMessage("Setup finished successfully");
	return true;
}

void RigsOfRods::createScene(void)
{
	String tft=SETTINGS.getSetting("Texture Filtering");
	TextureFilterOptions tfo=TFO_NONE;
	if (tft=="Bilinear") tfo=TFO_BILINEAR;
	if (tft=="Trilinear") tfo=TFO_TRILINEAR;
	if (tft=="Anisotropic (best looking)") tfo=TFO_ANISOTROPIC;


	// Test which syntax are allowed for shaders
	LogManager::getSingleton().logMessage("* supported shader profiles:");
	const GpuProgramManager::SyntaxCodes &syntaxCodes = GpuProgramManager::getSingleton().getSupportedSyntax();
	for (GpuProgramManager::SyntaxCodes::const_iterator iter = syntaxCodes.begin();iter != syntaxCodes.end();++iter)
		LogManager::getSingleton().logMessage(" - "+(*iter));

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
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
MessageBox( NULL, "Run the RoRconfig program first.", "Configuration error", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
printf("\n\nConfiguration error: Run the RoRconfig program first.\n\n");
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
printf("\n\nConfiguration error: Run the RoRconfig program first.\n\n");
//CFOptionFlags flgs;
//CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, "Configuration error", "Run the RoRconfig program first.", NULL, NULL, NULL, &flgs);
#endif
		return false;
	}
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
		rgm.addResourceLocation(filename+dirsep+"vehicles", "FileSystem", "VehicleFolders");
		rgm.addResourceLocation(filename+dirsep+"terrains", "FileSystem", "TerrainFolders");
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
		//add textures/temp for unpacked ddx support
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"textures"+dirsep+"temp", "FileSystem", "TerrainFolders");
		//add objects
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"objects", "FileSystem", "TerrainFolders");
	}
}


//BELOW IS THE C BOOTSTRAP===================================================================

#ifdef USE_REPORT
#include "crashrpt.h"
#endif

// simpleopt by http://code.jellycan.com/simpleopt/
// license: MIT
#include "SimpleOpt.h"

#define HELPTEXT "--help (this)\n-map <map> (loads map on startup)\n-truck <truck> (loads truck on startup)\n-setup shows the ogre configurator\n-version shows the version information\n-enter enters the selected truck\n\nFor example: RoR.exe -map oahu -truck semi"

// option identifiers
enum { OPT_HELP, OPT_MAP, OPT_TRUCK, OPT_SETUP, OPT_CMD, OPT_WDIR, OPT_ETM, OPT_BUILD, OPT_CONFIG, OPT_VER, OPT_CHECKCACHE, OPT_TRUCKCONFIG, OPT_ENTERTRUCK};

// option array
CSimpleOpt::SOption cmdline_options[] = {
	{ OPT_MAP,   ("-map"),    SO_REQ_SEP },
	{ OPT_MAP,   ("-terrain"),    SO_REQ_SEP },
	{ OPT_TRUCK, ("-truck"),  SO_REQ_SEP },
	{ OPT_ENTERTRUCK, ("-enter"),  SO_NONE },
	{ OPT_CMD,   ("-cmd"),   SO_REQ_SEP },
	{ OPT_WDIR,  ("-wd"),     SO_REQ_SEP },
	{ OPT_SETUP, ("-setup"),  SO_NONE    },
	{ OPT_CONFIG,("-config"), SO_NONE    },
	{ OPT_TRUCKCONFIG,("-truckconfig"), SO_REQ_SEP    },
	{ OPT_BUILD, ("-build"),  SO_NONE    },
	{ OPT_HELP,  ("--help"),  SO_NONE    },
	{ OPT_CHECKCACHE,  ("-checkcache"),  SO_NONE    },
	{ OPT_VER,   ("-version"),SO_NONE    },
	SO_END_OF_OPTIONS
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "ShellAPI.h"

#ifdef BUGTRAP
#include "BugTrap.h"
#pragma comment(lib, "BugTrapN.lib")  // Link to Unicode DLL

static void setupExceptionHandler()
{
    BT_SetAppName("Rigs of Rods 0.36 Beta");
	BT_SetSupportEMail("thomas@rigsofrods.com");
    BT_SetFlags(BTF_DETAILEDMODE | BTF_EDITMAIL);
    //BT_SetSupportServer("localhost", 9999);
    BT_SetSupportURL("http://forum.rigsofrods.com");
	
	// catch c++ exceptions:
	BT_SetTerminate();
}

#endif //BUGTRAP

#endif //OGRE_PLATFORM_WIN32

#ifdef __cplusplus
extern "C" {
#endif

void showUsage()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	MessageBox( NULL, HELPTEXT, "Command Line Arguments", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
#else
	std::cerr << HELPTEXT << std::endl;
#endif
}

void showVersion()
{
	char tmp[1024] = "";
	sprintf(tmp, "Rigs of Rods version %s\n%s\n%s\n", ROR_VERSION_STRING, SVN_REVISION, SVN_ID);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	MessageBox( NULL, tmp, "Version Information", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
#else
	std::cerr << tmp << std::endl;
#endif
}

int main(int argc, char *argv[])
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	//start working dir is highly unpredictable in MacOSX (typically you start in "/"!)
	//oh, thats quite hacked - thomas
	char str[256];
	strcpy(str, argv[0]);
	char *pt=str+strlen(str);
	while (*pt!='/') pt--;
	*pt=0;
	chdir(str);
	chdir("../../..");
	getwd(str);
	printf("GETWD=%s\n", str);
#endif

#ifdef BUGTRAP
	setupExceptionHandler();
#endif //BUGTRAP

	// Create application object
	RigsOfRods app;
	app.buildmode=false;

//MacOSX adds an extra argument in the for of -psn_0_XXXXXX when the app is double clicked
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE
	CSimpleOpt args(argc, argv, cmdline_options);
	while (args.Next()) {
		if (args.LastError() == SO_SUCCESS) {
			if (args.OptionId() == OPT_HELP) {
				showUsage();
				return 0;
			} else if (args.OptionId() == OPT_TRUCK) {
				SETTINGS.setSetting("Preselected Truck", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_TRUCKCONFIG) {
				SETTINGS.setSetting("Preselected TruckConfig", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_MAP) {
				SETTINGS.setSetting("Preselected Map", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_CMD) {
				SETTINGS.setSetting("cmdline CMD", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_CONFIG) {
				SETTINGS.setSetting("configfile", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_WDIR) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				SetCurrentDirectory(args.OptionArg());
#endif
			} else if (args.OptionId() == OPT_CHECKCACHE) {
				// just regen cache and exit
				SETTINGS.setSetting("regen-cache-only", "True");
			} else if (args.OptionId() == OPT_ENTERTRUCK) {
				SETTINGS.setSetting("Enter Preselected Truck", "Yes");
			} else if (args.OptionId() == OPT_SETUP) {
				app.useogreconfig = true;
			} else if (args.OptionId() == OPT_BUILD) {
				app.buildmode = true;
			} else if (args.OptionId() == OPT_VER) {
				showVersion();
				return 0;
			}
		} else {
			showUsage();
			return 1;
		}
	}
#endif

	try {
		app.go();
	} catch(Ogre::Exception& e)
	{
		// try to shutdown input system upon an error
		if(InputEngine::instanceExists()) // this prevents the creating of it, if not existing
			INPUTENGINE.prepareShutdown();

		String url = "http://wiki.rigsofrods.com/index.php?title=Error_" + StringConverter::toString(e.getNumber())+"#"+e.getSource();
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		String err = e.getFullDescription() + "\n\n" + "You can eventually get help here:\n\n" + url + "\n\nDo you want to open that address in your default browser now?";
		int Response = MessageBox( NULL, err.c_str(), "An exception has occured!", MB_YESNO | MB_ICONERROR | MB_TOPMOST);
		// 6 = yes, 7 = no
		if(Response == 6)
		{
			// Microsoft conversion hell follows :|
			char tmp[255], tmp2[255];
			wchar_t ws1[255], ws2[255];
			strncpy(tmp, "open", 255);
			mbstowcs(ws1, tmp, 255);
			strncpy(tmp2, url.c_str(), 255);
			mbstowcs(ws2, tmp2, 255);
			ShellExecuteW(NULL, ws1, ws2, NULL, NULL, SW_SHOWNORMAL);
		}
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		String err = "\n####################\n" + e.getFullDescription() + "\n\n" + "You can eventually get help here:\n\n" + url + "\n\n(copy that into the address field of your browser)\n####################\n";
		std::cerr << "An exception has occured: " << err.c_str() << std::endl;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	String err = e.getFullDescription();
	std::cerr << "An exception has occured: " << err.c_str() << std::endl;
	//CFOptionFlags flgs;
	//CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, "An exception has occured!", err.c_str(), NULL, NULL, NULL, &flgs);
#endif
		return 1;
	}

	return 0;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
	return main(__argc, __argv);
}
#endif


#ifdef __cplusplus
}
#endif

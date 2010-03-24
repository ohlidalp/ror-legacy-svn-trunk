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
#include "errorutils.h"

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
	loadMainResource("layouts", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("scripts", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("sounds", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("textures", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	loadMainResource("hydrax", "Hydrax"); // special resourcegroup required!
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
		showError(_L("Configuration error"), _L("Run the RoRconfig program first."));
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
Ogre::String getVersionString()
{
	char tmp[1024] = "";
	sprintf(tmp, "Rigs of Rods\n"
		" version: %s\n"
		" revision: %s\n"
		" full revision: %s\n"
		" protocol version: %s\n"
		" build time: %s, %s\n"
		, ROR_VERSION_STRING, SVN_REVISION, SVN_ID, RORNET_VERSION, __DATE__, __TIME__);
	return Ogre::String(tmp);
}

#ifdef USE_WINDOWS_CRASH_REPORT
// see http://code.google.com/p/crashrpt/
#include "crashrpt.h"

// Define the crash callback
BOOL WINAPI crashCallback(LPVOID /*lpvState*/)
{
	// Now add these two files to the error report
	
	// logs
	crAddFile((SETTINGS.getSetting("Log Path") + "RoR.log").c_str(), "Rigs of Rods Log");
	crAddFile((SETTINGS.getSetting("Log Path") + "mygui.log").c_str(), "Rigs of Rods GUI Log");
	crAddFile((SETTINGS.getSetting("Log Path") + "configlog.txt").c_str(), "Rigs of Rods Configurator Log");
	crAddFile((SETTINGS.getSetting("Program Path") + "wizard.log").c_str(), "Rigs of Rods Installer Log");

	// cache
	crAddFile((SETTINGS.getSetting("Cache Path") + "mods.cache").c_str(), "Rigs of Rods Cache File");

	// configs
	crAddFile((SETTINGS.getSetting("Config Root") + "ground_models.cfg").c_str(), "Rigs of Rods Ground Configuration");
	crAddFile((SETTINGS.getSetting("Config Root") + "input.map").c_str(), "Rigs of Rods Input Configuration");
	crAddFile((SETTINGS.getSetting("Config Root") + "ogre.cfg").c_str(), "Rigs of Rods Renderer Configuration");
	crAddFile((SETTINGS.getSetting("Config Root") + "RoR.cfg").c_str(), "Rigs of Rods Configuration");

	crAddProperty("Version", ROR_VERSION_STRING);
	crAddProperty("Revision", SVN_REVISION);
	crAddProperty("full_revision", SVN_ID);
	crAddProperty("protocol_version", RORNET_VERSION);
	crAddProperty("build_date", __DATE__);
	crAddProperty("build_time", __TIME__);

	crAddProperty("System_GUID", SETTINGS.getSetting("GUID").c_str());
	crAddProperty("Multiplayer", (SETTINGS.getSetting("Network enable")=="Yes")?"1":"0");
	
	crAddScreenshot(CR_AS_MAIN_WINDOW);
	// Return TRUE to allow crash report generation
	return TRUE;
}

void install_crashrpt()
{
	// Install CrashRpt support
	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);  
	info.pszAppName = "Rigs of Rods";
	info.pszAppVersion = ROR_VERSION_STRING;
	info.pszEmailSubject = "Error Report for Rigs of Rods";
	info.pszEmailTo = "thomas@rigsofrods.com";

	char tmp[512]="";
	sprintf(tmp, "http://api.rigsofrods.com/crashreport/?version=%s_%s", __DATE__, __TIME__);
	for(unsigned int i=0;i<strnlen(tmp, 512);i++)
	{
		if(tmp[i] == ' ')
			tmp[i] = '_';
	}

	info.pszUrl = tmp;
	info.pfnCrashCallback = crashCallback; 
	info.uPriorities[CR_HTTP]  = 3;  // Try HTTP the first
	info.uPriorities[CR_SMTP]  = 2;  // Try SMTP the second
	info.uPriorities[CR_SMAPI] = 1; // Try Simple MAPI the last  
	info.dwFlags = 0; // Install all available exception handlers
	info.pszPrivacyPolicyURL = "http://wiki.rigsofrods.com/pages/Crash_Report_Privacy_Policy"; // URL for the Privacy Policy link

	int nInstResult = crInstall(&info);
	if(nInstResult!=0)
	{
		// Something goes wrong!
		TCHAR szErrorMsg[512];
		szErrorMsg[0]=0;

		crGetLastErrorMsg(szErrorMsg, 512);
		printf("%s\n", szErrorMsg);


		showError("Exception handling registration problem", String(szErrorMsg));

		assert(nInstResult==0);
	}
}

void uninstall_crashrpt()
{
	// Unset crash handlers
	int nUninstResult = crUninstall();
	assert(nUninstResult==0);
}

void test_crashrpt()
{
	// emulate null pointer exception (access violation)
	crEmulateCrash(CR_WIN32_STRUCTURED_EXCEPTION);
}
#endif

// simpleopt by http://code.jellycan.com/simpleopt/
// license: MIT
#include "SimpleOpt.h"

#define HELPTEXT "--help (this)\n-map <map> (loads map on startup)\n-truck <truck> (loads truck on startup)\n-setup shows the ogre configurator\n-version shows the version information\n-enter enters the selected truck\n-userpath <path> sets the user directory\nFor example: RoR.exe -map oahu -truck semi"

// option identifiers
enum { OPT_HELP, OPT_MAP, OPT_TRUCK, OPT_SETUP, OPT_CMD, OPT_WDIR, OPT_ETM, OPT_BUILD, OPT_CONFIG, OPT_VER, OPT_CHECKCACHE, OPT_TRUCKCONFIG, OPT_ENTERTRUCK, OPT_BENCH, OPT_STREAMCACHEGEN, OPT_BENCHNUM, OPT_USERPATH, OPT_BENCHPOS, OPT_BENCHPOSERR, OPT_NOCRASHCRPT};

// option array
CSimpleOpt::SOption cmdline_options[] = {
	{ OPT_MAP,            ((char *)"-map"),         SO_REQ_SEP },
	{ OPT_MAP,            ((char *)"-terrain"),     SO_REQ_SEP },
	{ OPT_TRUCK,          ((char *)"-truck"),       SO_REQ_SEP },
	{ OPT_ENTERTRUCK,     ((char *)"-enter"),       SO_NONE },
	{ OPT_CMD,            ((char *)"-cmd"),         SO_REQ_SEP },
	{ OPT_WDIR,           ((char *)"-wd"),          SO_REQ_SEP },
	{ OPT_SETUP,          ((char *)"-setup"),       SO_NONE    },
	{ OPT_CONFIG,         ((char *)"-config"),      SO_NONE    },
	{ OPT_TRUCKCONFIG,    ((char *)"-truckconfig"), SO_REQ_SEP    },
	{ OPT_BUILD,          ((char *)"-build"),       SO_NONE    },
	{ OPT_HELP,           ((char *)"--help"),       SO_NONE    },
	{ OPT_HELP,           ((char *)"-help"),        SO_NONE    },
	{ OPT_CHECKCACHE,     ((char *)"-checkcache"),  SO_NONE    },
	{ OPT_VER,            ((char *)"-version"),     SO_NONE    },
	{ OPT_USERPATH,       ((char *)"-userpath"),   SO_REQ_SEP    },
	{ OPT_BENCH,          ((char *)"-benchmark"),   SO_REQ_SEP    },
	{ OPT_BENCHPOS,       ((char *)"-benchmark-final-position"),   SO_REQ_SEP    },
	{ OPT_BENCHPOSERR,    ((char *)"-benchmark-final-position-error"),   SO_REQ_SEP    },
	{ OPT_BENCHNUM,       ((char *)"-benchmarktrucks"),       SO_REQ_SEP },
	{ OPT_BENCHNUM,       ((char *)"-benchmark-trucks"),       SO_REQ_SEP },
	{ OPT_STREAMCACHEGEN, ((char *)"-streamcachegen"),   SO_NONE    },
	{ OPT_NOCRASHCRPT,    ((char *)"-nocrashrpt"),   SO_NONE    },
SO_END_OF_OPTIONS
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX // required to stop windows.h messing up std::min
#endif //NOMINMAX
#include "windows.h"
#include "ShellAPI.h"
#endif //OGRE_PLATFORM_WIN32

#ifdef __cplusplus
extern "C" {
#endif

void showUsage()
{
	showInfo(_L("Command Line Arguments"), HELPTEXT);
}

void showVersion()
{
	showInfo(_L("Version Information"), getVersionString());
#ifdef __GNUC__
	printf(" * built with gcc %d.%d.%d\n", __GNUC_MINOR__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif //__GNUC__
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
			} else if (args.OptionId() == OPT_BENCH) {
				SETTINGS.setSetting("Benchmark", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCHNUM) {
				SETTINGS.setSetting("BenchmarkTrucks", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCHPOS) {
				SETTINGS.setSetting("BenchmarkFinalPosition", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCHPOSERR) {
				SETTINGS.setSetting("BenchmarkFinalPositionError", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_NOCRASHCRPT) {
				SETTINGS.setSetting("NoCrashRpt", "Yes");
			} else if (args.OptionId() == OPT_USERPATH) {
				SETTINGS.setSetting("userpath", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_CONFIG) {
				SETTINGS.setSetting("configfile", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_WDIR) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				SetCurrentDirectory(args.OptionArg());
#endif
			} else if (args.OptionId() == OPT_STREAMCACHEGEN) {
				SETTINGS.setSetting("streamCacheGenerationOnly", "Yes");
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

#ifdef USE_WINDOWS_CRASH_REPORT
	if(SETTINGS.getSetting("NoCrashRpt").empty())
		install_crashrpt();

	//test_crashrpt();
#endif //USE_WINDOWS_CRASH_REPORT

	try {
		app.go();
	} catch(Ogre::Exception& e)
	{
		// try to shutdown input system upon an error
		if(InputEngine::instanceExists()) // this prevents the creating of it, if not existing
			INPUTENGINE.prepareShutdown();

		String url = "http://wiki.rigsofrods.com/index.php?title=Error_" + StringConverter::toString(e.getNumber())+"#"+e.getSource();
		showWebError("An exception has occured!", e.getFullDescription(), url);
		return 1;
	}

#ifdef USE_WINDOWS_CRASH_REPORT
	if(SETTINGS.getSetting("NoCrashRpt").empty())
		uninstall_crashrpt();
#endif //USE_WINDOWS_CRASH_REPORT

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
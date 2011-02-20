//|||||||||||||||||||||||||||||||||||||||||||||||

#include "AdvancedOgreFramework.h"

#include "Settings.h"
#include "errorutils.h"
#include "language.h"

//|||||||||||||||||||||||||||||||||||||||||||||||

using namespace Ogre;

//|||||||||||||||||||||||||||||||||||||||||||||||

template<> OgreFramework* Ogre::Singleton<OgreFramework>::ms_Singleton = 0;

//|||||||||||||||||||||||||||||||||||||||||||||||

OgreFramework::OgreFramework() : hwnd(0), mainhwnd(0), name()
{
    m_pRoot				= 0;
    m_pRenderWnd		= 0;
    m_pViewport			= 0;
    m_pTimer			= 0;
}

//|||||||||||||||||||||||||||||||||||||||||||||||

OgreFramework::~OgreFramework()
{
	LogManager::getSingleton().logMessage("Shutdown OGRE...");
    //if(m_pTrayMgr)      delete m_pTrayMgr;
    //if(m_pRoot)			delete m_pRoot;
}

//|||||||||||||||||||||||||||||||||||||||||||||||

bool OgreFramework::configure(void)
{
	// Show the configuration dialog and initialise the system
	// You can skip this and use root.restoreConfig() to load configuration
	// settings if you were sure there are valid ones saved in ogre.cfg
	bool useogreconfig = SETTINGS.getBooleanSetting("USE_OGRE_CONFIG");
	if(hwnd == 0)
	{
		//default mode
		bool ok = false;
		if(useogreconfig)
			ok = m_pRoot->showConfigDialog();
		else
			ok = m_pRoot->restoreConfig();
		if(ok)
		{
			// If returned true, user clicked OK so initialise
			// Here we choose to let the system create a default rendering window by passing 'true'
			m_pRenderWnd = m_pRoot->initialise(true);
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
		if(!m_pRoot->restoreConfig())
		{
			showError(_L("Configuration error"), _L("Run the RoRconfig program first."));
			return false;
		}

		m_pRoot->initialise(false);

		Ogre::NameValuePairList param;
		param["externalWindowHandle"] = Ogre::StringConverter::toString(hwnd);
		m_pRenderWnd = m_pRoot->createRenderWindow(name, 320, 240, false, &param);
		return true;
	}
	return false;
}

bool OgreFramework::initOgre(Ogre::String name, unsigned int hwnd, unsigned int mainhwnd)
{
	this->name     = name;
	this->hwnd     = hwnd;
	this->mainhwnd = mainhwnd;

	if(!SETTINGS.setupPaths())
		return false;

	// load RoR.cfg directly after setting up paths
	SETTINGS.loadSettings(SETTINGS.getSetting("Config Root")+"RoR.cfg");

	String logFilename   = SETTINGS.getSetting("Log Path") + name + Ogre::String(".log");
	String pluginsConfig = SETTINGS.getSetting("plugins.cfg");
	String ogreConfig    = SETTINGS.getSetting("ogre.cfg");
    m_pRoot = new Ogre::Root(pluginsConfig, ogreConfig, logFilename);

	// configure RoR
	configure();

    m_pViewport = m_pRenderWnd->addViewport(0);
    m_pViewport->setBackgroundColour(ColourValue(0.5f, 0.5f, 0.5f, 1.0f));

    m_pViewport->setCamera(0);
	m_pViewport->setBackgroundColour(ColourValue::Black);

	//
	//mRoot->setFrameSmoothingPeriod(2.0);

	// init inputsystem


    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
    //Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    //m_pTrayMgr = new OgreBites::SdkTrayManager("AOFTrayMgr", m_pRenderWnd, m_pMouse, 0);

    m_pTimer = new Ogre::Timer();
    m_pTimer->reset();

    m_pRenderWnd->setActive(true);

	// set window icon correctly
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#ifndef _UNICODE
	if(hwnd == 0)
	{
		// only in non-embedded mode
		size_t hWnd = 0;
		m_pRenderWnd->getCustomAttribute("WINDOW", &hWnd);

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

    return true;
}

void OgreFramework::updateOgre(double timeSinceLastFrame)
{
}
//|||||||||||||||||||||||||||||||||||||||||||||||
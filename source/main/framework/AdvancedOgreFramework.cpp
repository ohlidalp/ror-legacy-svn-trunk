//|||||||||||||||||||||||||||||||||||||||||||||||

#include "AdvancedOgreFramework.h"

#include "Settings.h"
#include "errorutils.h"
#include "language.h"

#include "RoRWindowEventUtilities.h"

//|||||||||||||||||||||||||||||||||||||||||||||||

using namespace Ogre;

//|||||||||||||||||||||||||||||||||||||||||||||||

template<> OgreFramework* Ogre::Singleton<OgreFramework>::ms_Singleton = 0;

//|||||||||||||||||||||||||||||||||||||||||||||||

OgreFramework::OgreFramework() : hwnd(Ogre::String()), mainhwnd(Ogre::String()), name(), embedded(false)
{
    m_pRoot			    = 0;
    m_pRenderWnd		= 0;
    m_pViewport			= 0;
    m_pTimer			= 0;
}

//|||||||||||||||||||||||||||||||||||||||||||||||

OgreFramework::~OgreFramework()
{
	LOG("Shutdown OGRE...");
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
	if(hwnd.empty())
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
			m_pRenderWnd = m_pRoot->initialise(true, "Rigs of Rods version " + String(ROR_VERSION_STRING));

			// set window icon correctly
#ifndef ROR_EMBEDDED
			{
				// only in non-embedded mode
				size_t hWnd = 0;
				m_pRenderWnd->getCustomAttribute("WINDOW", &hWnd);

				char buf[MAX_PATH];
				::GetModuleFileNameA(0, (LPCH)&buf, MAX_PATH);

				HINSTANCE instance = ::GetModuleHandleA(buf);
				HICON hIcon = ::LoadIconA(instance, MAKEINTRESOURCE(101));
				if (hIcon)
				{
					::SendMessageA((HWND)hWnd, WM_SETICON, 1, (LPARAM)hIcon);
					::SendMessageA((HWND)hWnd, WM_SETICON, 0, (LPARAM)hIcon);
				}
			}
#endif //ROR_EMBEDDED

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

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		param["externalWindowHandle"] = hwnd;
#else
		param["parentWindowHandle"] = hwnd;
		printf("### parentWindowHandle =  %s\n", hwnd.c_str());
#endif
		m_pRenderWnd = m_pRoot->createRenderWindow(name, 320, 240, false, &param);
		return true;
	}
	return false;
}

bool OgreFramework::initOgre(Ogre::String name, Ogre::String hwnd, Ogre::String mainhwnd, bool embedded)
{
	this->name     = name;
	this->hwnd     = hwnd;
	this->mainhwnd = mainhwnd;
	this->embedded = embedded;

	if(!SETTINGS.setupPaths())
		return false;

	// load RoR.cfg directly after setting up paths
	try
	{
		SETTINGS.loadSettings(SSETTING("Config Root")+"RoR.cfg");
	} catch(Ogre::Exception& e)
	{
		String url = "http://wiki.rigsofrods.com/index.php?title=Error_" + TOSTRING(e.getNumber())+"#"+e.getSource();
		showOgreWebError("A fatal exception has occured!", e.getFullDescription(), url);
		exit(1);
	}


	String logFilename   = SSETTING("Log Path") + name + Ogre::String(".log");
	String pluginsConfig = SSETTING("plugins.cfg");
	String ogreConfig    = SSETTING("ogre.cfg");
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

    return true;
}


void OgreFramework::resized(Ogre::Vector2 size)
{
	// trigger resizing of all sub-components
	RoRWindowEventUtilities::triggerResize(m_pRenderWnd);

	// Set the aspect ratio for the new size
	if(m_pViewport->getCamera())
		m_pViewport->getCamera()->setAspectRatio(Ogre::Real(size.x) / Ogre::Real(size.y));
}
//|||||||||||||||||||||||||||||||||||||||||||||||







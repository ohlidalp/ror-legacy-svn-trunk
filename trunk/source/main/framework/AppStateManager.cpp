//|||||||||||||||||||||||||||||||||||||||||||||||

#include "AppStateManager.h"

#include "RoRWindowEventUtilities.h"
#include <OgreLogManager.h>

using namespace Ogre;

//|||||||||||||||||||||||||||||||||||||||||||||||

AppStateManager::AppStateManager()
{
	m_bShutdown = false;
	m_bNoRendering = false;
	pthread_mutex_init(&lock, NULL);
}

//|||||||||||||||||||||||||||||||||||||||||||||||

AppStateManager::~AppStateManager()
{
	state_info si;

    while(!m_ActiveStateStack.empty())
	{
		m_ActiveStateStack.back()->exit();
		m_ActiveStateStack.pop_back();
	}

	while(!m_States.empty())
	{
		si = m_States.back();
        si.state->destroy();
        m_States.pop_back();
	}
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::manageAppState(Ogre::String stateName, AppState* state)
{
	try
	{
		state_info new_state_info;
		new_state_info.name = stateName;
		new_state_info.state = state;
		m_States.push_back(new_state_info);
	}
	catch(std::exception& e)
	{
		delete state;
		throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "Error while trying to manage a new AppState\n" + Ogre::String(e.what()), "AppStateManager.cpp (39)");
	}
}

//|||||||||||||||||||||||||||||||||||||||||||||||

AppState* AppStateManager::findByName(Ogre::String stateName)
{
	std::vector<state_info>::iterator itr;

	for(itr=m_States.begin();itr!=m_States.end();itr++)
	{
		if(itr->name==stateName)
			return itr->state;
	}

	return 0;
}

//|||||||||||||||||||||||||||||||||||||||||||||||
void AppStateManager::update(double dt)
{
	pthread_mutex_lock(&lock);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	RoRWindowEventUtilities::messagePump();
#endif
	if(OgreFramework::getSingletonPtr()->m_pRenderWnd->isClosed())
	{
		shutdown();
		pthread_mutex_unlock(&lock);
		return;
	}

	m_ActiveStateStack.back()->update(dt);
	OgreFramework::getSingletonPtr()->m_pRoot->renderOneFrame();
	pthread_mutex_unlock(&lock);
}

void AppStateManager::start(AppState* state)
{
	changeAppState(state);

	int timeSinceLastFrame = 1;
	int startTime = 0;

	while(!m_bShutdown)
	{
		startTime = OgreFramework::getSingletonPtr()->m_pTimer->getMillisecondsCPU();

		// no more actual rendering?
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		if(m_bNoRendering)
		{
#ifdef WIN32
			Sleep(100);
#else
			sleep(100);
#endif // WIN32
		} else
		{
			update(timeSinceLastFrame);
		}

		/*
		// BUGGY:
		if(OgreFramework::getSingletonPtr()->m_pRenderWnd->isActive())
		{
			update(timeSinceLastFrame);
		} else
		{
			Sleep(1000);
			// linux: sleep(1);
		}
		*/
#else
		// BUG: somehow, OgreFramework::getSingletonPtr()->m_pRenderWnd->isActive()) returns always false under linux, even if window is active. Removed this optimization for now
		update(timeSinceLastFrame);
#endif // WIN32

		timeSinceLastFrame = OgreFramework::getSingletonPtr()->m_pTimer->getMillisecondsCPU() - startTime;
	}
	LOG("Main loop quit");
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::changeAppState(AppState* state)
{
	if(!m_ActiveStateStack.empty())
	{
		m_ActiveStateStack.back()->exit();
		m_ActiveStateStack.pop_back();
	}

	m_ActiveStateStack.push_back(state);
	init(state);
	m_ActiveStateStack.back()->enter();
}

//|||||||||||||||||||||||||||||||||||||||||||||||

bool AppStateManager::pushAppState(AppState* state)
{
	if(!m_ActiveStateStack.empty())
	{
		if(!m_ActiveStateStack.back()->pause())
			return false;
	}

	m_ActiveStateStack.push_back(state);
	init(state);
	m_ActiveStateStack.back()->enter();

	return true;
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::popAppState()
{
	if(!m_ActiveStateStack.empty())
	{
		m_ActiveStateStack.back()->exit();
		m_ActiveStateStack.pop_back();
	}

	if(!m_ActiveStateStack.empty())
	{
		init(m_ActiveStateStack.back());
		m_ActiveStateStack.back()->resume();
	}
    else
		shutdown();
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::popAllAndPushAppState(AppState* state)
{
    while(!m_ActiveStateStack.empty())
    {
        m_ActiveStateStack.back()->exit();
        m_ActiveStateStack.pop_back();
    }

    pushAppState(state);
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::pauseAppState()
{
	if(!m_ActiveStateStack.empty())
	{
		m_ActiveStateStack.back()->pause();
	}

	if(m_ActiveStateStack.size() > 2)
	{
		init(m_ActiveStateStack.at(m_ActiveStateStack.size() - 2));
		m_ActiveStateStack.at(m_ActiveStateStack.size() - 2)->resume();
	}
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::shutdown()
{
	// shutdown needs to be synced
	pthread_mutex_lock(&lock);
	m_bShutdown = true;
	pthread_mutex_unlock(&lock);
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::pauseRendering()
{
	// shutdown needs to be synced
	pthread_mutex_lock(&lock);
	m_bNoRendering = true;
	pthread_mutex_unlock(&lock);
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::init(AppState* state)
{

	OgreFramework::getSingletonPtr()->m_pRenderWnd->resetStatistics();
}

void AppStateManager::resized(Ogre::RenderWindow *r)
{
	m_ActiveStateStack.back()->resized(r);
}

//|||||||||||||||||||||||||||||||||||||||||||||||
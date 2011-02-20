#include "GameState.h"

#include "RoRFrameListener.h"

using namespace Ogre;

GameState::GameState()
{
}

void GameState::enter()
{
    LogManager::getSingleton().logMessage("Entering GameState...");

	m_pSceneMgr = OgreFramework::getSingletonPtr()->m_pRoot->createSceneManager(ST_EXTERIOR_CLOSE);

	//CREATE CAMERA
	LogManager::getSingleton().logMessage("Creating camera");
	// Create the camera
	m_pCamera = m_pSceneMgr->createCamera("PlayerCam");
	// Position it at 500 in Z direction
	m_pCamera->setPosition(Vector3(128,25,128));
	// Look back along -Z
	m_pCamera->lookAt(Vector3(0,0,-300));
	m_pCamera->setNearClipDistance( 0.5 );
	m_pCamera->setFarClipDistance( 1000.0*1.733 );
	m_pCamera->setFOVy(Degree(60));
	m_pCamera->setAutoAspectRatio(true);
	OgreFramework::getSingletonPtr()->m_pViewport->setCamera(m_pCamera);


	LogManager::getSingleton().logMessage("Adding Frame Listener");
	bool isEmbedded = OgreFramework::getSingleton().isEmbedded();
	
	RoRFrameListener *mFrameListener = new RoRFrameListener(
		OgreFramework::getSingleton().m_pRenderWnd,
		m_pCamera,
		m_pSceneMgr,
		OgreFramework::getSingletonPtr()->m_pRoot,
		OgreFramework::getSingleton().isEmbedded(),
		OgreFramework::getSingleton().getMainHWND()
		);
	
	OgreFramework::getSingleton().m_pRoot->addFrameListener(mFrameListener);

}

bool GameState::pause()
{
    LogManager::getSingleton().logMessage("Pausing GameState...");

    return true;
}

void GameState::resume()
{
    LogManager::getSingleton().logMessage("Resuming GameState...");

    OgreFramework::getSingletonPtr()->m_pViewport->setCamera(m_pCamera);
    m_bQuit = false;
}

void GameState::exit()
{
    LogManager::getSingleton().logMessage("Leaving GameState...");

    m_pSceneMgr->destroyCamera(m_pCamera);
    if(m_pSceneMgr)
        OgreFramework::getSingletonPtr()->m_pRoot->destroySceneManager(m_pSceneMgr);
}

void GameState::createScene()
{
}

void GameState::update(double timeSinceLastFrame)
{
}

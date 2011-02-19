
#include "GameState.hpp"

//|||||||||||||||||||||||||||||||||||||||||||||||

using namespace Ogre;

//|||||||||||||||||||||||||||||||||||||||||||||||

GameState::GameState()
{
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void GameState::enter()
{
    OgreFramework::getSingletonPtr()->m_pLog->logMessage("Entering GameState...");

    m_pSceneMgr = OgreFramework::getSingletonPtr()->m_pRoot->createSceneManager(ST_GENERIC, "GameSceneMgr");
    m_pSceneMgr->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));

    m_pRSQ = m_pSceneMgr->createRayQuery(Ray());
    m_pRSQ->setQueryMask(OGRE_HEAD_MASK);

    m_pCamera = m_pSceneMgr->createCamera("GameCamera");
    m_pCamera->setPosition(Vector3(5, 60, 60));
    m_pCamera->lookAt(Vector3(5, 20, 0));
    m_pCamera->setNearClipDistance(5);

    m_pCamera->setAspectRatio(Real(OgreFramework::getSingletonPtr()->m_pViewport->getActualWidth()) /
        Real(OgreFramework::getSingletonPtr()->m_pViewport->getActualHeight()));

    OgreFramework::getSingletonPtr()->m_pViewport->setCamera(m_pCamera);
    m_pCurrentObject = 0;

    createScene();
}

//|||||||||||||||||||||||||||||||||||||||||||||||

bool GameState::pause()
{
    OgreFramework::getSingletonPtr()->m_pLog->logMessage("Pausing GameState...");

    return true;
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void GameState::resume()
{
    OgreFramework::getSingletonPtr()->m_pLog->logMessage("Resuming GameState...");

    OgreFramework::getSingletonPtr()->m_pViewport->setCamera(m_pCamera);
    m_bQuit = false;
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void GameState::exit()
{
    OgreFramework::getSingletonPtr()->m_pLog->logMessage("Leaving GameState...");

    m_pSceneMgr->destroyCamera(m_pCamera);
    m_pSceneMgr->destroyQuery(m_pRSQ);
    if(m_pSceneMgr)
        OgreFramework::getSingletonPtr()->m_pRoot->destroySceneManager(m_pSceneMgr);
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void GameState::createScene()
{
    m_pSceneMgr->createLight("Light")->setPosition(75,75,75);

	/*
    DotSceneLoader* pDotSceneLoader = new DotSceneLoader();
    pDotSceneLoader->parseDotScene("CubeScene.xml", "General", m_pSceneMgr, m_pSceneMgr->getRootSceneNode());
    delete pDotSceneLoader;
	*/

    m_pSceneMgr->getEntity("Cube01")->setQueryFlags(CUBE_MASK);
    m_pSceneMgr->getEntity("Cube02")->setQueryFlags(CUBE_MASK);
    m_pSceneMgr->getEntity("Cube03")->setQueryFlags(CUBE_MASK);

    m_pOgreHeadEntity = m_pSceneMgr->createEntity("Cube", "ogrehead.mesh");
    m_pOgreHeadEntity->setQueryFlags(OGRE_HEAD_MASK);
    m_pOgreHeadNode = m_pSceneMgr->getRootSceneNode()->createChildSceneNode("CubeNode");
    m_pOgreHeadNode->attachObject(m_pOgreHeadEntity);
    m_pOgreHeadNode->setPosition(Vector3(0, 0, -25));

    m_pOgreHeadMat = m_pOgreHeadEntity->getSubEntity(1)->getMaterial();
    m_pOgreHeadMatHigh = m_pOgreHeadMat->clone("OgreHeadMatHigh");
    m_pOgreHeadMatHigh->getTechnique(0)->getPass(0)->setAmbient(1, 0, 0);
    m_pOgreHeadMatHigh->getTechnique(0)->getPass(0)->setDiffuse(1, 0, 0, 0);
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void GameState::moveCamera()
{
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void GameState::update(double timeSinceLastFrame)
{

}

//|||||||||||||||||||||||||||||||||||||||||||||||
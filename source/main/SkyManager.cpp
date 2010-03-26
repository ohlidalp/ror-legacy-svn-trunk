#include "SkyManager.h"

#include "Ogre.h"
#include "Caelum.h"

using namespace std;
using namespace Ogre;
using namespace Caelum;

//---------------------------------------------------------------------
template<> SkyManager * Singleton< SkyManager >::ms_Singleton = 0;
SkyManager* SkyManager::getSingletonPtr(void)
{
	return ms_Singleton;
}
SkyManager& SkyManager::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

SkyManager::SkyManager()
{
}

SkyManager::~SkyManager()
{
}

void SkyManager::init(Ogre::SceneManager *mScene, Ogre::RenderWindow *mWindow, Ogre::Camera *mCamera)
{
	Caelum::CaelumSystem::CaelumComponent componentMask;
	componentMask = static_cast<Caelum::CaelumSystem::CaelumComponent> (
			Caelum::CaelumSystem::CAELUM_COMPONENT_SUN |				
			Caelum::CaelumSystem::CAELUM_COMPONENT_MOON |
			Caelum::CaelumSystem::CAELUM_COMPONENT_SKY_DOME |
			//Caelum::CaelumSystem::CAELUM_COMPONENT_IMAGE_STARFIELD |
			Caelum::CaelumSystem::CAELUM_COMPONENT_POINT_STARFIELD |
			Caelum::CaelumSystem::CAELUM_COMPONENT_SCREEN_SPACE_FOG |
			Caelum::CaelumSystem::CAELUM_COMPONENT_CLOUDS |
			0);

	// Initialise CaelumSystem.
	mCaelumSystem = new Caelum::CaelumSystem (Root::getSingletonPtr(), mScene, componentMask);

	// Set time acceleration.
	mCaelumSpeedFactor = 512;
	mCaelumSystem->getUniversalClock()->setTimeScale(512);

	// Register caelum as a listener.
	mWindow->addListener (mCaelumSystem);
	Root::getSingletonPtr()->addFrameListener(mCaelumSystem);

	mCaelumSystem->setSceneFogDensityMultiplier (0.0015);
	mCaelumSystem->setManageAmbientLight (true);
	mCaelumSystem->setMinimumAmbientLight (Ogre::ColourValue (0.1, 0.1, 0.1));

	mCaelumSystem->setDepthComposer (new Caelum::DepthComposer (mScene));

	Caelum::DepthComposerInstance* inst = mCaelumSystem->getDepthComposer ()->getViewportInstance (mCamera->getViewport());
	if(inst)
	{
		//inst->getDepthRenderer()->setRenderGroupRangeFilter (20, 80);
		inst->getDepthRenderer()->setViewportVisibilityMask (~0x00001000);
		mCaelumSystem->forceSubcomponentVisibilityFlags (0x00001000);

		mCaelumSystem->setGroundFogDensityMultiplier (0.03);
		mCaelumSystem->getDepthComposer ()->setGroundFogVerticalDecay (0.06);
		mCaelumSystem->getDepthComposer ()->setGroundFogBaseLevel (0);
	}

	//CaelumPlugin::getSingleton ().loadCaelumSystemFromScript (mCaelumSystem, "sky_system_name");
}

void SkyManager::setTimeFactor(double factor)
{
    mCaelumSpeedFactor += factor;
    mCaelumSystem->getUniversalClock()->setTimeScale (mCaelumSpeedFactor);
}


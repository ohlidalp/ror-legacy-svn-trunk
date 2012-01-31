/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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

#ifdef USE_CAELUM

#include "SkyManager.h"

#include "Settings.h"

#include "Ogre.h"
#include <Caelum.h>



using namespace std;
using namespace Ogre;
using namespace Caelum;

//---------------------------------------------------------------------
SkyManager::SkyManager() : mCamera(0), mCaelumSystem(0)
{
}

SkyManager::~SkyManager()
{
}

void SkyManager::init(Ogre::SceneManager *mScene, Ogre::RenderWindow *mWindow, Ogre::Camera *mCamera)
{
	// Initialise CaelumSystem.
	this->mCamera = mCamera;
	mCaelumSystem = new Caelum::CaelumSystem (Root::getSingletonPtr(), mScene, Caelum::CaelumSystem::CAELUM_COMPONENTS_NONE);
	mCaelumSystem->attachViewport(mCamera->getViewport());

	/*
	// TODO: set real time, and let the user select his true location
	mCaelumSystem->getUniversalClock()->setGregorianDateTime(2008, 4, 9, 6, 33, 0);
	mCaelumSystem->setObserverLongitude(Ogre::Degree(0));
	mCaelumSystem->setObserverLatitude(Ogre::Degree(0));
	mCaelumSystem->getUniversalClock()->setTimeScale(100);
	*/

	// Register caelum as a listener.
	mWindow->addListener (mCaelumSystem);
	Root::getSingletonPtr()->addFrameListener(mCaelumSystem);

	
}

void SkyManager::notifyCameraChanged(Camera *cam)
{
	if(mCaelumSystem)
		mCaelumSystem->notifyCameraChanged(cam);
}

void SkyManager::forceUpdate(float dt)
{
	if(mCaelumSystem)
		mCaelumSystem->updateSubcomponents(dt);
}

void SkyManager::loadScript(Ogre::String script)
{
	// load the caelum config
	try
	{
		CaelumPlugin::getSingleton().loadCaelumSystemFromScript (mCaelumSystem, script);

		// overwrite some settings
#ifdef CAELUM_VERSION_SEC
		// important: overwrite fog setings if not using infinite farclip
		if(mCamera->getFarClipDistance() > 0)
		{
			// non infinite farclip
			Real farclip = mCamera->getFarClipDistance();
			mCaelumSystem->setManageSceneFog(Ogre::FOG_LINEAR);
			mCaelumSystem->setManageSceneFogStart(farclip * 0.7f);
			mCaelumSystem->setManageSceneFogEnd(farclip * 0.9f);
		} else
		{
			// no fog in infinite farclip
			mCaelumSystem->setManageSceneFog(Ogre::FOG_NONE);
		}
#else
#error please use a recent Caelum version, see http://www.rigsofrods.com/wiki/pages/Compiling_3rd_party_libraries#Caelum
#endif // CAELUM_VERSION
		// now optimize the moon a bit
		if(mCaelumSystem->getMoon())
		{
			mCaelumSystem->getMoon()->setAutoDisable(true);
			//mCaelumSystem->getMoon()->setAutoDisableThreshold(1);
			mCaelumSystem->getMoon()->setForceDisable(true);
			mCaelumSystem->getMoon()->getMainLight()->setCastShadows(false);
		}

		mCaelumSystem->setEnsureSingleShadowSource(true);
		mCaelumSystem->setEnsureSingleLightSource(true);

		// enforcing update, so shadows are set correctly before creating the terrain
		forceUpdate(0.1);

	} catch(Ogre::Exception& e)
	{
		LOG("exception upon loading sky script: " + e.getFullDescription());
	}
}

void SkyManager::setTimeFactor(LongReal factor)
{
    mCaelumSystem->getUniversalClock()->setTimeScale (factor);
}

Ogre::Light *SkyManager::getMainLight()
{
	if(mCaelumSystem && mCaelumSystem->getSun())
		return mCaelumSystem->getSun()->getMainLight();
	return 0;
}
LongReal SkyManager::getTimeFactor()
{
    return mCaelumSystem->getUniversalClock()->getTimeScale();
}

Ogre::String SkyManager::getPrettyTime()
{
	int ignore;
	int hour;
	int minute;
	Caelum::LongReal second;
	Caelum::Astronomy::getGregorianDateTimeFromJulianDay(mCaelumSystem->getJulianDay()
	, ignore, ignore, ignore, hour, minute, second);
	
	return Ogre::StringConverter::toString( hour, 2, '0' )
	+ ":" + Ogre::StringConverter::toString( minute, 2, '0' )
	+ ":" + Ogre::StringConverter::toString( (int)second, 2, '0' );
}

#endif //USE_CAELUM

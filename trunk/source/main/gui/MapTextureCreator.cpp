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
#include "MapTextureCreator.h"

#include "ResourceBuffer.h"
#include "RoRFrameListener.h"
#include "water.h"

using namespace Ogre;

int MapTextureCreator::mCounter = 0;

MapTextureCreator::MapTextureCreator(SceneManager *mgr, Camera *maincam, RoRFrameListener *efl) :
	  mSceneManager(mgr)
	, mMainCam(mMainCam)
	, mEfl(efl)
	, mCamdir(Quaternion::ZERO)
	, mCamera(NULL)
	, mCampos(Vector3::ZERO)
	, mMaterial(NULL)
	, mRttTex(NULL)
	, mStatics(NULL)
	, mTextureUnitState(NULL)
	, mViewport(NULL)
{
	mCounter++;
}

bool MapTextureCreator::init()
{
	TexturePtr texture = TextureManager::getSingleton().createManual("MapRttTex"+TOSTRING(mCounter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 1024, 1024, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
	
	if ( texture.isNull() ) return false;;

	mRttTex = texture->getBuffer()->getRenderTarget();

	if ( !mRttTex ) return false;;

	mRttTex->setAutoUpdated(true);

	mCamera = mSceneManager->createCamera("MapRenderCam");

	mViewport = mRttTex->addViewport(mCamera);
	mViewport->setBackgroundColour(ColourValue::White);
	mViewport->setOverlaysEnabled(false);

	mMaterial = MaterialManager::getSingleton().create("MapRttMat"+TOSTRING(mCounter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	if ( mMaterial.isNull() ) return false;

	mTextureUnitState = mMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("MapRttTex"+TOSTRING(mCounter));

	mRttTex->addListener(this);

	mCamera->setFarClipDistance(0);
	mCamera->setAspectRatio(1.0);
	mCamera->setFixedYawAxis(false);
	mCamera->setProjectionType(PT_ORTHOGRAPHIC);
	//mCamera->setFOVy(Radian(Math::HALF_PI));

	return true;
}


void MapTextureCreator::setStaticGeometry(StaticGeometry *geo)
{
	mStatics = geo;
}

void MapTextureCreator::update()
{
	if ( !mRttTex && !init() ) return;

	float width = mEfl->mapsizex;
	float height = mEfl->mapsizez;
	
	mCamera->setOrthoWindow(1024, 1024);
	mCamera->setPosition(width*0.5f, 100.0f, height*0.5f);
	mCamera->lookAt(width*0.5f + 0.0001f, 0.0f, height*0.5f);

	mRttTex->update();
#if 0
	// 1 = max out = total overview

	float width = mEfl->mapsizex;
	float height = mEfl->mapsizez;

	//LOG(TOSTRING(mZoom));
	mCamera->setOrthoWindow(1024, 1024);
	mCamera->setPosition(width*0.5f, 100, height*0.5f);
	mCamera->lookAt(width*0.5f + 0.0001f, 0, height*0.5f);
	//mCamera->setOrientation(Quaternion(Radian(0), Vector3::UNIT_Z));


	/*
	// this is bugged, so deactivated for now
	float f = 50-mZoom;
	if(f<20)
		f=20;
	for (int i=0; i<mEfl->getTruckCount(); i++)
		mEfl->getTruck(i)->preMapLabelRenderUpdate(true, f);
	*/

	if (mStatics)
	{
		mStatics->setRenderingDistance(0);
	}
	// thats a huge workaround to be able to not use the normal LOD

	setFogVisible(false);

	mRttTex->update();

	setFogVisible(true);

	if (mEfl->getWater())
	{
		mEfl->getWater()->setVisible(false);
	}
	if (mStatics)
	{
		mStatics->setRenderingDistance(1000);
	}
	if (mEfl->getWater())
	{
		mEfl->getWater()->setVisible(true);
	}
	/*
	// deactivated for the moment
	for (int i=0; i<mEfl->getTruckCount(); i++)
		mEfl->getTruck(i)->preMapLabelRenderUpdate(false);
	*/
#endif
}

void MapTextureCreator::setFogVisible(bool value)
{
#if 0
	return;
	int fogmode = 0; //mEfl->getFogMode();
	//LOG("fogswitch: "+TOSTRING(fogmode)+" / "+TOSTRING(value));
	if(!fogmode || fogmode == 2)
		return;

	// this refuses to work, somehow:
	if(fogmode == 1)
	{
		// TODO: tofix: caelum
		// USE_CAELUM
		if(value)
			static_cast<Caelum::StoredImageSkyColourModel *>(mEfl->getCaelumModel())->setFogDensity(mEfl->getFogDensity());
		else
			static_cast<Caelum::StoredImageSkyColourModel *>(mEfl->getCaelumModel())->setFogDensity(0);

		// force Caelum to update
		if(value)
			mEfl->getCaelumSystem()->setLocalTime(mEfl->getCaelumSystem()->getLocalTime()+1);
		else
			mEfl->getCaelumSystem()->setLocalTime(mEfl->getCaelumSystem()->getLocalTime()-1);
	}
	else if(fogmode == 3)
	{
		if(value)
			mSceneManager->setFog(FOG_LINEAR, mSceneManager->getFogColour(), mEfl->getFogDensity(), mSceneManager->getFogStart(), mSceneManager->getFogEnd());
		else
			mSceneManager->setFog(FOG_NONE, mSceneManager->getFogColour(), mEfl->getFogDensity(), mSceneManager->getFogStart(), mSceneManager->getFogEnd());
	}

#endif //0
}

String MapTextureCreator::getMaterialName()
{
	return "MapRttMat"+TOSTRING(mCounter);
}

String MapTextureCreator::getRTName()
{
	return "MapRttTex"+TOSTRING(mCounter);
}

void MapTextureCreator::preRenderTargetUpdate(const RenderTargetEvent& evt)
{
	Water *w = mEfl->getWater();
	if ( w )
	{
		w->setVisible(false);
	}
}
void MapTextureCreator::postRenderTargetUpdate(const RenderTargetEvent& evt)
{
	Water *w = mEfl->getWater();
	if ( w )
	{
		w->setVisible(true);
	}
}

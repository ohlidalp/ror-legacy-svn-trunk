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

#include "heightfinder.h"
#include "ResourceBuffer.h"
#include "RoRFrameListener.h"
#include "water.h"

using namespace Ogre;

int MapTextureCreator::mCounter = 0;

MapTextureCreator::MapTextureCreator(SceneManager *mgr, Camera *maincam, RoRFrameListener *efl) :
	  mSceneManager(mgr)
	, mMainCam(mMainCam)
	, mEfl(efl)
	, mCamDir(Quaternion::ZERO)
	, mCamPos(Vector3::ZERO)
	, mCamera(NULL)
	, mMaterial(NULL)
	, mRttTex(NULL)
	, mStatics(NULL)
	, mTextureUnitState(NULL)
	, mViewport(NULL)
	, mZoom(3.0f)
{
	mCounter++;
	init();
}

bool MapTextureCreator::init()
{
	TexturePtr texture = TextureManager::getSingleton().createManual("MapRttTex" + TOSTRING(mCounter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 1024, 1024, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
	
	if ( texture.isNull() ) return false;;

	mRttTex = texture->getBuffer()->getRenderTarget();

	if ( !mRttTex ) return false;;

	mRttTex->setAutoUpdated(true);

	mCamera = mSceneManager->createCamera("MapRenderCam" + TOSTRING(mCounter));

	mViewport = mRttTex->addViewport(mCamera);
	mViewport->setBackgroundColour(ColourValue::Black);
	mViewport->setOverlaysEnabled(false);

	mMaterial = MaterialManager::getSingleton().create("MapRttMat" + TOSTRING(mCounter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	if ( mMaterial.isNull() ) return false;

	mTextureUnitState = mMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("MapRttTex" + TOSTRING(mCounter));

	mRttTex->addListener(this);

	mCamera->setPosition(0.0f, 1000.0f, 0.0f);
	mCamera->lookAt(Vector3::ZERO);

	mCamera->setFarClipDistance(0.0f);
	mCamera->setAspectRatio(1.0f);
	mCamera->setFixedYawAxis(false);
	mCamera->setProjectionType(PT_ORTHOGRAPHIC);
	mCamera->setFOVy(Radian(Math::HALF_PI));
	mCamera->setNearClipDistance(mZoom);

	return true;
}

void MapTextureCreator::setCameraMode(PolygonMode pm)
{
	mCamera->setPolygonMode(pm);
}

void MapTextureCreator::setCameraZoom(float z)
{
	mZoom = std::max(0.3f, z);
}

void MapTextureCreator::setCamPosition(Vector3 pos, Quaternion direction)
{
	mCamPos = pos;
	mCamDir = direction;
}

void MapTextureCreator::setStaticGeometry(StaticGeometry *geo)
{
	mStatics = geo;
}

void MapTextureCreator::update()
{
	if ( !mRttTex ) return;

	float width = mEfl->mapsizex;
	float height = mEfl->mapsizez;
	float zoomFactor = mZoom * ((width + height) / 2.0f) * 0.002f;
	
	mCamPos = Vector3(mEfl->mapsizex / 2.0f, mEfl->hfinder->getHeightAt(mEfl->mapsizex / 2.0f, mEfl->mapsizez / 2.0f) , mEfl->mapsizez / 2.0f);
	mCamDir = Quaternion(Degree(0), Vector3::UNIT_X);

	mCamera->setNearClipDistance(mZoom);
	mCamera->setPosition(mCamPos + Vector3(0.0f, zoomFactor, 0.0f));
	if ( mCamDir != Quaternion::ZERO )
	{
		mCamera->setOrientation(mCamDir);
	}
	mCamera->lookAt(mCamPos - Vector3(0.0f, zoomFactor, 0.0f));

	float f = std::max(20.0f, 50.0f - mZoom);

	if ( mStatics )
	{
		mStatics->setRenderingDistance(0);
	}

	mRttTex->update();

	Water *w = mEfl->getWater();

	if ( w )
	{
		w->setVisible(false);
	}
	if ( mStatics )
	{
		mStatics->setRenderingDistance(1000);
	}
	if ( w )
	{
		w->setVisible(true);
	}
}

String MapTextureCreator::getMaterialName()
{
	return "MapRttMat" + TOSTRING(mCounter);
}

String MapTextureCreator::getRTName()
{
	return "MapRttTex" + TOSTRING(mCounter);
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

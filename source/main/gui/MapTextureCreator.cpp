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

#include "BeamFactory.h"
#include "ResourceBuffer.h"
#include "RoRFrameListener.h"
#include "water.h"

using namespace Ogre;

int MapTextureCreator::mCounter = 0;

MapTextureCreator::MapTextureCreator(SceneManager *scm, Camera *cam, RoRFrameListener *efl) :
	  mSceneManager(scm)
	, mMainCam(cam)
	, mEfl(efl)
	, mCamOrientation(Quaternion::ZERO)
	, mCamLookAt(Vector3::ZERO)
	, mCamera(NULL)
	, mMaterial(NULL)
	, mRttTex(NULL)
	, mStatics(NULL)
	, mTextureUnitState(NULL)
	, mViewport(NULL)
	, mCamZoom(3.0f)
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

	mCamera->setFarClipDistance(0.0f);
	mCamera->setAspectRatio(1.0f);
	mCamera->setFixedYawAxis(false);
	mCamera->setProjectionType(PT_ORTHOGRAPHIC);
	mCamera->setFOVy(Radian(Math::HALF_PI));
	mCamera->setNearClipDistance(mCamZoom);

	return true;
}

void MapTextureCreator::setCameraMode(PolygonMode polygonMode)
{
	mCamera->setPolygonMode(polygonMode);
}

void MapTextureCreator::setCameraZoom(Real zoom)
{
	mCamZoom = std::max(0.3f, zoom);
}

void MapTextureCreator::setCameraZoomRelative(Real zoomDelta)
{
	mCamZoom = std::max(0.3f, mCamZoom + zoomDelta * mCamZoom / 100.0f);
}

void MapTextureCreator::setCamera(Vector3 lookAt, Quaternion orientation)
{
	mCamLookAt = lookAt;
	mCamOrientation = orientation;
}

void MapTextureCreator::setStaticGeometry(StaticGeometry *staticGeometry)
{
	mStatics = staticGeometry;
}

void MapTextureCreator::update()
{
	if ( !mRttTex ) return;

	float width = mEfl->mapsizex;
	float height = mEfl->mapsizez;
	float zoomFactor = mCamZoom * ((width + height) / 2.0f) * 0.002f;

	mCamera->setNearClipDistance(mCamZoom);
	mCamera->setPosition(mCamLookAt + Vector3(0.0f, zoomFactor, 0.0f));
	if ( mCamOrientation != Quaternion::ZERO )
	{
		mCamera->setOrientation(mCamOrientation);
	}
	mCamera->lookAt(mCamLookAt);

	preRenderTargetUpdate();

	mRttTex->update();

	postRenderTargetUpdate();
}

String MapTextureCreator::getMaterialName()
{
	return "MapRttMat" + TOSTRING(mCounter);
}

String MapTextureCreator::getRTName()
{
	return "MapRttTex" + TOSTRING(mCounter);
}

void MapTextureCreator::preRenderTargetUpdate()
{
	Beam **trucks = BeamFactory::getSingleton().getTrucks();

	float f = std::max(20.0f, 50.0f - mCamZoom);

	for (int i=0; i < BeamFactory::getSingleton().getTruckCount(); i++)
	{
		if ( trucks[i] )
		{
			trucks[i]->preMapLabelRenderUpdate(true, f);
		}
	}

	Water *w = mEfl->getWater();

	if ( w )
	{
		w->setVisible(false);
	}

	if ( mStatics )
	{
		mStatics->setRenderingDistance(0);
	}
}

void MapTextureCreator::postRenderTargetUpdate()
{
	Beam **trucks = BeamFactory::getSingleton().getTrucks();

	for (int i=0; i < BeamFactory::getSingleton().getTruckCount(); i++)
	{
		if ( trucks[i] )
		{
			trucks[i]->preMapLabelRenderUpdate(false);
		}
	}

	Water *w = mEfl->getWater();

	if ( w )
	{
		w->setVisible(true);
	}

	if ( mStatics )
	{
		mStatics->setRenderingDistance(1000);
	}
}

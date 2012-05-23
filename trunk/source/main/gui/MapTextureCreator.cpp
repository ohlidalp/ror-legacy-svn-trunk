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
#include "IWater.h"
#include "MapControl.h"
#include "ResourceBuffer.h"

using namespace Ogre;

int MapTextureCreator::mCounter = 0;

MapTextureCreator::MapTextureCreator(SceneManager *scm, Camera *cam, MapControl *ctrl) :
	  mSceneManager(scm)
	, mMainCam(cam)
	, mMapControl(ctrl)
	, mCamera(NULL)
	, mMapCenter(Vector3::ZERO)
	, mMapZoom(0.0f)
	, mMaterial(NULL)
	, mRttTex(NULL)
	, mStatics(NULL)
	, mTextureUnitState(NULL)
	, mViewport(NULL)
{
	mCounter++;
	init();
}

bool MapTextureCreator::init()
{
	TexturePtr texture = TextureManager::getSingleton().createManual("MapRttTex" + TOSTRING(mCounter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 2048, 2048, TU_RENDERTARGET, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
	
	if ( texture.isNull() ) return false;;

	mRttTex = texture->getBuffer()->getRenderTarget();

	if ( !mRttTex ) return false;

	mRttTex->setAutoUpdated(false);

	mCamera = mSceneManager->createCamera("MapRenderCam" + TOSTRING(mCounter));

	mViewport = mRttTex->addViewport(mCamera);
	mViewport->setBackgroundColour(ColourValue::Black);
	mViewport->setOverlaysEnabled(false);

	mMaterial = MaterialManager::getSingleton().create("MapRttMat" + TOSTRING(mCounter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	if ( mMaterial.isNull() ) return false;

	mTextureUnitState = mMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("MapRttTex" + TOSTRING(mCounter));

	mRttTex->addListener(this);

	mCamera->setFixedYawAxis(false);
	mCamera->setProjectionType(PT_ORTHOGRAPHIC);
	mCamera->setNearClipDistance(1.0f);

	return true;
}

void MapTextureCreator::setMapZoom(Real zoomValue)
{
	mMapZoom = std::max(0.0f, zoomValue);
	mMapZoom = std::min(zoomValue, 1.0f);
}

void MapTextureCreator::setMapZoomRelative(Real zoomDelta)
{
	setMapZoom(mMapZoom + zoomDelta * mMapZoom / 100.0f);
}

void MapTextureCreator::setMapCenter(Vector3 position)
{
	mMapCenter = position;
	mMapCenter.y = 0.0f;
}

void MapTextureCreator::setStaticGeometry(StaticGeometry *staticGeometry)
{
	mStatics = staticGeometry;
}

void MapTextureCreator::update()
{
	if ( !mRttTex ) return;

	Vector3 mapSize = mMapControl->getMapSize();
	float orthoWindowWidth  = mapSize.x - (mapSize.x - 20.0f) * mMapZoom;
	float orthoWindowHeight = mapSize.z - (mapSize.z - 20.0f) * mMapZoom;

	mCamera->setFarClipDistance(mapSize.y + 3.0f);
	mCamera->setOrthoWindow(orthoWindowWidth, orthoWindowHeight);
	mCamera->setPosition(mMapCenter + Vector3(0.0f, mapSize.y + 2.0f, 0.0f));
	mCamera->lookAt(mMapCenter);

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

	float f = 20.0f + 30.0f * mMapZoom;

	for (int i=0; i < BeamFactory::getSingleton().getTruckCount(); i++)
	{
		if ( trucks[i] )
		{
			trucks[i]->preMapLabelRenderUpdate(true, f);
		}
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

	if ( mStatics )
	{
		mStatics->setRenderingDistance(1000);
	}
}

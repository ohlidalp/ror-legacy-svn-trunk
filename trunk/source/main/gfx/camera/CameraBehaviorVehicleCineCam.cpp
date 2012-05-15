/*/
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
#include "CameraBehaviorVehicleCineCam.h"

#include "Beam.h"
#include "OverlayWrapper.h"
#include "Settings.h"

using namespace Ogre;

CameraBehaviorVehicleCineCam::CameraBehaviorVehicleCineCam() :
	  CameraBehaviorVehicle()
{
	fovInternal = FSETTING("FOV Internal", 75);
	fovExternal = FSETTING("FOV External", 60);
}

void CameraBehaviorVehicleCineCam::update(const CameraManager::cameraContext_t &ctx)
{
	Vector3 dir = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[ctx.mCurrTruck->currentcamera]].smoothpos
				 - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodedir[ctx.mCurrTruck->currentcamera]].smoothpos).normalisedCopy();

	Vector3 roll = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[ctx.mCurrTruck->currentcamera]].smoothpos
				  - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranoderoll[ctx.mCurrTruck->currentcamera]].smoothpos).normalisedCopy();

	Vector3 up = dir.crossProduct(roll);

	roll = up.crossProduct(dir);

	CameraBehavior::update(ctx);

	Quaternion orientation = Quaternion(camRotX, up) * Quaternion(Degree(180.0) + camRotY, roll) * Quaternion(roll, up, dir);
	
	ctx.mCamera->setPosition(ctx.mCurrTruck->nodes[ctx.mCurrTruck->cinecameranodepos[ctx.mCurrTruck->currentcamera]].smoothpos);
	ctx.mCamera->setOrientation(orientation);
}

void CameraBehaviorVehicleCineCam::activate(const CameraManager::cameraContext_t &ctx)
{
	if (ctx.mCurrTruck->freecinecamera <= 0)
	{
		CameraManager::getSingleton().switchToNextBehavior();
		return;
	}

	ctx.mCamera->setFOVy(Degree(fovInternal));

	camRotX = 0.0f;
	camRotY = Degree(DEFAULT_INTERNAL_CAM_PITCH);

	ctx.mCurrTruck->prepareInside(true);

	if ( ctx.mOverlayWrapper )
	{
		if(ctx.mCurrTruck->driveable == AIRPLANE)
			ctx.mOverlayWrapper->showDashboardOverlays(true, ctx.mCurrTruck);
		else
			ctx.mOverlayWrapper->showDashboardOverlays(false, ctx.mCurrTruck);
	}

	ctx.mCurrTruck->currentcamera = 0;
	ctx.mCurrTruck->changedCamera();
}

void CameraBehaviorVehicleCineCam::deactivate(const CameraManager::cameraContext_t &ctx)
{
	ctx.mCamera->setFOVy(Degree(fovExternal));

	ctx.mCurrTruck->prepareInside(false);

	if ( ctx.mOverlayWrapper )
	{
		ctx.mOverlayWrapper->showDashboardOverlays(true, ctx.mCurrTruck);
	}

	ctx.mCurrTruck->currentcamera= - 1;
	ctx.mCurrTruck->changedCamera();
}

bool CameraBehaviorVehicleCineCam::switchBehavior(const CameraManager::cameraContext_t &ctx)
{
	if ( ctx.mCurrTruck->currentcamera < ctx.mCurrTruck->freecinecamera-1 )
	{
		ctx.mCurrTruck->currentcamera++;
		ctx.mCurrTruck->changedCamera();
		return false;
	}
	
	return true;
}

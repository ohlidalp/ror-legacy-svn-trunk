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
	camDist = 0.1f;
	camRatio = 0.01f;
	minCamDist = 0.0f;
	maxCamDist = 0.5f;
}

void CameraBehaviorVehicleCineCam::update(const CameraManager::cameraContext_t &ctx)
{
	Vector3 dir = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[ctx.mCurrTruck->currentcamera]].smoothpos
				 - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodedir[ctx.mCurrTruck->currentcamera]].smoothpos).normalisedCopy();

	targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	targetPitch     = 0.0f;

	if ( camPitching )
	{
		targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	}

	camCenterPosition = ctx.mCurrTruck->nodes[ctx.mCurrTruck->cinecameranodepos[ctx.mCurrTruck->currentcamera]].smoothpos;

	CameraBehavior::update(ctx);
}

void CameraBehaviorVehicleCineCam::activate(const CameraManager::cameraContext_t &ctx)
{
	float fov = FSETTING("FOV Internal", 75);

	ctx.mCamera->setFOVy(Degree(fov));

	camRotX = 0.0f;
	camRotY = 0.15f;

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
	float fov = FSETTING("FOV External", 60);

	ctx.mCamera->setFOVy(Degree(fov));
	ctx.mCamera->roll(Radian(0));

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

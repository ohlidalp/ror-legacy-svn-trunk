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
	CameraBehavior::update(ctx);

	Vector3 dir = (currTruck->nodes[currTruck->cameranodepos[currTruck->currentcamera]].smoothpos
				 - currTruck->nodes[currTruck->cameranodedir[currTruck->currentcamera]].smoothpos).normalisedCopy();

	Vector3 roll = (currTruck->nodes[currTruck->cameranodepos[currTruck->currentcamera]].smoothpos
				  - currTruck->nodes[currTruck->cameranoderoll[currTruck->currentcamera]].smoothpos).normalisedCopy();

	Vector3 up = dir.crossProduct(roll);

	roll = up.crossProduct(dir);

	Quaternion orientation = Quaternion(camRotX, up) * Quaternion(Degree(180.0) + camRotY, roll) * Quaternion(roll, up, dir);

	ctx.mCamera->setPosition(currTruck->nodes[currTruck->cinecameranodepos[currTruck->currentcamera]].smoothpos);
	ctx.mCamera->setOrientation(orientation);
}

void CameraBehaviorVehicleCineCam::activate(const CameraManager::cameraContext_t &ctx)
{
	CameraBehaviorVehicle::activate(ctx);

	if ( currTruck->freecinecamera <= 0 )
	{
		CameraManager::getSingleton().switchToNextBehavior();
		return;
	}

	ctx.mCamera->setFOVy(Degree(fovInternal));

	reset(ctx);

	currTruck->prepareInside(true);

	if ( ctx.mOverlayWrapper )
	{
		if ( currTruck->driveable == AIRPLANE )
			ctx.mOverlayWrapper->showDashboardOverlays(true, currTruck);
		else
			ctx.mOverlayWrapper->showDashboardOverlays(false, currTruck);
	}

	currTruck->currentcamera = 0;
	currTruck->changedCamera();
}

void CameraBehaviorVehicleCineCam::deactivate(const CameraManager::cameraContext_t &ctx)
{
	ctx.mCamera->setFOVy(Degree(fovExternal));

	currTruck->prepareInside(false);

	if ( ctx.mOverlayWrapper )
	{
		ctx.mOverlayWrapper->showDashboardOverlays(true, currTruck);
	}

	currTruck->currentcamera = -1;
	currTruck->changedCamera();
}

void CameraBehaviorVehicleCineCam::reset(const CameraManager::cameraContext_t &ctx)
{
	camRotX = 0.0f;
	camRotY = Degree(DEFAULT_INTERNAL_CAM_PITCH);
}

bool CameraBehaviorVehicleCineCam::switchBehavior(const CameraManager::cameraContext_t &ctx)
{
	if ( currTruck->currentcamera < currTruck->freecinecamera-1 )
	{
		currTruck->currentcamera++;
		currTruck->changedCamera();
		return false;
	}
	return true;
}

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
#include "CameraBehaviorVehicleCineCam.h"

#include "Beam.h"
#include "DepthOfFieldEffect.h"
#include "Settings.h"

using namespace Ogre;

CameraBehaviorVehicleCineCam::CameraBehaviorVehicleCineCam()
{
	camRatio = 0.0f;
}

bool CameraBehaviorVehicleCineCam::switchBehavior(const CameraManager::cameraContext_t &ctx)
{
	if ( ctx.mCurrTruck->currentcamera < ctx.mCurrTruck->freecinecamera-1 )
	{
		ctx.mCurrTruck->currentcamera++;

		// TODO

		return false;
	}
	
	// TODO

	ctx.mCurrTruck->currentcamera = -1;

	return true;
}

void CameraBehaviorVehicleCineCam::update(const CameraManager::cameraContext_t &ctx)
{
	// TODO
}

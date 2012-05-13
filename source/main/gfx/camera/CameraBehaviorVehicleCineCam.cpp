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

void CameraBehaviorVehicleCineCam::activate(CameraManager::cameraContext &ctx)
{
	float fov = FSETTING("FOV Internal", 75);

	if ( ctx.mDOF )
	{
		ctx.mDOF->setFocusMode(DOFManager::Manual);
		ctx.mDOF->setLensFOV(Degree(fov));
	}

	ctx.mCamera->setFOVy(Degree(fov));
}

void CameraBehaviorVehicleCineCam::deactivate(CameraManager::cameraContext &ctx)
{

}

bool CameraBehaviorVehicleCineCam::switchBehavior(CameraManager::cameraContext &ctx)
{
	return true;
}

void CameraBehaviorVehicleCineCam::update(CameraManager::cameraContext &ctx)
{

}

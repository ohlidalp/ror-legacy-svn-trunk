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
#include "CameraBehaviorVehicleInternal.h"

#include "CameraManager.h"
#include "Settings.h"

using namespace Ogre;

void CameraBehaviorVehicleInternal::activate(cameraContext_t &ctx)
{
	float fov = FSETTING("FOV Internal", 75);

	DOFManager *dof = CameraManager::getSingleton().getDOFManager();
	if ( dof )
	{
		dof->setFocusMode(DOFManager::Manual);
		dof->setLensFOV(Degree(fov));
	}

	Camera *cam = CameraManager::getSingleton().getCamera();
	cam->setFOVy(Degree(fov));
}

void CameraBehaviorVehicleInternal::deactivate(cameraContext_t &ctx)
{
}

void CameraBehaviorVehicleInternal::update(cameraContext_t &ctx)
{
}

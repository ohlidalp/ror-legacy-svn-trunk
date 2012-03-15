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
#include "CameraBehaviorVehicleOrbit.h"

#include <Ogre.h>
#include "CameraManager.h"
#include "Console.h"
#include "InputEngine.h"
#include "language.h"
#include "Settings.h"

#include "BeamFactory.h"

using namespace Ogre;

CameraBehaviorVehicleOrbit::CameraBehaviorVehicleOrbit() :
	  externalCameraMode(0)
{
	externalCameraMode = (SSETTING("External Camera Mode", "Pitching") == "Static")? 1 : 0;
}

void CameraBehaviorVehicleOrbit::update(cameraContext_t &ctx)
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if(!curr_truck) return;

	// Make all the changes to the camera
	Vector3 dir = curr_truck->nodes[curr_truck->cameranodepos[0]].smoothpos - curr_truck->nodes[curr_truck->cameranodedir[0]].smoothpos;
	dir.normalise();
	targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));

	if(!externalCameraMode)
		targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	else
		targetPitch = 0;
	{
	camRatio = 1.0f / (curr_truck->tdt * 4.0f);

	CameraBehaviorOrbit::update(ctx);
}

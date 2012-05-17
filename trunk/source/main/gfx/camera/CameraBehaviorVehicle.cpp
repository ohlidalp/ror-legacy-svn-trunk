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
#include "CameraBehaviorVehicle.h"

#include "Beam.h"
#include "Settings.h"

using namespace Ogre;

CameraBehaviorVehicle::CameraBehaviorVehicle() :
	  CameraBehavior()
	, camPitching(true)
{
	camRotY = 0.5f;

	if ( SSETTING("External Camera Mode", "Pitching") == "Static" )
		camPitching = false;
}

void CameraBehaviorVehicle::update(const CameraManager::cameraContext_t &ctx)
{
	Vector3 dir = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[0]].smoothpos
				 - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodedir[0]].smoothpos).normalisedCopy();

	targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	targetPitch     = 0.0f;

	if ( camPitching )
	{
		targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	}
	
	camIntertia = 1.0f / (ctx.mDt * 4.0f);

	camLookAt = ctx.mCurrTruck->getPosition();

	CameraBehavior::update(ctx);
}

void CameraBehaviorVehicle::activate(const CameraManager::cameraContext_t &ctx)
{
	camDistMin = ctx.mCurrTruck->getMinimalCameraRadius() * 2.0f;
	camDist = camDistMin * 1.5f;
}

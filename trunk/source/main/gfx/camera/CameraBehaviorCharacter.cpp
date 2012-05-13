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
#include "CameraBehaviorCharacter.h"

#include "RoRFrameListener.h"

using namespace Ogre;

CameraBehaviorCharacter::CameraBehaviorCharacter() :
	  camMode(THIRD_PERSON)
{
}

void CameraBehaviorCharacter::update(const CameraManager::cameraContext_t &ctx)
{
	Character *person = ctx.mEfl->person;

	targetDirection   = -person->getAngle() - Math::HALF_PI;

	if ( camMode == FIRST_PERSON )
	{
		camRotY = 0.1f;
		camRatio = 0.0f;
		maxCamDist = 0.2f;
		camCenterPosition =  person->getPosition() + Vector3(0.1f, 1.82f, 0.0f);
	} else if ( camMode == THIRD_PERSON )
	{
		camRotY = 0.3f;
		camDist = 5.0f;
		camRatio = 11.0f;
		minCamDist = 3.0f;
		maxCamDist = 6.0f;
		camCenterPosition =  person->getPosition() + Vector3(0.0f, 1.1f, 0.0f);
	}

	CameraBehaviorOrbit::update(ctx);
}

// TODO: First-person mouse interaction

bool CameraBehaviorCharacter::switchBehavior(const CameraManager::cameraContext_t &ctx)
{
	camMode = !camMode;

	return false;
}
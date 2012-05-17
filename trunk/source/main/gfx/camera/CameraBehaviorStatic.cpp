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
#include "CameraBehaviorStatic.h"

#include "Beam.h"
#include "Character.h"
#include "DepthOfFieldEffect.h"
#include "heightfinder.h"
#include "RoRFrameListener.h"

using namespace Ogre;

void CameraBehaviorStatic::update(const CameraManager::cameraContext_t &ctx)
{
	Vector3 lookAt(Vector3::ZERO);
	Vector3 camPosition(0.0f, 0.0f, 5.0f);

	if ( ctx.mCurrTruck )
	{
		lookAt = ctx.mCurrTruck->getPosition();
	} else
	{
		lookAt = ctx.mCharacter->getPosition();
	}

	camPosition.x = ((int)(lookAt.x) / 100) * 100 + 50;
	camPosition.z = ((int)(lookAt.z) / 100) * 100 + 50;

	if ( RoRFrameListener::hfinder )
	{
		camPosition.y += RoRFrameListener::hfinder->getHeightAt(camPosition.x, camPosition.z);
	} else
	{
		camPosition.y += lookAt.y;
	}
	
	float camDist = camPosition.distance(lookAt);
	float fov = atan2(20.0f, camDist);

	ctx.mCamera->setPosition(camPosition);
	ctx.mCamera->lookAt(lookAt);
	ctx.mCamera->setFOVy(Radian(fov));

	if ( ctx.mDof )
	{
		ctx.mDof->setFocusMode(DOFManager::Manual);
		ctx.mDof->setFocus(camDist);
		ctx.mDof->setLensFOV(Radian(fov));
	}
}

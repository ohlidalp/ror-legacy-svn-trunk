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
#include "CameraBehavior.h"

#include "InputEngine.h"
#include "Ogre.h"

using namespace Ogre;

CameraBehavior::CameraBehavior() :
	  camRotX(0.0f)
	, camRotY(0.3f)
	, camDist(5.0f)
	, minCamDist(0.0f)
	, maxCamDist(0.0f)
	, camRatio(11.0f)
	, camIdealPosition(Vector3::ZERO)
	, camCenterPosition(Vector3::ZERO)
	, camTranslation(Vector3::ZERO)
	, targetDirection(0.0f)
	, targetPitch(0.0f)
{
}

void CameraBehavior::update(const CameraManager::cameraContext_t &ctx)
{
	if (INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_LOOKBACK))
	{
		if(camRotX > Degree(0))
			camRotX = Degree(0);
		else
			camRotX = Degree(180);
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_LEFT))
	{
		// Move camera left
		camRotX -= ctx.mRotScale;
	}

	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_RIGHT))
	{
		// Move camera right
		camRotX += ctx.mRotScale;
	}
	if ((INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_UP)) && camRotY<Degree(88))
	{
		// Move camera up
		camRotY += ctx.mRotScale;
	}

	if ((INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_DOWN)) && camRotY>Degree(-80))
	{
		// Move camera down
		camRotY -= ctx.mRotScale;
	}

	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_IN) && camDist>1)
	{
		// Move camera near
		camDist -= ctx.mTransScale;
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_IN_FAST) && camDist>1)
	{
		// Move camera near
		camDist -= ctx.mTransScale * 10;
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_OUT))
	{
		// Move camera far
		camDist += ctx.mTransScale;
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_OUT_FAST))
	{
		// Move camera far
		camDist += ctx.mTransScale * 10;
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_RESET))
	{
		camRotX = 0;
		camRotY = CameraManager::DEFAULT_INTERNAL_CAM_PITCH;
		camDist = 20;
	}

	if ( minCamDist > 0 )
		camDist = std::max(minCamDist, camDist);
	if ( maxCamDist > 0 )
		camDist = std::min(camDist, maxCamDist);

	camIdealPosition = camDist * 0.5f * Vector3( \
			  sin(targetDirection + camRotX.valueRadians()) * cos(targetPitch + camRotY.valueRadians()) \
			, sin(targetPitch     + camRotY.valueRadians()) \
			, cos(targetDirection + camRotX.valueRadians()) * cos(targetPitch + camRotY.valueRadians()) \
			);

	float real_camdist = camIdealPosition.length();

	camIdealPosition = camIdealPosition + camCenterPosition + camTranslation;
	Vector3 newPosition = ( camIdealPosition + camRatio * ctx.mCamera->getPosition() ) / (camRatio+1.0f);

	ctx.mCamera->setPosition(newPosition);
	ctx.mCamera->lookAt(camCenterPosition);
}

bool CameraBehavior::mouseMoved(const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;

	if ( ms.buttonDown(OIS::MB_Right) )
	{
		camRotX += Degree( (float)ms.X.rel * 0.13f);
		camRotY += Degree(-(float)ms.Y.rel * 0.13f);
		camDist +=        -(float)ms.Z.rel * 0.02f;
		return true;
	}

	return false;
}

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
	  camDist(5.0f)
	, camDistMax(0.0f)
	, camDistMin(0.0f)
	, camLookAt(Vector3::ZERO)
	, camIntertia(11.0f)
	, camRotX(0.0f)
	, camRotY(0.3f)
	, targetDirection(0.0f)
	, targetPitch(0.0f)
{
}

void CameraBehavior::update(const CameraManager::cameraContext_t &ctx)
{
	if ( INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_LOOKBACK) )
	{
		if ( camRotX > Degree(0) )
		{
			camRotX = Degree(0);
		} else
		{
			camRotX = Degree(180);
		}
	}

	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_LEFT) )
	{
		camRotX -= ctx.mRotScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_RIGHT) )
	{
		camRotX += ctx.mRotScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_UP) && camRotY < Degree(88) )
	{
		camRotY += ctx.mRotScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_DOWN) && camRotY > Degree(-80) )
	{
		camRotY -= ctx.mRotScale;
	}

	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_IN) && camDist > 1 )
	{
		camDist -= ctx.mTransScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_IN_FAST) && camDist > 1 )
	{
		camDist -= ctx.mTransScale * 10;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_OUT) )
	{
		camDist += ctx.mTransScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_OUT_FAST) )
	{
		camDist += ctx.mTransScale * 10;
	}

	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_RESET) )
	{
		camRotX = 0.0f;
		camRotY = 0.0f;
		camDist = 5.0f;
	}

	if ( camDistMin > 0.0f )
	{
		camDist = std::max(camDistMin, camDist);
	}
	if ( camDistMax > 0.0f )
	{
		camDist = std::min(camDist, camDistMax);
	}

	Vector3 desiredPosition = camLookAt + camDist * 0.5f * Vector3( \
			  sin(targetDirection + camRotX.valueRadians()) * cos(targetPitch + camRotY.valueRadians()) \
			, sin(targetPitch     + camRotY.valueRadians()) \
			, cos(targetDirection + camRotX.valueRadians()) * cos(targetPitch + camRotY.valueRadians()) \
			);

	Vector3 position = (desiredPosition + ctx.mCamera->getPosition() * camIntertia) / (1.0f + camIntertia);

	ctx.mCamera->setPosition(position);
	ctx.mCamera->lookAt(camLookAt);
}

bool CameraBehavior::mouseMoved(const CameraManager::cameraContext_t &ctx, const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;

	if ( ms.buttonDown(OIS::MB_Right) )
	{
		camRotX += Degree( ms.X.rel * 0.13f);
		camRotY += Degree(-ms.Y.rel * 0.13f);
		camDist +=        -ms.Z.rel * 0.02f;
		return true;
	}

	return false;
}

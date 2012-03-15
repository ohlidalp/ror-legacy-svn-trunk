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
#include "CameraBehaviorOrbit.h"

#include <Ogre.h>
#include "CameraManager.h"
#include "Console.h"
#include "InputEngine.h"
#include "language.h"
#include "Settings.h"

using namespace Ogre;

CameraBehaviorOrbit::CameraBehaviorOrbit() :
	  camRotX(0)
	, camRotY(0)
	, camDist(5)
	, minCamDist(3)
	, camRatio(11)
	, camIdealPosition(Vector3::ZERO)
	, camCenterPoint(Vector3::ZERO)
	, camTranslation(Vector3::ZERO)
	, targetDirection(0)
	, targetPitch(0)
{


}

void CameraBehaviorOrbit::activate()
{
	float fov = FSETTING("FOV External", 60);

	DOFManager *dof = CameraManager::getSingleton().getDOFManager();
	if(dof)
	{
		dof->setFocusMode(DOFManager::Manual);
		dof->setLensFOV(Degree(fov));
	}

	camCenterPoint = Vector3(0, 3, 0);

	Camera *cam = CameraManager::getSingleton().getCamera();
	cam->setFOVy(Degree(fov));
}

void CameraBehaviorOrbit::deactivate()
{
}

void CameraBehaviorOrbit::update(cameraContext_t &ctx)
{
	Camera *cam = CameraManager::getSingleton().getCamera();

	/*
	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
		camTranslation.x -= mMoveScale;	// Move camera left

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
		camTranslation.x += mMoveScale;	// Move camera RIGHT

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_FORWARD))
		camTranslation.z -= mMoveScale;	// Move camera forward

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_BACKWARDS))
		camTranslation.z += mMoveScale;	// Move camera backward

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_UP))
		camTranslation.y += mMoveScale;	// Move camera up

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_DOWN))
		camTranslation.y -= mMoveScale;	// Move camera down

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_UP))
		camRotY += mRotScale;

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_DOWN))
		camRotY -= mRotScale;

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_RIGHT))
		camRotX -= mRotScale;

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_LEFT))
		camRotX += mRotScale;
*/

	if (INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_LOOKBACK))
	{
		if(camRotX > Degree(0))
			camRotX=Degree(0);
		else
			camRotX=Degree(180);
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_LEFT))
	{
		// Move camera left
		camRotX -= ctx.rotationScale;
	}

	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_RIGHT))
	{
		// Move camera RIGHT
		camRotX += ctx.rotationScale;
	}
	if ((INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_UP)) && camRotY<Degree(88))
	{
		// Move camera up
		camRotY += ctx.rotationScale;
	}

	if ((INPUTENGINE.getEventBoolValue(EV_CAMERA_ROTATE_DOWN)) && camRotY>Degree(-80))
	{
		// Move camera down
		camRotY -= ctx.rotationScale;
	}

	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_IN) && camDist>1)
	{
		// Move camera near
		camDist -= ctx.translationScale;
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_IN_FAST) && camDist>1)
	{
		// Move camera near
		camDist -= ctx.translationScale * 10;
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_OUT))
	{
		// Move camera far
		camDist += ctx.translationScale;
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_OUT_FAST))
	{
		// Move camera far
		camDist += ctx.translationScale * 10;
	}
	if (INPUTENGINE.getEventBoolValue(EV_CAMERA_RESET))
	{
		camRotX=0;
		//if (cameramode!=CAMERA_INT)
		//	camRotY=Degree(12);
		//else
			camRotY = DEFAULT_INTERNAL_CAM_PITCH;
		camDist=20;
	}



	// set Minimal Cam distance
	if(camDist < minCamDist) camDist = minCamDist;

	camIdealPosition = camDist * 0.5f * Vector3( \
			  sin(targetDirection + camRotX.valueRadians()) * cos(targetPitch + camRotY.valueRadians()) \
			, sin(targetPitch     + camRotY.valueRadians()) \
			, cos(targetDirection + camRotX.valueRadians()) * cos(targetPitch + camRotY.valueRadians()) \
			);

	float real_camdist = camIdealPosition.length();

	camIdealPosition = camIdealPosition + camCenterPoint + camTranslation;
	Vector3 newposition = ( camIdealPosition + camRatio * cam->getPosition() ) / (camRatio+1.0f);

	/*
	Real h=hfinder->getHeightAt(newposition.x,newposition.z);

	if (w && !w->allowUnderWater() && w->getHeightWaves(newposition) > h)
		h=w->getHeightWaves(newposition);

	h+=1.0;
	if (newposition.y<h) newposition.y=h;
	*/

	cam->setPosition(newposition);
	cam->lookAt(camCenterPoint);

	lastPosition = camCenterPoint;

	DOFManager *dof = CameraManager::getSingleton().getDOFManager();
	if(dof)
	{
		dof->setFocusMode(DOFManager::Manual);
		dof->setFocus(real_camdist);
	}
}

bool CameraBehaviorOrbit::mouseMoved(const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;
	Camera *cam = CameraManager::getSingleton().getCamera();

	if(ms.buttonDown(OIS::MB_Right))
	{
		camRotX += Degree( (float)ms.X.rel * 0.13f);
		camRotY += Degree(-(float)ms.Y.rel * 0.13f);
		camDist += -(float)ms.Z.rel * 0.02f;
		return true;
	}
	return false;
}

bool CameraBehaviorOrbit::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return false;
}

bool CameraBehaviorOrbit::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return false;
}

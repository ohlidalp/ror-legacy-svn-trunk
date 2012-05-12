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
#include "CameraBehaviorFree.h"

#include "CameraManager.h"
#include "Console.h"
#include "DepthOfFieldEffect.h"
#include "InputEngine.h"
#include "Settings.h"
#include "language.h"
#include <Ogre.h>

using namespace Ogre;

void CameraBehaviorFree::activate(CameraManager::cameraContext_t &ctx)
{
	DOFManager *dof = CameraManager::getSingleton().getDOFManager();
	if ( dof )
	{
		dof->setFocusMode(DOFManager::Auto);
	}

	ctx.cam->setFixedYawAxis(true, Vector3::UNIT_Y);

	LOG("entering free camera mode");

	CONSOLE_PUTMESSAGE(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("free camera"), "camera_go.png", 3000, false);

#ifdef USE_MYGUI
	MyGUI::PointerManager::getInstance().setVisible(false);
#endif // USE_MYGUI
}

void CameraBehaviorFree::deactivate(CameraManager::cameraContext_t &ctx)
{
	DOFManager *dof = CameraManager::getSingleton().getDOFManager();
	if ( dof )
	{
		dof->setFocusMode(DOFManager::Manual);
	}

	LOG("exiting free camera mode");

	CONSOLE_PUTMESSAGE(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("normal camera"), "camera.png", 3000, false);

#ifdef USE_MYGUI
	MyGUI::PointerManager::getInstance().setVisible(true);
#endif // USE_MYGUI
}

void CameraBehaviorFree::update(CameraManager::cameraContext_t &ctx)
{
	Vector3 mTranslateVector = Vector3::ZERO;
	Degree mRotX(0.0f);
	Degree mRotY(0.0f);

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
		mTranslateVector.x -= ctx.translationScale;	// Move camera left

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
		mTranslateVector.x += ctx.translationScale;	// Move camera RIGHT

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_FORWARD))
		mTranslateVector.z -= ctx.translationScale;	// Move camera forward

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_BACKWARDS))
		mTranslateVector.z += ctx.translationScale;	// Move camera backward

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_UP))
		mRotY += ctx.rotationScale;

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_DOWN))
		mRotY -= ctx.rotationScale;

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_UP))
		mTranslateVector.y += ctx.translationScale;	// Move camera up

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_DOWN))
		mTranslateVector.y -= ctx.translationScale;	// Move camera down

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_RIGHT))
		mRotX -= ctx.rotationScale;

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_LEFT))
		mRotX += ctx.rotationScale;

	ctx.cam->yaw(mRotX);
	ctx.cam->pitch(mRotY);

	ctx.cam->setFixedYawAxis(false);

	Vector3 trans = ctx.cam->getOrientation() * mTranslateVector;
	ctx.cam->setPosition(ctx.cam->getPosition() + trans);
}

bool CameraBehaviorFree::mouseMoved(const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;
	Camera *cam = CameraManager::getSingleton().getCamera();

	cam->yaw(Degree(-(float)ms.X.rel * 0.13f));
	cam->pitch(Degree(-(float)ms.Y.rel * 0.13f));

	return true;
}

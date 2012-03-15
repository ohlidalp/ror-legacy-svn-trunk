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

#include <Ogre.h>
#include "CameraManager.h"
#include "Console.h"
#include "InputEngine.h"
#include "language.h"

using namespace Ogre;


void CameraBehaviorFree::activate()
{
	// enter free camera mode
	if(mDOF) mDOF->setFocusMode(DOFManager::Auto);
	storedcameramode = cameramode;
	cameramode = CAMERA_FREE;
	LOG("entering free camera mode");
#ifdef USE_MYGUI
	Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("free camera"), "camera_go.png", 3000);
#endif // USE_MYGUI
}

void CameraBehaviorFree::deactivate()
{
	// change back to normal camera
	if(mDOF) mDOF->setFocusMode(DOFManager::Manual);
	cameramode = storedcameramode;
	LOG("exiting free camera mode");
#ifdef USE_MYGUI
	Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("normal camera"), "camera.png", 3000);
#endif // USE_MYGUI
}

void CameraBehaviorFree::update(float dt)
{
	// this is a workaround for the free camera mode :)
	Real mMoveScale = 0.1;
	Ogre::Degree mRotScale(0.1f);
	Ogre::Degree mRotX(0);
	Ogre::Degree mRotY(0);
	Vector3 mTranslateVector = Vector3::ZERO;

	if(INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) || INPUTENGINE.isKeyDown(OIS::KC_RSHIFT))
	{
		mRotScale *= 3;
		mMoveScale *= 3;
	}

	if(INPUTENGINE.isKeyDown(OIS::KC_LCONTROL))
	{
		mRotScale *= 30;
		mMoveScale *= 30;
	}

	if(INPUTENGINE.isKeyDown(OIS::KC_LMENU))
	{
		mRotScale *= 0.05;
		mMoveScale *= 0.05;
	}


	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
		mTranslateVector.x = -mMoveScale;	// Move camera left

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
		mTranslateVector.x = mMoveScale;	// Move camera RIGHT

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_FORWARD))
		mTranslateVector.z = -mMoveScale;	// Move camera forward

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_BACKWARDS))
		mTranslateVector.z = mMoveScale;	// Move camera backward

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_UP))
		mRotY += mRotScale;

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_DOWN))
		mRotY += -mRotScale;

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_UP))
		mTranslateVector.y = mMoveScale;	// Move camera up

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_DOWN))
		mTranslateVector.y = -mMoveScale;	// Move camera down

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_RIGHT))
		mRotX += -mRotScale;

	if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_LEFT))
		mRotX += mRotScale;

	mCamera->yaw(mRotX);
	mCamera->pitch(mRotY);

	Vector3 trans = mCamera->getOrientation() * mTranslateVector;
	setCameraPositionWithCollision(mCamera->getPosition() + trans);
}

bool CameraBehaviorFree::mouseMoved(const OIS::MouseEvent& _arg)
{
	return false;
}

bool CameraBehaviorFree::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return false;
}

bool CameraBehaviorFree::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return false;
}

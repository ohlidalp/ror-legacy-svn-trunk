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

#include "InputEngine.h"
#include "mygui/BaseLayout.h"
#include "Ogre.h"

using namespace Ogre;

void CameraBehaviorFree::update(const CameraManager::cameraContext_t &ctx)
{
	Vector3 mTrans = Vector3::ZERO;
	Degree mRotX(0.0f);
	Degree mRotY(0.0f);

	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT) )
	{
		mTrans.x -= ctx.mTransScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT) )
	{
		mTrans.x += ctx.mTransScale;
	}

	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_FORWARD) )
	{
		mTrans.z -= ctx.mTransScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_BACKWARDS) )
	{
		mTrans.z += ctx.mTransScale;
	}

	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_UP) )
	{
		mRotY += ctx.mRotScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_DOWN) )
	{
		mRotY -= ctx.mRotScale;
	}

	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_UP) )
	{
		mTrans.y += ctx.mTransScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_DOWN) )
	{
		mTrans.y -= ctx.mTransScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_RIGHT) )
	{
		mRotX -= ctx.mRotScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CHARACTER_LEFT) )
	{
		mRotX += ctx.mRotScale;
	}

	ctx.mCamera->yaw(mRotX);
	ctx.mCamera->pitch(mRotY);

	ctx.mCamera->setPosition(ctx.mCamera->getPosition() + ctx.mCamera->getOrientation() * mTrans);
}

void CameraBehaviorFree::activate(const CameraManager::cameraContext_t &ctx)
{
	ctx.mCamera->setFixedYawAxis(true, Vector3::UNIT_Y);
#ifdef USE_MYGUI
	MyGUI::PointerManager::getInstance().setVisible(false);
#endif // USE_MYGUI
}

void CameraBehaviorFree::deactivate(const CameraManager::cameraContext_t &ctx)
{
	ctx.mCamera->setFixedYawAxis(false);
#ifdef USE_MYGUI
	MyGUI::PointerManager::getInstance().setVisible(true);
#endif // USE_MYGUI
}

bool CameraBehaviorFree::mouseMoved(const CameraManager::cameraContext_t &ctx, const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;

	ctx.mCamera->yaw(Degree(-ms.X.rel * 0.13f));
	ctx.mCamera->pitch(Degree(-ms.Y.rel * 0.13f));

	return true;
}

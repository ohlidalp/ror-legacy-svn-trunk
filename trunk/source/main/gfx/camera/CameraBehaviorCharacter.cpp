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

#include "Character.h"
#include "mygui/BaseLayout.h"

using namespace Ogre;

CameraBehaviorCharacter::CameraBehaviorCharacter() :
	  CameraBehavior()
	, camMode(CHARACTER_THIRD_PERSON)
{
	camPositionOffset = Vector3(0.0f, 1.1f, 0.0f);
}

void CameraBehaviorCharacter::update(const CameraManager::cameraContext_t &ctx)
{
	targetDirection = -ctx.mCharacter->getAngle() - Math::HALF_PI;
	camLookAt       =  ctx.mCharacter->getPosition() + camPositionOffset;

	CameraBehavior::update(ctx);
}

bool CameraBehaviorCharacter::mouseMoved(const CameraManager::cameraContext_t &ctx, const OIS::MouseEvent& _arg)
{
	if ( camMode == CHARACTER_FIRST_PERSON )
	{
		const OIS::MouseState ms = _arg.state;
		float angle = ctx.mCharacter->getAngle();
		
		camRotY += Degree(ms.Y.rel * 0.13f);
		angle   +=        ms.X.rel * 0.13f;

		ctx.mCharacter->setAngle(angle);

		return true;
	}

	return CameraBehavior::mouseMoved(ctx, _arg);
}

void CameraBehaviorCharacter::reset(const CameraManager::cameraContext_t &ctx)
{
	camRotX =  0.0f;

	if ( camMode == CHARACTER_FIRST_PERSON )
	{
		camRotY = 0.1f;
		camDist = 0.1f;
		camIntertia = 0.0f;
		camPositionOffset = Vector3(0.0f, 1.82f, 0.0f);
#ifdef USE_MYGUI
		MyGUI::PointerManager::getInstance().setVisible(false);
#endif // USE_MYGUI
	} else if ( camMode == CHARACTER_THIRD_PERSON )
	{
		camRotY = 0.3f;
		camDist = 5.0f;
		camIntertia = 11.0f;
		camPositionOffset = Vector3(0.0f, 1.1f, 0.0f);
#ifdef USE_MYGUI
		MyGUI::PointerManager::getInstance().setVisible(true);
#endif // USE_MYGUI
	}
}

bool CameraBehaviorCharacter::switchBehavior(const CameraManager::cameraContext_t &ctx)
{
	camMode = (camMode + 1) % CHARACTER_END;

	reset(ctx);

	return false;
}

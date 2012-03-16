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
#include "CameraBehaviorWheelChase.h"

#include <Ogre.h>
#include "CameraManager.h"
#include "Console.h"
#include "InputEngine.h"
#include "language.h"

#include "BeamFactory.h"

using namespace Ogre;

void CameraBehaviorWheelChase::activate(cameraContext_t &ctx)
{
}

void CameraBehaviorWheelChase::deactivate(cameraContext_t &ctx)
{
}

void CameraBehaviorWheelChase::update(cameraContext_t &ctx)
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if(!curr_truck) return;

	//ctx.cam->setFixedYawAxis(false);

	int i = 0;

	Vector3 axis  = curr_truck->wheels[i].refnode1->smoothpos - curr_truck->wheels[i].refnode0->smoothpos;
	Vector3 cpos  = curr_truck->wheels[i].refnode0->smoothpos - axis * 3;
	Vector3 clook = curr_truck->wheels[i].refnode1->smoothpos;

	//ctx.cam->setFixedYawAxis(false);

	ctx.cam->lookAt(clook);

	// TODO: FIX
	//ctx.cam->roll(Ogre::Degree(-curr_truck->wheels[i].speed));

	ctx.cam->setPosition(cpos);
}

bool CameraBehaviorWheelChase::mouseMoved(const OIS::MouseEvent& _arg)
{
	return false;
}

bool CameraBehaviorWheelChase::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return false;
}

bool CameraBehaviorWheelChase::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return false;
}

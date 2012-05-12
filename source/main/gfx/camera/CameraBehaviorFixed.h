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
#ifndef __CAMERA_BEHAVIOR_FIXEd_H_
#define __CAMERA_BEHAVIOR_FIXEd_H_

#include "RoRPrerequisites.h"
#include "CameraBehavior.h"

class CameraBehaviorFixed : public CameraBehavior
{
public:

	void activate(CameraManager::cameraContext_t &ctx);
	void deactivate(CameraManager::cameraContext_t &ctx);

	void update(CameraManager::cameraContext_t &ctx);

	bool mouseMoved(const OIS::MouseEvent& _arg) { return false; };
	bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id) { return false; };
	bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id) { return false; };

	bool allowInteraction() { return false; };
	bool switchBehavior() { return true; }
};

#endif // __CAMERA_BEHAVIOR_FIXEd_H_

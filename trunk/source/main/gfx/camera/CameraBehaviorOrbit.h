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
#ifndef __CAMERA_BEHAVIOR_ORBIT_H_
#define __CAMERA_BEHAVIOR_ORBIT_H_

#include "RoRPrerequisites.h"

#include "CameraBehavior.h"

class CameraBehaviorOrbit : public CameraBehavior
{
public:

	CameraBehaviorOrbit();

	void update(CameraManager::cameraContext &ctx);

	bool mouseMoved(const OIS::MouseEvent& _arg);
	bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id) { return false; };
	bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id) { return false; };

	void activate(CameraManager::cameraContext &ctx);
	void deactivate(CameraManager::cameraContext &ctx);

	bool allowInteraction() { return false; };

protected:

	Ogre::Radian camRotX, camRotY;
	Ogre::Vector3 camIdealPosition, camCenterPosition, camTranslation;
	float camDist, minCamDist, camRatio;
	float targetDirection, targetPitch;
};

#endif // __CAMERA_BEHAVIOR_ORBIT_H_

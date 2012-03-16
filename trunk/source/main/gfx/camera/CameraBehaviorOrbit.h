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
#ifndef CAMERABEHAVIORORBIT_H__
#define CAMERABEHAVIORORBIT_H__

#include "RoRPrerequisites.h"
#include "CameraBehavior.h"

class CameraBehaviorOrbit : public CameraBehavior
{
protected:
	Ogre::Radian camRotX, camRotY;
	float camDist, minCamDist, camRatio;
	Ogre::Vector3 camIdealPosition, camCenterPoint, lastPosition, camTranslation;
	float targetDirection, targetPitch;

public:

	CameraBehaviorOrbit();

	void activate(cameraContext_t &ctx);
	void deactivate(cameraContext_t &ctx);

	void update(cameraContext_t &ctx);

	bool mouseMoved(const OIS::MouseEvent& _arg);
	bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
	bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);

	bool allowInteraction() { return false; };
};

#endif // CAMERABEHAVIORORBIT_H__



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
#ifndef __CAMERA_BEHAVIOR_VEHICLE_SPLINE_H_
#define __CAMERA_BEHAVIOR_VEHICLE_SPLINE_H_

#include "RoRPrerequisites.h"

#include "CameraBehaviorVehicle.h"

class CameraBehaviorVehicleSpline : public CameraBehaviorVehicle
{
public:

	CameraBehaviorVehicleSpline();

	void update(const CameraManager::cameraContext_t &ctx);
	
	bool mouseMoved(const CameraManager::cameraContext_t &ctx, const OIS::MouseEvent& _arg);

	void activate(const CameraManager::cameraContext_t &ctx, bool reset = true);
	void reset(const CameraManager::cameraContext_t &ctx);

	void updateSpline();
	void updateSplineDisplay();

protected:

	Ogre::ManualObject* myManualObject;
	Ogre::SceneNode* mySceneNode;
	Ogre::SimpleSpline* spline;
	Ogre::Real splineLength;
	Ogre::Real splinePos;
	bool splineClosed;

	std::vector<node*> splineNodes;

	static const int splineDrawResolution = 200;
};

#endif // __CAMERA_BEHAVIOR_VEHICLE_SPLINE_H_

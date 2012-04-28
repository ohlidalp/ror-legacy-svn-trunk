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
#ifndef CAMERAMANAGER_H__
#define CAMERAMANAGER_H__

#include "RoRPrerequisites.h"

#include "DepthOfFieldEffect.h"
#include "OIS.h"
#include "Ogre.h"
#include "Singleton.h"

#include "CameraBehavior.h"

#define MAIN_CAMERA CameraManager::getSingleton().getCamera()
#define CAMERA_MODE CameraManager::getSingleton().getCameraMode()
#define DEFAULT_INTERNAL_CAM_PITCH Ogre::Degree(-15)

class CameraManager : public RoRSingletonNoCreation < CameraManager >
{
	friend class SceneMouse;

protected:

	DOFManager *mDOF;
	Ogre::Camera *mCamera;
	Ogre::Radian pushcamRotX, pushcamRotY;
	Ogre::SceneManager *mSceneMgr;
	Ogre::Vector3 mLastPosition;
	cameraContext_t ctx;
	float mMoveScale, mRotScale;
	float mMoveSpeed, mRotateSpeed;

	CameraBehavior *currentBehavior;
	int currentBehaviorID;

	std::map <int , CameraBehavior *> globalBehaviors;

	void switchBehavior(int newBehavior);
	void switchToNextBehavior();
	void createGlobalBehaviors();

	bool mouseMoved(const OIS::MouseEvent& _arg);
	bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
	bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);

public:

	CameraManager(Ogre::SceneManager *scm, Ogre::Camera *cam);
	~CameraManager();

	enum
	{
		CAMERA_CHARACTER_ORBIT=0
	  , CAMERA_VEHICLE_INTERNAL
	  , CAMERA_VEHICLE_ORBIT
	  , CAMERA_VEHICLE_WHEELCHASE
	  , CAMERA_VEHICLE_SPLINE
	  , CAMERA_END
	  , CAMERA_FREE
	  , CAMERA_FIX
	};
	
	void triggerFOVUpdate() { /*TODO: Think about this*/ };
	void update(float dt);

	bool allowInteraction();

	Ogre::Camera *getCamera() { return mCamera; };
	int getCameraMode() { return mCamera->getPolygonMode(); };
	inline DOFManager *getDOFManager() { return mDOF; }
};

#endif // CAMERAMANAGER_H__

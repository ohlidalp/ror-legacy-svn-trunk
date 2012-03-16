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
#include "Singleton.h"

#include <map>

#include "CameraBehavior.h"

#define MAIN_CAMERA CameraManager::getSingleton().getCamera()
#define CAMERA_MODE CameraManager::getSingleton().getCameraMode()
#define DEFAULT_INTERNAL_CAM_PITCH Degree(-15)

class CameraManager : public RoRSingletonNoCreation < CameraManager >
{
	friend class SceneMouse;
protected:
	Ogre::SceneManager *mSceneMgr;
	Ogre::Camera *mCamera;
	int cameramode, lastcameramode;
	bool camCollided;
	Ogre::Vector3 camPosColl;
	Ogre::Radian pushcamRotX, pushcamRotY;
	float mMoveScale, mRotScale;
	Ogre::Vector3 lastPosition;
	int mSceneDetailIndex;
	//    Real dirSpeed;
	float mMoveSpeed, mRotateSpeed;
	DOFManager *mDOF;
	bool enforceCameraFOVUpdate;
	Ogre::Vector3 cdoppler;
	cameraContext_t ctx;

	CameraBehavior *currentBehavior;

	enum {   CAMBEHAVIOR_FREE
		   , CAMBEHAVIOR_CHARACTER_ORBIT
		   , CAMBEHAVIOR_VEHICLE_ORBIT
		   , CAMBEHAVIOR_VEHICLE_WHEELCHASE
		   , CAMBEHAVIOR_END
	};

	std::map <int , CameraBehavior *> globalBehaviors;

	bool mouseMoved(const OIS::MouseEvent& _arg);
	bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
	bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);

public:
	CameraManager(Ogre::SceneManager *scm, Ogre::Camera *cam);
	~CameraManager();

	void updateInput();
	void switchBehavior(int newBehavior);

	enum { CAMERA_EXT=0,
		CAMERA_FIX,
		CAMERA_INT,
		CAMERA_END,
		CAMERA_FREE,
		CAMERA_FREE_FIXED,
		CAMERA_EXTERNALCONTROL=9999
	};

	//void setCameraRotation(Ogre::Radian x, Ogre::Radian y, Ogre::Real distance) { camRotX=x; camRotY=y; camDist=distance;};
	//Ogre::Radian getCameraRotationX() { return camRotX; };

	void update(float dt);
	bool setCameraPositionWithCollision(Ogre::Vector3 newPos);
	Ogre::Camera *getCamera() { return mCamera; };
	int getCameraMode() { return cameramode; };
	inline DOFManager *getDOFManager() { return mDOF; }



	void createGlobalBehaviors();
	void triggerFOVUpdate();

	bool allowInteraction();
};

#endif // CAMERAMANAGER_H__



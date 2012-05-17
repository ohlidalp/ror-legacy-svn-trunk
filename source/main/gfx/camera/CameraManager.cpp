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
#include "CameraManager.h"

#include "BeamFactory.h"
#include "InputEngine.h"

#include "CameraBehavior.h"
#include "CameraBehaviorCharacter.h"
#include "CameraBehaviorFixed.h"
#include "CameraBehaviorFree.h"
#include "CameraBehaviorStatic.h"
#include "CameraBehaviorVehicle.h"
#include "CameraBehaviorVehicleCineCam.h"
#include "CameraBehaviorVehicleSpline.h"

#include "ICameraBehavior.h"

using namespace Ogre;

CameraManager::CameraManager(SceneManager *scm, Camera *cam, RoRFrameListener *efl, Character *ps, OverlayWrapper *ow, DOFManager *dof) : 
	  currentBehavior(0)
	, currentBehaviorID(-1)
	, mTransScale(1.0f)
	, mTransSpeed(50.0f)
	, mRotScale(0.1f)
	, mRotateSpeed(100.0f)
{
	setSingleton(this);

	createGlobalBehaviors();

	ctx.mCamera = cam;
	ctx.mCharacter = ps;
	ctx.mCurrTruck = 0;
	ctx.mDof = dof;
	ctx.mEfl = efl;
	ctx.mOverlayWrapper = ow;
	ctx.mSceneMgr = scm;
}

CameraManager::~CameraManager()
{
	for (std::map <int , ICameraBehavior *>::iterator it = globalBehaviors.begin(); it != globalBehaviors.end(); ++it)
	{
		delete it->second;
	}

	globalBehaviors.clear();
}

void CameraManager::createGlobalBehaviors()
{
	globalBehaviors.insert(std::pair<int, ICameraBehavior*>(CAMERA_BEHAVIOR_CHARACTER, new CameraBehaviorCharacter()));
	globalBehaviors.insert(std::pair<int, ICameraBehavior*>(CAMERA_BEHAVIOR_VEHICLE, new CameraBehaviorVehicle()));
	globalBehaviors.insert(std::pair<int, ICameraBehavior*>(CAMERA_BEHAVIOR_STATIC, new CameraBehaviorStatic()));
	globalBehaviors.insert(std::pair<int, ICameraBehavior*>(CAMERA_BEHAVIOR_VEHICLE_SPLINE, new CameraBehaviorVehicleSpline()));
	globalBehaviors.insert(std::pair<int, ICameraBehavior*>(CAMERA_BEHAVIOR_VEHICLE_CINECAM, new CameraBehaviorVehicleCineCam()));
	globalBehaviors.insert(std::pair<int, ICameraBehavior*>(CAMERA_BEHAVIOR_FREE, new CameraBehaviorFree()));
	globalBehaviors.insert(std::pair<int, ICameraBehavior*>(CAMERA_BEHAVIOR_FIXED, new CameraBehaviorFixed()));
}

void CameraManager::switchToNextBehavior()
{
	if ( !currentBehavior || currentBehavior->switchBehavior(ctx) )
	{
		int i = (currentBehaviorID + 1) % CAMERA_BEHAVIOR_END;
		switchBehavior(i);
	}
}

void CameraManager::switchBehavior(int newBehaviorID)
{
	if (newBehaviorID == currentBehaviorID)
	{
		return;
	}

	if ( globalBehaviors.find(newBehaviorID) == globalBehaviors.end() )
	{
		return;
	}

	// deactivate old
	if ( currentBehavior )
	{
		currentBehavior->deactivate(ctx);
	}

	// set new
	currentBehavior = globalBehaviors[newBehaviorID];
	currentBehaviorID = newBehaviorID;

	// activate new
	currentBehavior->activate(ctx);
}

void CameraManager::update(float dt)
{
	if ( dt == 0 ) return;

	mTransScale = mTransSpeed  * dt;
	mRotScale   = mRotateSpeed * dt;

	ctx.mCurrTruck  = BeamFactory::getSingleton().getCurrentTruck();
	ctx.mDt         = dt;
	ctx.mRotScale   = Degree(mRotScale);
	ctx.mTransScale = mTransScale;

	if ( INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_CHANGE) )
	{
		switchToNextBehavior();
	}

	if ( INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_FREE_MODE_FIX) )
	{
		switchBehavior(CAMERA_BEHAVIOR_FIXED);
	}

	if ( !ctx.mCurrTruck && INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_FREE_MODE) )
	{
		switchBehavior(CAMERA_BEHAVIOR_FREE);
	}

	if ( !ctx.mCurrTruck && dynamic_cast<CameraBehaviorVehicle*>(currentBehavior) )
	{
		switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
	} else if ( ctx.mCurrTruck && !dynamic_cast<CameraBehaviorVehicle*>(currentBehavior) )
	{
		if ( currentBehaviorID != CAMERA_BEHAVIOR_STATIC && currentBehaviorID != CAMERA_BEHAVIOR_FIXED )
		{
			switchBehavior(CAMERA_BEHAVIOR_VEHICLE);
		}
	}

	if ( currentBehavior )
	{
		currentBehavior->update(ctx);
	} else
	{
		switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
	}
}

bool CameraManager::mouseMoved(const OIS::MouseEvent& _arg)
{
	if ( !currentBehavior ) return false;
	return currentBehavior->mouseMoved(ctx, _arg);
}

bool CameraManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	if ( !currentBehavior ) return false;
	return currentBehavior->mousePressed(ctx, _arg, _id);
}

bool CameraManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	if ( !currentBehavior ) return false;
	return currentBehavior->mouseReleased(ctx, _arg, _id);
}

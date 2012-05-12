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
#include "CameraBehaviorVehicleCineCam.h"

#include "BeamFactory.h"
#include "CameraManager.h"
#include "DepthOfFieldEffect.h"
#include "Settings.h"

using namespace Ogre;

void CameraBehaviorVehicleCineCam::activate(CameraManager::cameraContext_t &ctx)
{
	float fov = FSETTING("FOV Internal", 75);

	DOFManager *dof = CameraManager::getSingleton().getDOFManager();
	if ( dof )
	{
		dof->setFocusMode(DOFManager::Manual);
		dof->setLensFOV(Degree(fov));
	}

	Camera *cam = CameraManager::getSingleton().getCamera();
	cam->setFOVy(Degree(fov));
}

void CameraBehaviorVehicleCineCam::deactivate(CameraManager::cameraContext_t &ctx)
{
}

bool CameraBehaviorVehicleCineCam::switchBehavior()
{

	return true;
}


void CameraBehaviorVehicleCineCam::update(CameraManager::cameraContext_t &ctx)
{
#if 0
			int currentcamera=curr_truck->currentcamera;

			float fov = FSETTING("FOV Internal", 75);

			if (changeCamMode)
				mCamera->setFOVy(Degree(fov));

			if (curr_truck->cinecameranodepos>=0) lastPosition=curr_truck->nodes[curr_truck->cinecameranodepos[currentcamera]].smoothpos;
			else lastPosition=curr_truck->nodes[curr_truck->cameranodepos[currentcamera]].smoothpos;
			mCamera->setPosition(lastPosition);
			if (curr_truck->cablightNode)
				curr_truck->cablightNode->setPosition(lastPosition);
			//old direction code
			/*
			Vector3 dir=curr_truck->nodes[curr_truck->cameranodepos[currentcamera]].smoothpos-curr_truck->nodes[curr_truck->cameranoderoll[currentcamera]].smoothpos;
			dir.normalise();
			mCamera->lookAt(lastPosition+curr_truck->nodes[curr_truck->cameranodepos[currentcamera]].smoothpos-curr_truck->nodes[curr_truck->cameranodedir[currentcamera]].smoothpos);
			mCamera->roll(Radian(asin(dir.dotProduct(Vector3::UNIT_Y))));
			mCamera->yaw(-camRotX);
			mCamera->pitch(camRotY);
			*/
			Vector3 dir=curr_truck->nodes[curr_truck->cameranodepos[currentcamera]].smoothpos-curr_truck->nodes[curr_truck->cameranodedir[currentcamera]].smoothpos;
			dir.normalise();
			Vector3 side=curr_truck->nodes[curr_truck->cameranodepos[currentcamera]].smoothpos-curr_truck->nodes[curr_truck->cameranoderoll[currentcamera]].smoothpos;
			side.normalise();
			if (curr_truck->revroll[currentcamera]) side=-side; //to fix broken vehicles
			Vector3 up=dir.crossProduct(side);
			//we recompute the side vector to be sure we make an orthonormal system
			side=up.crossProduct(dir);
			Quaternion cdir=Quaternion(camRotX, up)*Quaternion(Degree(180.0)+camRotY, side)*Quaternion(side, up, dir);
			mCamera->setOrientation(cdir);
#endif
}

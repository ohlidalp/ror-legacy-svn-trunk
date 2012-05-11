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
#include "Console.h"
#include "envmap.h"
#include "heightfinder.h"
#include "InputEngine.h"
#include "language.h"
#include "Ogre.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "SoundScriptManager.h"
#include "SkyManager.h"
#include "water.h"

#include "CameraBehaviorCharacter.h"
#include "CameraBehaviorFixed.h"
#include "CameraBehaviorFree.h"
#include "CameraBehaviorOrbit.h"
#include "CameraBehaviorVehicleCineCam.h"
#include "CameraBehaviorVehicleOrbit.h"
#include "CameraBehaviorVehicleSpline.h"
#include "OverlayWrapper.h"

using namespace Ogre;

CameraManager::CameraManager(SceneManager *scm, Camera *cam, RoRFrameListener *efl, HeightFinder *hf) : 
	  currentBehavior(0)
	, currentBehaviorID(-1)
	, mCamera(cam)
	, mDOF(0)
	, mEfl(efl)
	, mHfinder(hf)
	, mLastPosition(Vector3::ZERO)
	, mMoveScale(1.0f)
	, mMoveSpeed(50.0f)
	, mRotScale(0.1f)
	, mRotateSpeed(100.0f)
	, mSceneMgr(scm)
{
	setSingleton(this);

	if ( BSETTING("DOF", false) )
	{
		mDOF = new DOFManager(mSceneMgr, mCamera->getViewport(), Root::getSingletonPtr(), mCamera);
		mDOF->setEnabled(true);
	}

	createGlobalBehaviors();
	ctx.cam = mCamera;
	ctx.scm = mSceneMgr;
	
	switchBehavior(CAMERA_CHARACTER);
	switchBehavior(CAMERA_VEHICLE_ORBIT);
	//switchBehavior(CAMERA_VEHICLE_SPLINE);
}

CameraManager::~CameraManager()
{

}

void CameraManager::createGlobalBehaviors()
{
	globalBehaviors.insert( std::pair<int, CameraBehavior*>(CAMERA_CHARACTER, new CameraBehaviorCharacter()) );
	globalBehaviors.insert( std::pair<int, CameraBehavior*>(CAMERA_VEHICLE_INTERNAL, new CameraBehaviorVehicleCineCam()) );
	globalBehaviors.insert( std::pair<int, CameraBehavior*>(CAMERA_VEHICLE_ORBIT, new CameraBehaviorVehicleOrbit()) );
	globalBehaviors.insert( std::pair<int, CameraBehavior*>(CAMERA_VEHICLE_SPLINE, new CameraBehaviorVehicleSpline()) );
	globalBehaviors.insert( std::pair<int, CameraBehavior*>(CAMERA_FREE, new CameraBehaviorFree()) );
	globalBehaviors.insert( std::pair<int, CameraBehavior*>(CAMERA_FIXED, new CameraBehaviorFixed()) );
}

void CameraManager::switchToNextBehavior()
{
	int i = (currentBehaviorID + 1) % CAMERA_END;
	switchBehavior(i);
}

void CameraManager::switchBehavior(int newBehavior)
{
	if ( globalBehaviors.find(newBehavior) == globalBehaviors.end() )
	{
		return;
	}

	// deactivate old
	if ( currentBehavior )
	{
		currentBehavior->deactivate(ctx);
	}

	// set new
	currentBehavior = globalBehaviors[newBehavior];
	currentBehaviorID = newBehavior;

	// activate new
	currentBehavior->activate(ctx);
}

void CameraManager::update(float dt)
{
	if (dt == 0) return;

	mMoveScale = mMoveSpeed   * dt;
	mRotScale  = mRotateSpeed * dt;

	if(INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_CHANGE))
	{
		switchToNextBehavior();
	}
#ifdef MYGUI
	if (SceneMouse::getSingleton().isMouseGrabbed()) return; // freeze camera
#endif //MYGUI

	ctx.dt               = dt;
	ctx.translationScale = mMoveScale;
	ctx.rotationScale    = Degree(mRotScale);

	if(!currentBehavior->allowInteraction())
	{
		if(INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) || INPUTENGINE.isKeyDown(OIS::KC_RSHIFT))
		{
			ctx.rotationScale *= 3;
			ctx.translationScale *= 3;
		}

		if(INPUTENGINE.isKeyDown(OIS::KC_LCONTROL))
		{
			ctx.rotationScale *= 30;
			ctx.translationScale *= 30;
		}
		if(INPUTENGINE.isKeyDown(OIS::KC_LMENU))
		{
			ctx.rotationScale *= 0.05;
			ctx.translationScale *= 0.05;
		}
	}

	// update current behavior
	if(currentBehavior)
	{
		currentBehavior->update(ctx);
	}

#ifdef USE_OPENAL
	// update audio listener position
	Vector3 cameraSpeed = (mCamera->getPosition() - mLastPosition) / dt;
	mLastPosition = mCamera->getPosition();

	SoundScriptManager::getSingleton().setCamera(mCamera->getPosition(), mCamera->getDirection(), mCamera->getUp(), cameraSpeed);
#endif // USE_OPENAL

#if 0
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

	bool changeCamMode = (lastcameramode != cameramode) || enforceCameraFOVUpdate;
	enforceCameraFOVUpdate = false;
	lastcameramode = cameramode;

	if (!curr_truck)
	{
		//perso mode
		if (cameramode==CAMERA_EXT)
		{
			if (collisions->forcecam)
			{
				mCamera->setPosition(collisions->forcecampos);
				mCamera->lookAt(person->getPosition()+Vector3(0.0,1.0,0.0));
				float fov = FSETTING("FOV External", 60);
				if(changeCamMode)
					mCamera->setFOVy(Degree(fov+20));
				collisions->forcecam=false;
				if(mDOF) mDOF->setFocusMode(DOFManager::Auto);
			}
		}
		else if (cameramode==CAMERA_FIX)
		{
			float px, pz;
			px=((int)(person->getPosition().x)/100)*100;
			pz=((int)(person->getPosition().z)/100)*100;
			Real h=hfinder->getHeightAt(px+50.0,pz+50.0);
			Real random = Math::RangeRandom(0.0f, 1.0f);
			if(w && random > 0.3f && !w->allowUnderWater())
			{
				// chance of 30% to get an underwater view?
				if (w && w->getHeightWaves(Vector3(px+50.0,0,pz+50.0)) > h)
					h=w->getHeightWaves(Vector3(px+50.0,0,pz+50.0));
			} else if (w && w->allowUnderWater())
			{
				h = w->getHeightWaves(Vector3(px+50.0,0,pz+50.0) - 10.0f);
			}
			mCamera->setPosition(Vector3(px+50.0, h+1.7, pz+50.0));
			mCamera->lookAt(person->getPosition());
			float real_camDist = (mCamera->getPosition()-person->getPosition()).length();
			Radian fov = Radian(atan2(20.0f,real_camDist));
			mCamera->setFOVy(fov);

			if(mDOF)
			{
				mDOF->setFocusMode(DOFManager::Manual);
				mDOF->setFocus(real_camDist);
				mDOF->setLensFOV(fov);
			}

		}
		else if (cameramode==CAMERA_VEHICLE_INTERNAL)
		{
			float fov = FSETTING("FOV Internal", 75);
			if(changeCamMode)
				mCamera->setFOVy(Degree(fov));
			mCamera->setPosition(person->getPosition()+Vector3(0,1.7,0));
			Vector3 dir=Vector3(cos(person->getAngle()), 0.0, sin(person->getAngle()));
			mCamera->lookAt(mCamera->getPosition()+dir);
			mCamera->yaw(-camRotX);
			mCamera->pitch(camRotY);
			if (w && lastPosition.y<w->getHeightWaves(lastPosition))
			{
				cameramode=CAMERA_EXT;
				camRotX=pushcamRotX;
				camRotY=pushcamRotY;
			}
			if(mDOF)
			{
				mDOF->setFocusMode(DOFManager::Auto);
				mDOF->setLensFOV(Degree(fov));
			}

		}
	}
	else
	{
		// in truck
		if (cameramode==CAMERA_EXT)
		{
			if (collisions->forcecam)
			{
				mCamera->setPosition(collisions->forcecampos);
				mCamera->lookAt(curr_truck->getPosition());
				if(changeCamMode)
					mCamera->setFOVy(Degree(80));
				collisions->forcecam=false;

				if(mDOF)
				{
					mDOF->setFocusMode(DOFManager::Auto);
					mDOF->setLensFOV(Degree(80));
				}
			}
		}
		if (cameramode==CAMERA_FIX)
		{
			if (curr_truck->driveable==AIRPLANE)
			{
				if ((mCamera->getPosition()-curr_truck->getPosition()).length()>500.0)
				{
					Vector3 newposition=curr_truck->getPosition();
					Vector3 dir=curr_truck->nodes[0].Velocity;
					dir.normalise();
					newposition=newposition+dir*450.0+Vector3(5.0, 0.0, 5.0);
					Real h=hfinder->getHeightAt(newposition.x,newposition.z);
					Real random = Math::RangeRandom(0.0f, 1.0f);
					if(w && random > 0.3f && !w->allowUnderWater())
					{
						// chance of 30% to get an underwater view?
						if (w && w->getHeightWaves(newposition)>h)
							h=w->getHeightWaves(newposition);
					} else if (w && w->allowUnderWater())
					{
						h=w->getHeightWaves(newposition) - 10.0f;
					}

					if (newposition.y<h+2.0) newposition.y=h+2.0;
					mCamera->setPosition(newposition);
				}
				mCamera->lookAt(curr_truck->getPosition());
				float real_camDist = (mCamera->getPosition()-curr_truck->getPosition()).length();
				Radian fov = Radian(atan2(100.0f,real_camDist));
				mCamera->setFOVy(fov);
				if(mDOF)
				{
					mDOF->setFocusMode(DOFManager::Manual);
					mDOF->setFocus(real_camDist);
					mDOF->setLensFOV(fov);
				}

			}
			else
			{
				float px, pz;
				px=((int)(curr_truck->getPosition().x)/100)*100;
				pz=((int)(curr_truck->getPosition().z)/100)*100;
				Real h=hfinder->getHeightAt(px+50.0,pz+50.0);
				if (w && !w->allowUnderWater() && w->getHeightWaves(Vector3(px+50.0,0,pz+50.0))>h)
					h=w->getHeightWaves(Vector3(px+50.0,0,pz+50.0));
				mCamera->setPosition(Vector3(px+50.0, h+1.7, pz+50.0));
				mCamera->lookAt(curr_truck->getPosition());
				float real_camDist = (mCamera->getPosition()-curr_truck->getPosition()).length();
				Radian fov = Radian(atan2(20.0f,real_camDist));
				mCamera->setFOVy(fov);
				if(mDOF)
				{
					mDOF->setFocusMode(DOFManager::Manual);
					mDOF->setFocus(real_camDist);
					mDOF->setLensFOV(fov);
				}
			}
		}
		if (cameramode==CAMERA_VEHICLE_INTERNAL)
		{
			int currentcamera=curr_truck->currentcamera;

			float fov = FSETTING("FOV Internal", 75);

			if(changeCamMode)
				mCamera->setFOVy(Degree(fov));

			if (curr_truck->cinecameranodepos>=0) lastPosition=curr_truck->nodes[curr_truck->cinecameranodepos[currentcamera]].smoothpos;
			else lastPosition=curr_truck->nodes[curr_truck->cameranodepos[currentcamera]].smoothpos;
			mCamera->setPosition(lastPosition);
			if(curr_truck->cablightNode)
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

#ifdef USE_MPLATFORM
			mstat_t mStatInfo;

			// roll
			dir = curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranoderoll[0]].RelPosition;
			dir.normalise();

			float angle = asin(dir.dotProduct(Vector3::UNIT_Y));
			if (angle<-1) angle=-1;
			if (angle>1) angle=1;

			mStatInfo.roll = Radian(angle).valueRadians();

			//pitch
			dir=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
			dir.normalise();

			angle=asin(dir.dotProduct(Vector3::UNIT_Y));
			if (angle<-1) angle=-1;
			if (angle>1) angle=1;

			mStatInfo.pitch = Radian(angle).valueRadians();

			mStatInfo.speed    = curr_truck->WheelSpeed;
			mStatInfo.clutch   = curr_truck->engine->getClutch();
			mStatInfo.rpm      = curr_truck->engine->getRPM();
			mStatInfo.throttle = INPUTENGINE.getEventValue(EV_TRUCK_ACCELERAT);
			mStatInfo.gear	   = curr_truck->engine->getGear();
			mStatInfo.brake	   = INPUTENGINE.getEventValue(EV_TRUCK_BRAKE);
			mStatInfo.steer	   = curr_truck->hydrodircommand;


			mplatform->update(mCamera->getPosition(), mCamera->getOrientation(), mStatInfo);
#endif

			if (w && lastPosition.y<w->getHeightWaves(lastPosition))
			{
				cameramode=CAMERA_EXT;
				camRotX=pushcamRotX;
				camRotY=pushcamRotY;
				curr_truck->prepareInside(false);
				if(ow) ow->showDashboardOverlays(true, curr_truck);
			}

				if(mDOF)
				{
					mDOF->setFocusMode(DOFManager::Manual);
					mDOF->setFocus(2);
					mDOF->setLensFOV(Degree(fov));
				}

		}
	}

	//set LOD per truck
	/*
	// TODO: XXX: fix below
	for (int i=0; i<free_truck; i++)
	{
		if(!trucks[i]) continue;
		trucks[i]->setDetailLevel((mCamera->getPosition()-trucks[i]->getPosition()).length()>trucks[i]->fadeDist);
	}
	*/
#endif // 0
}

bool CameraManager::mouseMoved(const OIS::MouseEvent& _arg)
{
	if ( !currentBehavior ) return false;
	return currentBehavior->mouseMoved(_arg);
}

bool CameraManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	if ( !currentBehavior ) return false;
	return currentBehavior->mousePressed(_arg, _id);
}

bool CameraManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	if ( !currentBehavior ) return false;
	return currentBehavior->mouseReleased(_arg, _id);
}

bool CameraManager::allowInteraction()
{
	if ( !currentBehavior ) return false;
	return currentBehavior->allowInteraction();
}

#if 0
void CameraManager::updateInput()
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

	//camera mode
	if (cameramode != CAMERA_FREE && INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_CHANGE) && cameramode != CAMERA_FREE_FIXED)
	{
		if (cameramode==CAMERA_VEHICLE_INTERNAL)
		{
			//end of internal cam
			camRotX=pushcamRotX;
			camRotY=pushcamRotY;
		}
		cameramode++;
		if (cameramode==CAMERA_VEHICLE_INTERNAL)
		{
			//start of internal cam
			pushcamRotX=camRotX;
			pushcamRotY=camRotY;
			camRotX=0;
			camRotY=DEFAULT_INTERNAL_CAM_PITCH;
		}
		if (cameramode==CAMERA_END) cameramode=0;
	}
	//camera mode
	if (curr_truck && INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_CHANGE) && cameramode != CAMERA_FREE && cameramode != CAMERA_FREE_FIXED)
	{
		if (cameramode==CAMERA_VEHICLE_INTERNAL && curr_truck->currentcamera < curr_truck->freecinecamera-1)
		{
			curr_truck->currentcamera++;
			curr_truck->changedCamera();
		}
		else
		{
			OverlayWrapper *ow = OverlayWrapper::getSingletonPtrNoCreation();
			if (cameramode==CAMERA_VEHICLE_INTERNAL)
			{
				//end of internal cam
				camRotX=pushcamRotX;
				camRotY=pushcamRotY;
				curr_truck->prepareInside(false);
				if(ow) ow->showDashboardOverlays(true, curr_truck);
				curr_truck->currentcamera=-1;
				//if(bigMap) bigMap->setVisibility(true);
				curr_truck->changedCamera();
			}
			cameramode++;
			if (cameramode==CAMERA_VEHICLE_INTERNAL)
			{
				//start of internal cam
				pushcamRotX=camRotX;
				pushcamRotY=camRotY;
				camRotX=0;
				camRotY=DEFAULT_INTERNAL_CAM_PITCH;
				curr_truck->prepareInside(true);
				//if(bigMap) bigMap->setVisibility(false);
				// airplane dashboard in the plane visible
				if(ow)
				{
					if(curr_truck->driveable == AIRPLANE)
						ow->showDashboardOverlays(true, curr_truck);
					else
						ow->showDashboardOverlays(false, curr_truck);
				}
				curr_truck->currentcamera=0;
				curr_truck->changedCamera();
			}

			if (cameramode==CAMERA_END) cameramode = CAMERA_EXT;
		}
	}
	// camera FOV settings
	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_FOV_LESS))
	{
		int fov = mCamera->getFOVy().valueDegrees();
		if(fov>10)
			fov -= 2;
		mCamera->setFOVy(Degree(fov));
#ifdef USE_MYGUI
		Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: ") + TOSTRING(fov), "camera_edit.png", 2000);
#endif // USE_MYGUI
		// save the settings
		if (cameramode == CAMERA_VEHICLE_INTERNAL)
			SETTINGS.setSetting("FOV Internal", TOSTRING(fov));
		else if (cameramode == CAMERA_EXT)
			SETTINGS.setSetting("FOV External", TOSTRING(fov));
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_FOV_MORE))
	{
		int fov = mCamera->getFOVy().valueDegrees();
		if(fov<160)
			fov += 2;
		mCamera->setFOVy(Degree(fov));
#ifdef USE_MYGUI
		Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: ") + TOSTRING(fov), "camera_edit.png", 2000);
#endif // USE_MYGUI
		// save the settings
		if (cameramode == CAMERA_VEHICLE_INTERNAL)
			SETTINGS.setSetting("FOV Internal", TOSTRING(fov));
		else if (cameramode == CAMERA_EXT)
			SETTINGS.setSetting("FOV External", TOSTRING(fov));
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TOGGLE_RENDER_MODE, 0.5f))
	{
		mCamera->setPolygonMode( (mCamera->getPolygonMode() + 1) % 3 );
	}
	if (INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_FREE_MODE_FIX))
	{
		if(cameramode == CAMERA_FREE)
		{
			// change to fixed free camera: that is working like fixed cam
			cameramode = CAMERA_FREE_FIXED;
			if(mDOF) mDOF->setFocusMode(DOFManager::Auto);
			LOG("switching to fixed free camera mode");
#ifdef USE_MYGUI
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("fixed free camera"), "camera_link.png", 3000);
#endif // USE_MYGUI
		} else if(cameramode == CAMERA_FREE_FIXED)
		{
			cameramode = CAMERA_FREE;
			if(mDOF) mDOF->setFocusMode(DOFManager::Auto);
			LOG("switching to free camera mode from fixed mode");
#ifdef USE_MYGUI
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("free camera"), "camera_go.png", 3000);
#endif // USE_MYGUI
		}
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_FREE_MODE))
	{
		currentBehavior = globalBehaviors[CAMERA_FREE];
	}
}
#endif // 0

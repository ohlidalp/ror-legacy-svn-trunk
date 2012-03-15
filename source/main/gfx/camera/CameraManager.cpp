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
#include "Ogre.h"

#include "InputEngine.h"
#include "Settings.h"
#include "Console.h"
#include "language.h"

using namespace Ogre;

CameraManager::CameraManager(Ogre::SceneManager *scm, Ogre::Camera *cam) : 
	  mSceneMgr(scm)
	, mCamera(cam)
	// TODO: initialize other vars here
{
	setSingleton(this);

	externalCameraMode=0;
	lastcameramode=CAMERA_EXT;
	cameramode=CAMERA_EXT;
	externalCameraMode = (SSETTING("External Camera Mode", "Pitching") == "Static")? 1 : 0;
	mMoveScale = 0.0f;
	mRotScale = 0.0f;
	camIdealPosition = Vector3::ZERO;
	lastPosition = Vector3::ZERO;
	camRotX=0;
	camRotY=Degree(12);
	camDist=20;
	camCollided=false;
	camPosColl=Vector3::ZERO;
	mSceneDetailIndex = 0;
	mDOF=0;
	mRotateSpeed = 100;
	mMoveSpeed = 50;

	// DOF?
	bool useDOF = (BSETTING("DOF", false));
	if(useDOF)
	{
		mDOF = new DOFManager(mSceneMgr, mCamera->getViewport(), Ogre::Root::getSingletonPtr(), mCamera);
		mDOF->setEnabled(true);
	}

}

CameraManager::~CameraManager()
{

}

void CameraManager::updateInput()
{
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
		if (cameramode == CAMERA_INT)
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
		if (cameramode == CAMERA_INT)
			SETTINGS.setSetting("FOV Internal", TOSTRING(fov));
		else if (cameramode == CAMERA_EXT)
			SETTINGS.setSetting("FOV External", TOSTRING(fov));
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TOGGLE_RENDER_MODE, 0.5f))
	{
		mSceneDetailIndex = (mSceneDetailIndex+1)%3 ;
		switch(mSceneDetailIndex) {
		case 0 : mCamera->setPolygonMode(Ogre::PM_SOLID) ; break ;
		case 1 : mCamera->setPolygonMode(Ogre::PM_WIREFRAME) ; break ;
		case 2 : mCamera->setPolygonMode(Ogre::PM_POINTS) ; break ;
		}
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
		static int storedcameramode = -1;
		if(cameramode == CAMERA_FREE || cameramode == CAMERA_FREE_FIXED)
		{
			// change back to normal camera
			if(mDOF) mDOF->setFocusMode(DOFManager::Manual);
			cameramode = storedcameramode;
			LOG("exiting free camera mode");
#ifdef USE_MYGUI
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("normal camera"), "camera.png", 3000);
#endif // USE_MYGUI
		} else if(cameramode != CAMERA_FREE && cameramode != CAMERA_FREE_FIXED )
		{
			// enter free camera mode
			if(mDOF) mDOF->setFocusMode(DOFManager::Auto);
			storedcameramode = cameramode;
			cameramode = CAMERA_FREE;
			LOG("entering free camera mode");
#ifdef USE_MYGUI
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("free camera"), "camera_go.png", 3000);
#endif // USE_MYGUI
		}
	}

}

void CameraManager::update(float dt)
{

	// If this is the first frame, pick a speed
	if (dt == 0)
	{
		mMoveScale = 1;
		mRotScale = 0.1;
	}
	// Otherwise scale movement units by time passed since last frame
	else
	{
		// Move about 100 units per second,
		mMoveScale = mMoveSpeed * evt.timeSinceLastFrame;
		// Take about 10 seconds for full rotation
		mRotScale = mRotateSpeed * evt.timeSinceLastFrame;
	}



	if(!hfinder) return;
	if (loading_state!=ALL_LOADED && loading_state != EDITOR_PAUSE) return;

#ifdef MYGUI
	if (SceneMouse::getSingleton().isMouseGrabbed()) return; //freeze camera
#endif //MYGUI

	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

	bool changeCamMode = (lastcameramode != cameramode) || enforceCameraFOVUpdate;
	enforceCameraFOVUpdate = false;
	lastcameramode = cameramode;

	if (cameramode==CAMERA_FREE)
	{
		// this is a workaround for the free camera mode :)
		Real mMoveScale = 0.1;
		Ogre::Degree mRotScale(0.1f);
		Ogre::Degree mRotX(0);
		Ogre::Degree mRotY(0);
		Vector3 mTranslateVector = Vector3::ZERO;

		if(INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) || INPUTENGINE.isKeyDown(OIS::KC_RSHIFT))
		{
			mRotScale *= 3;
			mMoveScale *= 3;
		}

		if(INPUTENGINE.isKeyDown(OIS::KC_LCONTROL))
		{
			mRotScale *= 30;
			mMoveScale *= 30;
		}

		if(INPUTENGINE.isKeyDown(OIS::KC_LMENU))
		{
			mRotScale *= 0.05;
			mMoveScale *= 0.05;
		}


		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
			mTranslateVector.x = -mMoveScale;	// Move camera left

		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
			mTranslateVector.x = mMoveScale;	// Move camera RIGHT

		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_FORWARD))
			mTranslateVector.z = -mMoveScale;	// Move camera forward

		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_BACKWARDS))
			mTranslateVector.z = mMoveScale;	// Move camera backward

		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_UP))
			mRotY += mRotScale;

		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_ROT_DOWN))
			mRotY += -mRotScale;

		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_UP))
			mTranslateVector.y = mMoveScale;	// Move camera up

		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_DOWN))
			mTranslateVector.y = -mMoveScale;	// Move camera down

		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_RIGHT))
			mRotX += -mRotScale;

		if(INPUTENGINE.getEventBoolValue(EV_CHARACTER_LEFT))
			mRotX += mRotScale;

		mCamera->yaw(mRotX);
		mCamera->pitch(mRotY);

		Vector3 trans = mCamera->getOrientation() * mTranslateVector;
		setCameraPositionWithCollision(mCamera->getPosition() + trans);
	}
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
			else
			{
				// Make all the changes to the camera
				//Vector3 delta=lastPosition-personode->getPosition();
				//delta.y=0.0;
				float angle=-person->getAngle()-(3.14159/2.0);
				//			float angle2;
				//if (delta.length()>0.01) angle=atan2(delta.x,delta.z); else angle=lastangle;

				// fix camera distance a bit
				if(camDist < 3) camDist = 3.0f;

				camIdealPosition=camDist/2.0*Vector3(sin(angle+camRotX.valueRadians())*cos(camRotY.valueRadians()),sin(camRotY.valueRadians()),cos(angle+camRotX.valueRadians())*cos(camRotY.valueRadians()));

				float real_camdist = camIdealPosition.length();

				camIdealPosition=camIdealPosition+person->getPosition();
				Vector3 newposition=(camIdealPosition+10.0*mCamera->getPosition())/11.0;
				Real h=hfinder->getHeightAt(newposition.x,newposition.z);

				if (w && !w->allowUnderWater() && w->getHeightWaves(newposition) > h)
					h=w->getHeightWaves(newposition);

				h+=1.0;
				if (newposition.y<h) newposition.y=h;
				setCameraPositionWithCollision(newposition);
				mCamera->lookAt(person->getPosition()+Vector3(0.0,1.1f,0.0));

				float fov = FSETTING("FOV External", 60);

				if(changeCamMode)
					mCamera->setFOVy(Degree(fov));

				lastPosition=person->getPosition();

				if(mDOF)
				{
					mDOF->setFocusMode(DOFManager::Manual);
					mDOF->setFocus(real_camdist);
					mDOF->setLensFOV(Degree(fov));
				}

				//lastangle=angle;
			}
		}
		else if (cameramode==CAMERA_FIX)
		{
			float px, pz;
			px=((int)(person->getPosition().x)/100)*100;
			pz=((int)(person->getPosition().z)/100)*100;
			Real h=hfinder->getHeightAt(px+50.0,pz+50.0);
			Real random = Ogre::Math::RangeRandom(0.0f, 1.0f);
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
		else if (cameramode==CAMERA_INT)
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
			else
			{
				// Make all the changes to the camera
				//Vector3 delta=lastPosition-trucks[current_truck]->position;
				//delta.y=0.0;
				float angle;
				//			float angle2;
				//				if (delta.length()>0.05) angle=atan2(delta.x,delta.z); else angle=lastangle;
				Vector3 dir=curr_truck->nodes[curr_truck->cameranodepos[0]].smoothpos-curr_truck->nodes[curr_truck->cameranodedir[0]].smoothpos;
				dir.normalise();
				angle=-atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));

				if(externalCameraMode==0)
				{
					float pitch=-asin(dir.dotProduct(Vector3::UNIT_Y));
					camIdealPosition=camDist*Vector3(sin(angle+camRotX.valueRadians())*cos(pitch+camRotY.valueRadians()),sin(pitch+camRotY.valueRadians()),cos(angle+camRotX.valueRadians())*cos(pitch+camRotY.valueRadians()));
				} else if(externalCameraMode==1)
				{
					camIdealPosition=camDist*Vector3(sin(angle+camRotX.valueRadians())*cos(camRotY.valueRadians()),sin(camRotY.valueRadians()),cos(angle+camRotX.valueRadians())*cos(camRotY.valueRadians()));
				}

				float cam_realdist = camIdealPosition.length();
				camIdealPosition=camIdealPosition+curr_truck->getPosition();
				Vector3 oldposition=mCamera->getPosition()+curr_truck->nodes[0].Velocity*curr_truck->ttdt;
				float ratio=1.0/(curr_truck->tdt*4.0);
				//float ratio=0.001;
				//Vector3 newposition=(camIdealPosition+ratio*mCamera->getPosition())/(ratio+1.0);
				//Vector3 newposition=camIdealPosition;
				Vector3 newposition=(1/(ratio+1.0))*camIdealPosition+(ratio/(ratio+1.0))*oldposition;

				Real h=hfinder->getHeightAt(newposition.x,newposition.z);
				if (w && !w->allowUnderWater() && w->getHeightWaves(newposition)>h)
					h=w->getHeightWaves(newposition);
				h+=1.0;
				if (newposition.y<h) newposition.y=h;
				setCameraPositionWithCollision(newposition);
				mCamera->lookAt(curr_truck->getPosition());

				float fov = FSETTING("FOV External", 60);

				if(changeCamMode)
					mCamera->setFOVy(Degree(fov));

				lastPosition=curr_truck->getPosition();

				if(mDOF)
				{
					mDOF->setFocusMode(DOFManager::Manual);
					mDOF->setFocus(cam_realdist);
					mDOF->setLensFOV(Degree(80));
				}


				//lastangle=angle;
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
					Real random = Ogre::Math::RangeRandom(0.0f, 1.0f);
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
		if (cameramode==CAMERA_INT)
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
	//envmap
	if (envmap)
	{
		if (!envmap->inited)
		{
			envmap->forceUpdate(Vector3(terrainxsize/2.0, hfinder->getHeightAt(terrainxsize/2.0, terrainzsize/2.0)+50.0, terrainzsize/2.0));
		}
		if (curr_truck)
		{
			envmap->update(curr_truck->getPosition(), curr_truck);

#ifdef USE_CAELUM
			// important: switch back to normal camera
			if(SkyManager::getSingletonPtr())
				SkyManager::getSingleton().notifyCameraChanged(mCamera);
#endif // USE_CAELUM
		}
	}

	//position audio listener
	Vector3 cpos=mCamera->getPosition();
	Vector3 cdir=mCamera->getDirection();
	Vector3 cup=mCamera->getUp();
	Vector3 cspeed=(cpos-cdoppler)/dt;
	cdoppler=cpos;
	// XXX maybe thats the source of sound destorsion: runtime order???
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().setCamera(cpos, cdir, cup, cspeed);
#endif // OPENAL
	//if (cspeed.length()>50.0) {cspeed.normalise(); cspeed=50.0*cspeed;};
	//if (audioManager) audioManager->setListenerPosition(cpos.x, cpos.y, cpos.z, cspeed.x, cspeed.y, cspeed.z, cdir.x, cdir.y, cdir.z, cup.x, cup.y, cup.z);
	//water
	if (w)
	{
		if (curr_truck)
			w->moveTo(mCamera, w->getHeightWaves(curr_truck->getPosition()));
		else
			w->moveTo(mCamera, w->getHeight());
	}
}


bool CameraManager::setCameraPositionWithCollision(Vector3 newPos)
{
	// no collision of camera, normal mode
	mCamera->setPosition(newPos);
	return true;
}
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
#include "RoRFrameListener.h"

#include <OgreFontManager.h>
#include <OgrePrerequisites.h>
#include <OgreRTShaderSystem.h>
#include <OgreTerrain.h>
#include <OgreTerrainMaterialGeneratorA.h>
#include <OgreTerrainPaging.h>
#include <OgreTerrainQuadTreeNode.h>

#include "AdvancedScreen.h"
#include "autopilot.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "Character.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "CollisionTools.h"
#include "dashboard.h"
#include "DashBoardManager.h"
#include "DepthOfFieldEffect.h"
#include "DustManager.h"
#include "editor.h"
#include "envmap.h"
#include "errorutils.h"
#include "ExtinguishableFireAffector.h"
#include "FlexAirfoil.h"
#include "ForceFeedback.h"
#include "GlowMaterialListener.h"
#include "hdrlistener.h"
#include "Heathaze.h"
#include "InputEngine.h"
#include "IWater.h"
#include "language.h"
#include "MeshObject.h"
#include "MumbleIntegration.h"
#include "network.h"
#include "TerrainManager.h"
#include "OutProtocol.h"
#include "OverlayWrapper.h"
#include "PlayerColours.h"
#include "PreviewRenderer.h"
#include "ProceduralManager.h"
#include "Replay.h"
#include "RigsOfRods.h"
#include "Road.h"
#include "road2.h"
#include "RoRVersion.h"
#include "SceneMouse.h"
#include "ScopeLog.h"
#include "screwprop.h"
#include "Scripting.h"
#include "Settings.h"
#include "ShadowManager.h"
#include "SkyManager.h"
#include "SoundScriptManager.h"
#include "TruckHUD.h"
#include "turboprop.h"
#include "utils.h"
#include "vidcam.h"
#include "WaterOld.h"
#include "writeTextToTexture.h"

#ifdef USE_MYGUI
#include "gui_manager.h"
#include "gui_menu.h"
#include "gui_friction.h"
#include "gui_mp.h"
#include "SelectorWindow.h"
#include "LoadingWindow.h"
#include "Console.h"
#include "MapControl.h"
#include "MapTextureCreator.h"
#include "MapEntity.h"
#endif //USE_MYGUI

#include "ResourceBuffer.h"

#ifdef USE_PAGED
#include "BatchPage.h"
#include "GrassLoader.h"
#include "ImpostorPage.h"
#include "PagedGeometry.h"
#include "TreeLoader2D.h"
#include "TreeLoader3D.h"
#endif //USE_PAGED

#ifdef USE_MPLATFORM
#include "mplatform_fd.h"
#endif //USE_MPLATFORM

#ifdef HAS_EDITOR
#include "truckeditor.h"
#endif //HAS_EDITOR

#ifdef FEAT_TIMING
#include "BeamStats.h"
#endif //FEAT_TIMING

#ifdef USE_OIS_G27
#include "win32/Win32LogitechLEDs.h"
#endif //USE_OIS_G27

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <Windows.h>
#else
#include <stdio.h>
#include <wchar.h>
#endif

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//#include <CFUserNotification.h>
#endif

#ifdef USE_PAGED
using namespace Forests;
#endif //USE_PAGED

float RoRFrameListener::gravity = DEFAULT_GRAVITY;

/// This class just pretends to provide procedural page content to avoid page loading
class DummyPageProvider : public PageProvider
{
public:
	bool prepareProceduralPage(Page* page, PagedWorldSection* section) { return true; }
	bool loadProceduralPage(Page* page, PagedWorldSection* section) { return true; }
	bool unloadProceduralPage(Page* page, PagedWorldSection* section) { return true; }
	bool unprepareProceduralPage(Page* page, PagedWorldSection* section) { return true; }
};
DummyPageProvider mDummyPageProvider;

Material *terrainmaterial = 0;

char terrainoriginalmaterial[100];
bool shutdownall=false;

// static height finder
HeightFinder *RoRFrameListener::hfinder = 0;
// static elf singleton
RoRFrameListener *RoRFrameListener::eflsingleton = 0;

//workaround for pagedgeometry
inline float getTerrainHeight(Ogre::Real x, Ogre::Real z, void *unused=0)
{
	if (!RoRFrameListener::hfinder)
		return 1;
	return RoRFrameListener::hfinder->getHeightAt(x, z);
}

void RoRFrameListener::startTimer()
{
	raceStartTime = (int)rtime;
	if (ow)
	{
		ow->racing->show();
		ow->laptimes->show();
		ow->laptimems->show();
		ow->laptimemin->show();
	}
}

float RoRFrameListener::stopTimer()
{
	float time=rtime - raceStartTime;
	// let the display on
	if (ow)
	{
		wchar_t txt[256] = L"";
		UTFString fmt = _L("Last lap: %.2i'%.2i.%.2i");
		swprintf(txt, 256, fmt.asWStr_c_str(), ((int)(time))/60,((int)(time))%60, ((int)(time*100.0))%100);
		ow->lasttime->setCaption(UTFString(txt));
		//ow->racing->hide();
		ow->laptimes->hide();
		ow->laptimems->hide();
		ow->laptimemin->hide();
	}
	raceStartTime = -1;
	return time;
}

void RoRFrameListener::updateRacingGUI()
{
	if (!ow) return;
	// update racing gui if required
	float time=rtime - raceStartTime;
	wchar_t txt[10];
	swprintf(txt, 10, L"%.2i", ((int)(time*100.0))%100);
	ow->laptimems->setCaption(txt);
	swprintf(txt, 10, L"%.2i", ((int)(time))%60);
	ow->laptimes->setCaption(txt);
	swprintf(txt, 10, L"%.2i'", ((int)(time))/60);
	ow->laptimemin->setCaption(UTFString(txt));
}

void RoRFrameListener::updateIO(float dt)
{
	Beam *current_truck = BeamFactory::getSingleton().getCurrentTruck();
	if (current_truck && current_truck->driveable == TRUCK)
	{
#ifdef USE_OIS_G27
		// logitech G27 LEDs tachometer
		if (leds)
		{
			leds->play(current_truck->engine->getRPM(),
				current_truck->engine->getMaxRPM()*0.75,
				current_truck->engine->getMaxRPM());
		}
#endif //OIS_G27

		// force feedback
		if (forcefeedback)
		{
			int cameranodepos = 0;
			int cameranodedir = 0;
			int cameranoderoll = 0;

			if (current_truck->cameranodepos[0] >= 0 && current_truck->cameranodepos[0] < MAX_NODES)
				cameranodepos = current_truck->cameranodepos[0];
			if (current_truck->cameranodedir[0] >= 0 && current_truck->cameranodedir[0] < MAX_NODES)
				cameranodedir = current_truck->cameranodedir[0];
			if (current_truck->cameranoderoll[0] >= 0 && current_truck->cameranoderoll[0] < MAX_NODES)
				cameranoderoll = current_truck->cameranoderoll[0];

			Vector3 udir = current_truck->nodes[cameranodepos].RelPosition-current_truck->nodes[cameranodedir].RelPosition;
			Vector3 uroll = current_truck->nodes[cameranodepos].RelPosition-current_truck->nodes[cameranoderoll].RelPosition;
			
			udir.normalise();
			uroll.normalise();

			forcefeedback->setForces(-current_truck->ffforce.dotProduct(uroll)/10000.0,
				current_truck->ffforce.dotProduct(udir)/10000.0,
				current_truck->WheelSpeed,
				current_truck->hydrodircommand,
				current_truck->ffhydro);
		}
	}
}

void RoRFrameListener::updateGUI(float dt)
{
	if (!ow) return; // no gui, then skip this

	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if (!curr_truck) return;

	//update the truck info gui (also if not displayed!)
	ow->truckhud->update(dt, curr_truck, mSceneMgr, mCamera, mWindow, mTruckInfoOn);

#ifdef FEAT_TIMING
	BES.updateGUI(dt);
#endif

#ifdef USE_MYGUI
	// update mouse picking lines, etc
	SceneMouse::getSingleton().update(dt);
#endif //USE_MYGUI

	if (pressure_pressed)
	{
		Real angle = 135.0 - curr_truck->getPressure() * 2.7;
		ow->pressuretexture->setTextureRotate(Degree(angle));
	}

	// racing gui
	if (raceStartTime > 0)
		updateRacingGUI();

	if (curr_truck->driveable == TRUCK)
	{
		//TRUCK
		if (!curr_truck->engine) return;

		//special case for the editor
		if (curr_truck->editorId>=0 && editor)
		{
			ow->editor_pos->setCaption("Position: X=" +
				TOSTRING(curr_truck->nodes[curr_truck->editorId].AbsPosition.x)+
				"  Y="+TOSTRING(curr_truck->nodes[curr_truck->editorId].AbsPosition.y)+
				"  Z="+TOSTRING(curr_truck->nodes[curr_truck->editorId].AbsPosition.z)
				);
			ow->editor_angles->setCaption("Angles: 0.0 " +
				TOSTRING(editor->pturn)+
				"  "+TOSTRING(editor->ppitch)
				);
			char type[256];
			sprintf(type, "Object: %s", editor->curtype);
			ow->editor_object->setCaption(type);
		}

		// gears
		int truck_getgear = curr_truck->engine->getGear();
		if (truck_getgear>0)
		{
			size_t numgears = curr_truck->engine->getNumGears();
			String gearstr = TOSTRING(truck_getgear) + "/" + TOSTRING(numgears);
			ow->guiGear->setCaption(gearstr);
			ow->guiGear3D->setCaption(gearstr);
		} else if (truck_getgear==0)
		{
			ow->guiGear->setCaption("N");
			ow->guiGear3D->setCaption("N");
		} else
		{
			ow->guiGear->setCaption("R");
			ow->guiGear3D->setCaption("R");
		}
		//autogears
		int cg = curr_truck->engine->getAutoShift();
		for (int i=0; i<5; i++)
		{
			if (i==cg)
			{
				if (i==1)
				{
					ow->guiAuto[i]->setColourTop(ColourValue(1.0, 0.2, 0.2, 1.0));
					ow->guiAuto[i]->setColourBottom(ColourValue(0.8, 0.1, 0.1, 1.0));
					ow->guiAuto3D[i]->setColourTop(ColourValue(1.0, 0.2, 0.2, 1.0));
					ow->guiAuto3D[i]->setColourBottom(ColourValue(0.8, 0.1, 0.1, 1.0));
				} else
				{
					ow->guiAuto[i]->setColourTop(ColourValue(1.0, 1.0, 1.0, 1.0));
					ow->guiAuto[i]->setColourBottom(ColourValue(0.8, 0.8, 0.8, 1.0));
					ow->guiAuto3D[i]->setColourTop(ColourValue(1.0, 1.0, 1.0, 1.0));
					ow->guiAuto3D[i]->setColourBottom(ColourValue(0.8, 0.8, 0.8, 1.0));
				}
			} else
			{
				if (i==1)
				{
					ow->guiAuto[i]->setColourTop(ColourValue(0.4, 0.05, 0.05, 1.0));
					ow->guiAuto[i]->setColourBottom(ColourValue(0.3, 0.02, 0.2, 1.0));
					ow->guiAuto3D[i]->setColourTop(ColourValue(0.4, 0.05, 0.05, 1.0));
					ow->guiAuto3D[i]->setColourBottom(ColourValue(0.3, 0.02, 0.2, 1.0));
				} else
				{
					ow->guiAuto[i]->setColourTop(ColourValue(0.4, 0.4, 0.4, 1.0));
					ow->guiAuto[i]->setColourBottom(ColourValue(0.3, 0.3, 0.3, 1.0));
					ow->guiAuto3D[i]->setColourTop(ColourValue(0.4, 0.4, 0.4, 1.0));
					ow->guiAuto3D[i]->setColourBottom(ColourValue(0.3, 0.3, 0.3, 1.0));
				}
			}

		}

		// pedals
		ow->guipedclutch->setTop(-0.05*curr_truck->engine->getClutch()-0.01);
		ow->guipedbrake->setTop(-0.05*(1.0-curr_truck->brake/curr_truck->brakeforce)-0.01);
		ow->guipedacc->setTop(-0.05*(1.0-curr_truck->engine->getAcc())-0.01);

		// speedo / calculate speed
		Real guiSpeedFactor = 7.0 * (140.0 / curr_truck->speedoMax);
		Real angle = 140 - fabs(curr_truck->WheelSpeed * guiSpeedFactor);
		if (angle < -140)
			angle = -140;
		ow->speedotexture->setTextureRotate(Degree(angle));

		// calculate tach stuff
		Real tachoFactor = 0.072;
		if (curr_truck->useMaxRPMforGUI)
			tachoFactor = 0.072 * (3500 / curr_truck->engine->getMaxRPM());
		angle=126.0-fabs(curr_truck->engine->getRPM() * tachoFactor);
		if (angle<-120.0) angle=-120.0;
		if (angle>121.0) angle=121.0;
		ow->tachotexture->setTextureRotate(Degree(angle));

		// roll
		Vector3 dir=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranoderoll[0]].RelPosition;
		dir.normalise();
		//	roll_node->resetOrientation();
		angle=asin(dir.dotProduct(Vector3::UNIT_Y));
		if (angle<-1) angle=-1;
		if (angle>1) angle=1;
		//float jroll=angle*1.67;
		ow->rolltexture->setTextureRotate(Radian(angle));
		//	roll_node->roll(Radian(angle));

		// rollcorr
		if (curr_truck->free_active_shock && ow && ow->guiRoll && ow->rollcortexture)
		{
			//		rollcorr_node->resetOrientation();
			//		rollcorr_node->roll(Radian(-curr_truck->stabratio*5.0));
			ow->rollcortexture->setTextureRotate(Radian(-curr_truck->stabratio*10.0));
			if (curr_truck->stabcommand)
				ow->guiRoll->setMaterialName("tracks/rollmaskblink");
			else
				ow->guiRoll->setMaterialName("tracks/rollmask");
		}

		// pitch
		dir=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
		dir.normalise();
		//	pitch_node->resetOrientation();
		angle=asin(dir.dotProduct(Vector3::UNIT_Y));
		if (angle<-1) angle=-1;
		if (angle>1) angle=1;
		ow->pitchtexture->setTextureRotate(Radian(angle));
		//	pitch_node->roll(Radian(angle));

		// turbo
		angle=40.0-curr_truck->engine->getTurboPSI()*3.34;
		ow->turbotexture->setTextureRotate(Degree(angle));

		// indicators
		ow->igno->setMaterialName(String("tracks/ign-")         + ((curr_truck->engine->hasContact())?"on":"off"));
		ow->batto->setMaterialName(String("tracks/batt-")       + ((curr_truck->engine->hasContact() && !curr_truck->engine->isRunning())?"on":"off"));
		ow->pbrakeo->setMaterialName(String("tracks/pbrake-")   + ((curr_truck->parkingbrake)?"on":"off"));
		ow->lockedo->setMaterialName(String("tracks/locked-")   + ((curr_truck->isLocked())?"on":"off"));
		ow->lopresso->setMaterialName(String("tracks/lopress-") + ((!curr_truck->canwork)?"on":"off"));
		ow->clutcho->setMaterialName(String("tracks/clutch-")   + ((fabs(curr_truck->engine->getTorque())>=curr_truck->engine->getClutchForce()*10.0f)?"on":"off"));
		ow->lightso->setMaterialName(String("tracks/lights-")   + ((curr_truck->lights)?"on":"off"));

		if (curr_truck->tc_present)
		{
			if (curr_truck->tc_mode)
				if (curr_truck->tractioncontrol)
				{
							ow->tcontrolo->setMaterialName(String("tracks/tcontrol-act"));
					} else
				{
					ow->tcontrolo->setMaterialName(String("tracks/tcontrol-on"));
				}
			else
			{
				ow->tcontrolo->setMaterialName(String("tracks/tcontrol-off"));
			}
		} else
		{
			ow->tcontrolo->setMaterialName(String("tracks/trans"));
		}

		if (curr_truck->alb_present)
		{
			if (curr_truck->alb_mode)
				if (curr_truck->antilockbrake)
				{
							ow->antilocko->setMaterialName(String("tracks/antilock-act"));
				} else
				{
					ow->antilocko->setMaterialName(String("tracks/antilock-on"));
				}
			else
			{
				ow->antilocko->setMaterialName(String("tracks/antilock-off"));
			}
		} else
		{
			ow->antilocko->setMaterialName(String("tracks/trans"));
		}

		if (curr_truck->isTied())
		{
			if (fabs(curr_truck->commandkey[0].commandValue) > 0.000001f)
			{
				flipflop = !flipflop;
				if (flipflop)
					ow->securedo->setMaterialName("tracks/secured-on");
				else
					ow->securedo->setMaterialName("tracks/secured-off");
			} else
			{
				ow->securedo->setMaterialName("tracks/secured-on");
			}
		} else
		{
			ow->securedo->setMaterialName("tracks/secured-off");
		}

	} else if (ow && curr_truck->driveable == AIRPLANE)
	{
		//AIRPLANE GUI
		int ftp = curr_truck->free_aeroengine;

		//throttles
		ow->thro1->setTop(ow->thrtop+ow->thrheight*(1.0-curr_truck->aeroengines[0]->getThrottle())-1.0);
		if (ftp>1) ow->thro2->setTop(ow->thrtop+ow->thrheight*(1.0-curr_truck->aeroengines[1]->getThrottle())-1.0);
		if (ftp>2) ow->thro3->setTop(ow->thrtop+ow->thrheight*(1.0-curr_truck->aeroengines[2]->getThrottle())-1.0);
		if (ftp>3) ow->thro4->setTop(ow->thrtop+ow->thrheight*(1.0-curr_truck->aeroengines[3]->getThrottle())-1.0);

		//fire
		if (curr_truck->aeroengines[0]->isFailed()) ow->engfireo1->setMaterialName("tracks/engfire-on"); else ow->engfireo1->setMaterialName("tracks/engfire-off");
		if (ftp > 1 && curr_truck->aeroengines[1]->isFailed()) ow->engfireo2->setMaterialName("tracks/engfire-on"); else ow->engfireo2->setMaterialName("tracks/engfire-off");
		if (ftp > 2 && curr_truck->aeroengines[2]->isFailed()) ow->engfireo3->setMaterialName("tracks/engfire-on"); else ow->engfireo3->setMaterialName("tracks/engfire-off");
		if (ftp > 3 && curr_truck->aeroengines[3]->isFailed()) ow->engfireo4->setMaterialName("tracks/engfire-on"); else ow->engfireo4->setMaterialName("tracks/engfire-off");

		//airspeed
		float angle=0.0;
		float ground_speed_kt=curr_truck->nodes[0].Velocity.length()*1.9438; // 1.943 = m/s in knots/s

		//tropospheric model valid up to 11.000m (33.000ft)
		float altitude=curr_truck->nodes[0].AbsPosition.y;
		float sea_level_temperature=273.15+15.0; //in Kelvin
		float sea_level_pressure=101325; //in Pa
		float airtemperature=sea_level_temperature-altitude*0.0065; //in Kelvin
		float airpressure=sea_level_pressure*pow(1.0-0.0065*altitude/288.15, 5.24947); //in Pa
		float airdensity=airpressure*0.0000120896;//1.225 at sea level

		float kt = ground_speed_kt*sqrt(airdensity/1.225); //KIAS
		if (kt>23.0)
		{
			if (kt<50.0)
				angle=((kt-23.0)/1.111);
			else if (kt<100.0)
				angle=(24.0+(kt-50.0)/0.8621);
			else if (kt<300.0)
				angle=(82.0+(kt-100.0)/0.8065);
			else
				angle=329.0;
		}
		ow->airspeedtexture->setTextureRotate(Degree(-angle));

		// AOA
		angle=0;
		if (curr_truck->free_wing>4)
			angle=curr_truck->wings[4].fa->aoa;
		if (kt<10.0) angle=0;
		float absangle=angle;
		if (absangle<0) absangle=-absangle;
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().modulate(curr_truck, SS_MOD_AOA, absangle);
		if (absangle > 18.0) // TODO: magicccc
			SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_AOA);
		else
			SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_AOA);
#endif // OPENAL
		if (angle>25.0) angle=25.0;
		if (angle<-25.0) angle=-25.0;
		ow->aoatexture->setTextureRotate(Degree(-angle*4.7+90.0));

		// altimeter
		angle=curr_truck->nodes[0].AbsPosition.y*1.1811;
		ow->altimetertexture->setTextureRotate(Degree(-angle));
		char altc[10];
		sprintf(altc, "%03u", (int)(curr_truck->nodes[0].AbsPosition.y/30.48));
		ow->alt_value_taoe->setCaption(altc);

		//adi
		//roll
		Vector3 rollv=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranoderoll[0]].RelPosition;
		rollv.normalise();
		float rollangle=asin(rollv.dotProduct(Vector3::UNIT_Y));

		//pitch
		Vector3 dirv=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
		dirv.normalise();
		float pitchangle=asin(dirv.dotProduct(Vector3::UNIT_Y));
		Vector3 upv=dirv.crossProduct(-rollv);
		if (upv.y<0) rollangle=3.14159-rollangle;
		ow->adibugstexture->setTextureRotate(Radian(-rollangle));
		ow->aditapetexture->setTextureVScroll(-pitchangle*0.25);
		ow->aditapetexture->setTextureRotate(Radian(-rollangle));

		//hsi
		Vector3 idir=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
		//			idir.normalise();
		float dirangle=atan2(idir.dotProduct(Vector3::UNIT_X), idir.dotProduct(-Vector3::UNIT_Z));
		ow->hsirosetexture->setTextureRotate(Radian(dirangle));
		if (curr_truck->autopilot)
		{
			ow->hsibugtexture->setTextureRotate(Radian(dirangle)-Degree(curr_truck->autopilot->heading));
			float vdev=0;
			float hdev=0;
			curr_truck->autopilot->getRadioFix(localizers, free_localizer, &vdev, &hdev);
			if (hdev>15) hdev=15;
			if (hdev<-15) hdev=-15;
			ow->hsivtexture->setTextureUScroll(-hdev*0.02);
			if (vdev>15) vdev=15;
			if (vdev<-15) vdev=-15;
			ow->hsihtexture->setTextureVScroll(-vdev*0.02);
		}

		//vvi
		float vvi=curr_truck->nodes[0].Velocity.y*196.85;
		if (vvi<1000.0 && vvi>-1000.0) angle=vvi*0.047;
		if (vvi>1000.0 && vvi<6000.0) angle=47.0+(vvi-1000.0)*0.01175;
		if (vvi>6000.0) angle=105.75;
		if (vvi<-1000.0 && vvi>-6000.0) angle=-47.0+(vvi+1000.0)*0.01175;
		if (vvi<-6000.0) angle=-105.75;
		ow->vvitexture->setTextureRotate(Degree(-angle+90.0));

		//rpm
		float pcent=curr_truck->aeroengines[0]->getRPMpc();
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		ow->airrpm1texture->setTextureRotate(Degree(-angle));

		if (ftp>1) pcent=curr_truck->aeroengines[1]->getRPMpc(); else pcent=0;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		ow->airrpm2texture->setTextureRotate(Degree(-angle));

		if (ftp>2) pcent=curr_truck->aeroengines[2]->getRPMpc(); else pcent=0;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		ow->airrpm3texture->setTextureRotate(Degree(-angle));

		if (ftp>3) pcent=curr_truck->aeroengines[3]->getRPMpc(); else pcent=0;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		ow->airrpm4texture->setTextureRotate(Degree(-angle));

		if (curr_truck->aeroengines[0]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
		{
			Turboprop *tp=(Turboprop*)curr_truck->aeroengines[0];
			//pitch
			ow->airpitch1texture->setTextureRotate(Degree(-tp->pitch*2.0));
			//torque
			pcent=100.0*tp->indicated_torque/tp->max_torque;
			if (pcent<60.0) angle=-5.0+pcent*1.9167;
			else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
			else angle=314.0;
			ow->airtorque1texture->setTextureRotate(Degree(-angle));
		}

		if (ftp>1 && curr_truck->aeroengines[1]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
		{
			Turboprop *tp=(Turboprop*)curr_truck->aeroengines[1];
			//pitch
			ow->airpitch2texture->setTextureRotate(Degree(-tp->pitch*2.0));
			//torque
			pcent=100.0*tp->indicated_torque/tp->max_torque;
			if (pcent<60.0) angle=-5.0+pcent*1.9167;
			else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
			else angle=314.0;
			ow->airtorque2texture->setTextureRotate(Degree(-angle));
		}

		if (ftp>2 && curr_truck->aeroengines[2]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
		{
			Turboprop *tp=(Turboprop*)curr_truck->aeroengines[2];
			//pitch
			ow->airpitch3texture->setTextureRotate(Degree(-tp->pitch*2.0));
			//torque
			pcent=100.0*tp->indicated_torque/tp->max_torque;
			if (pcent<60.0) angle=-5.0+pcent*1.9167;
			else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
			else angle=314.0;
			ow->airtorque3texture->setTextureRotate(Degree(-angle));
		}

		if (ftp>3 && curr_truck->aeroengines[3]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
		{
			Turboprop *tp=(Turboprop*)curr_truck->aeroengines[3];
			//pitch
			ow->airpitch4texture->setTextureRotate(Degree(-tp->pitch*2.0));
			//torque
			pcent=100.0*tp->indicated_torque/tp->max_torque;
			if (pcent<60.0) angle=-5.0+pcent*1.9167;
			else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
			else angle=314.0;
			ow->airtorque4texture->setTextureRotate(Degree(-angle));
		}

		//starters
		if (curr_truck->aeroengines[0]->getIgnition()) ow->engstarto1->setMaterialName("tracks/engstart-on"); else ow->engstarto1->setMaterialName("tracks/engstart-off");
		if (ftp>1 && curr_truck->aeroengines[1]->getIgnition()) ow->engstarto2->setMaterialName("tracks/engstart-on"); else ow->engstarto2->setMaterialName("tracks/engstart-off");
		if (ftp>2 && curr_truck->aeroengines[2]->getIgnition()) ow->engstarto3->setMaterialName("tracks/engstart-on"); else ow->engstarto3->setMaterialName("tracks/engstart-off");
		if (ftp>3 && curr_truck->aeroengines[3]->getIgnition()) ow->engstarto4->setMaterialName("tracks/engstart-on"); else ow->engstarto4->setMaterialName("tracks/engstart-off");
	} else if (curr_truck->driveable==BOAT)
	{
		//BOAT GUI
		int fsp = curr_truck->free_screwprop;
		//throttles
		ow->bthro1->setTop(ow->thrtop+ow->thrheight*(0.5-curr_truck->screwprops[0]->getThrottle()/2.0)-1.0);
		if (fsp>1)
			ow->bthro2->setTop(ow->thrtop+ow->thrheight*(0.5-curr_truck->screwprops[1]->getThrottle()/2.0)-1.0);

		//position
		Vector3 dir=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
		dir.normalise();
		//moveBoatMapDot(curr_truck->position.x/mapsizex, curr_truck->position.z/mapsizez);
		//position

		char tmp[50]="";
		if (curr_truck->getLowestNode() != -1)
		{
			Vector3 pos = curr_truck->nodes[curr_truck->getLowestNode()].AbsPosition;
			float height =  pos.y - hfinder->getHeightAt(pos.x, pos.z);
			if (height>0.1 && height < 99.9)
			{
				sprintf(tmp, "%2.1f", height);
				ow->boat_depth_value_taoe->setCaption(tmp);
			} else
			{
				ow->boat_depth_value_taoe->setCaption("--.-");
			}
		}

		//waterspeed
		float angle=0.0;
		Vector3 hdir=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
		hdir.normalise();
		float kt=hdir.dotProduct(curr_truck->nodes[curr_truck->cameranodepos[0]].Velocity)*1.9438;
		angle=kt*4.2;
		ow->boatspeedtexture->setTextureRotate(Degree(-angle));
		ow->boatsteertexture->setTextureRotate(Degree(curr_truck->screwprops[0]->getRudder() * 170));
	}
}

void RoRFrameListener::setGravity(float value)
{
	gravity = value;
	BeamFactory::getSingleton().recalcGravityMasses();
}


// Constructor takes a RenderWindow because it uses that to determine input context
RoRFrameListener::RoRFrameListener(AppState *parentState, RenderWindow* win, Camera* cam, SceneManager* scm, Root* root, bool isEmbedded, Ogre::String inputhwnd) :
	clutch(0),
	collisions(0),
	dashboard(0),
	debugCollisions(false),
	dof(0),
	editor(0),
	editorfd(0),
	envmap(0),
	flaresMode(3), // on by default
	forcefeedback(0),
	freeTruckPosition(false),
	free_localizer(0),
	hdrListener(0),
	heathaze(0),
	hidegui(false),
	initialized(false),
	inputhwnd(inputhwnd),
	isEmbedded(isEmbedded),
	joyshiftlock(0),
	loadedTerrain("none"),
	loading_state(NONE_LOADED),
	mCamera(cam),
	mCollisionTools(0),
	mRoot(root),
	mSceneMgr(scm),
	mStatsOn(0),
	mTimeUntilNextToggle(0),
	mTruckInfoOn(false),
	mWindow(win),
	mapsizex(3000),
	mapsizey(1000),
	mapsizez(3000),
	mtc(0),
	net(0),
	netChat(0),
	netPointToUID(-1),
	net_quality(0),
	net_quality_changed(false),
	netcheckGUITimer(0),
	objcounter(0),
	objectCounter(0),
	ow(0),
	parentState(parentState),
	person(0),
	persostart(Vector3(0,0,0)),
	pressure_pressed(false),
	raceStartTime(-1),
	reload_box(0),
	reload_pos(Vector3::ZERO),
	road(0),
	rtime(0),
	shaderSchemeMode(1),
	surveyMap(0),
	surveyMapMode(SURVEY_MAP_NONE),
	terrainFileHash(""),
	terrainFileName(""),
	terrainHasTruckShop(false),
	terrainModHash(""),
	terrainName(""),
	terrainUID(""),
	truck_preload_num(0),
	w(0),
	terrainManager(0)
{
	RoRFrameListener::eflsingleton = this;

	pthread_mutex_init(&mutex_data, NULL);

	// we don't use overlays in embedded mode
	if (!isEmbedded)
	{
		ow = new OverlayWrapper(win);
	}

	enablePosStor = BSETTING("Position Storage", false);

#ifdef USE_MYGUI
	// init GUI
	new SceneMouse(scm);
	new GUIManager(root, scm, win);
	// create console, must be done early
	new Console();

	LoadingWindow::getSingleton();
	SelectorWindow::getSingleton();
	// create main menu :D
	if (!isEmbedded)
	{
		new GUI_MainMenu(this);
		GUI_Friction::getSingleton();
	}

	if (!BSETTING("REPO_MODE", false))
	{
		MyGUI::VectorWidgetPtr v = MyGUI::LayoutManager::getInstance().loadLayout("wallpaper.layout");
		// load random image in the wallpaper
		String randomWallpaper = GUIManager::getRandomWallpaperImage();
		if (!v.empty() && !randomWallpaper.empty())
		{
			MyGUI::Widget *mainw = v.at(0);
			if (mainw)
			{
				MyGUI::ImageBox *img = (MyGUI::ImageBox *)(mainw->getChildAt(0));
				if (img) img->setImageTexture(randomWallpaper);
			}
		}
	}
#endif //MYGUI

#ifdef USE_OIS_G27
	leds = 0;
#endif // USE_OIS_G27
#ifdef USE_OIS_G27
	leds = INPUTENGINE.getLogitechLEDsDevice();
#endif //OIS_G27

#ifdef USE_MPLATFORM
	mplatform = new MPlatform_FD();
	if (mplatform) mplatform->connect();
#endif

#ifdef USE_ANGELSCRIPT
	new ScriptEngine(this, 0);
	// print some log message so we know angelscript is alive :)
	ScriptEngine::getSingleton().scriptLog->logMessage("ScriptEngine running");
#endif

#ifdef HAS_EDITOR
	trucked = 0;
#endif

	gameStartTime = getTimeStamp();

	//network
	netmode = BSETTING("Network enable", false);

	if (ow)
	{
		// setup direction arrow
		Entity *arrent = mSceneMgr->createEntity("dirArrowEntity", "arrow2.mesh");
	#if OGRE_VERSION<0x010602
		arrent->setNormaliseNormals(true);
	#endif //OGRE_VERSION
		// Add entity to the scene node
		dirArrowNode= new SceneNode(mSceneMgr);
		dirArrowNode->attachObject(arrent);
		dirArrowNode->setVisible(false);
		dirArrowNode->setScale(0.1, 0.1, 0.1);
		dirArrowNode->setPosition(Vector3(-0.6, +0.4, -1));
		dirArrowNode->setFixedYawAxis(true, Vector3::UNIT_Y);
		dirvisible = false;
		dirArrowPointed = Vector3::ZERO;
		ow->directionOverlay->add3D(dirArrowNode);
	}

	INPUTENGINE.setupDefault(win, inputhwnd);

	// setup particle manager
	new DustManager(mSceneMgr);

	if (BSETTING("regen-cache-only", false))
	{
		CACHE.startup(scm, true);
		UTFString str = _L("Cache regeneration done.\n");
		if (CACHE.newFiles > 0)     str = str + TOUTFSTRING(CACHE.newFiles) + _L(" new files\n");
		if (CACHE.changedFiles > 0) str = str + TOUTFSTRING(CACHE.changedFiles) + _L(" changed files\n");
		if (CACHE.deletedFiles > 0) str = str + TOUTFSTRING(CACHE.deletedFiles) + _L(" deleted files\n");
		if (CACHE.newFiles + CACHE.changedFiles + CACHE.deletedFiles == 0) str = str + _L("no changes");
		str = str + _L("\n(These stats can be imprecise)");
		showError(_L("Cache regeneration done"), str);
		exit(0);
	}

	CACHE.startup(mSceneMgr);

	screenWidth=win->getWidth();
	screenHeight=win->getHeight();

	windowResized(win);
	RoRWindowEventUtilities::addWindowEventListener(win, this);

	debugCollisions = BSETTING("Debug Collisions", false);

	// get lights mode
	String lightsMode = SSETTING("Lights", "Only current vehicle, main lights");
	if (lightsMode == "None (fastest)")
		flaresMode = 0;
	else if (lightsMode == "No light sources")
		flaresMode = 1;
	else if (lightsMode == "Only current vehicle, main lights")
		flaresMode = 2;
	else if (lightsMode == "All vehicles, main lights")
		flaresMode = 3;
	else if (lightsMode == "All vehicles, all lights")
		flaresMode = 4;

	// heathaze effect
	if (BSETTING("HeatHaze", false))
	{
		heathaze = new HeatHaze(scm, win,cam);
		heathaze->setEnable(true);
	}

	// depth of field effect
	if (BSETTING("DOF", false))
	{
		dof = new DOFManager(mSceneMgr, mCamera->getViewport(), mRoot, mCamera);
		dof->setEnabled(true);
	}

	// force feedback
	if (BSETTING("Force Feedback", true))
	{
		//check if a device has been detected
		if (INPUTENGINE.getForceFeedbackDevice())
		{
			//retrieve gain values
			float ogain   = FSETTING("Force Feedback Gain",      100) / 100.0f;
			float stressg = FSETTING("Force Feedback Stress",    100) / 100.0f;
			float centg   = FSETTING("Force Feedback Centering", 0  ) / 100.0f;
			float camg    = FSETTING("Force Feedback Camera",    100) / 100.0f;

			forcefeedback = new ForceFeedback(INPUTENGINE.getForceFeedbackDevice(), ogain, stressg, centg, camg);
		}
	}

	String screenshotFormatString = SSETTING("Screenshot Format", "jpg (smaller, default)");
	if     (screenshotFormatString == "jpg (smaller, default)")
		strcpy(screenshotformat, "jpg");
	else if (screenshotFormatString =="png (bigger, no quality loss)")
		strcpy(screenshotformat, "png");
	else
		strncpy(screenshotformat, screenshotFormatString.c_str(), 10);

	//Joystick
	/*
	float deadzone=0.1;
	String deadzone_string = SSETTING("Controler Deadzone");
	if (deadzone_string != String("")) {
		deadzone = atof(deadzone_string.c_str());
	}
	*/
	//joy=new BeamJoystick(mInputManager, deadzone, useforce, &cfg);
	//useforce=joy->hasForce();

	// check command line args
	String cmd = SSETTING("cmdline CMD", "");
	String cmdAction = "";
	String cmdServerIP = "";
	String modName = "";
	long cmdServerPort = 0;
	Vector3 spawnLocation = Vector3::ZERO;
	if (!cmd.empty())
	{
		Ogre::StringVector str = StringUtil::split(cmd, "/");
		// process args now
		for(Ogre::StringVector::iterator it = str.begin(); it!=str.end(); it++)
		{

			String argstr = *it;
			Ogre::StringVector args = StringUtil::split(argstr, ":");
			if (args.size()<2) continue;
			if (args[0] == "action" && args.size() == 2) cmdAction = args[1];
			if (args[0] == "serverpass" && args.size() == 2) SETTINGS.setSetting("Server password", args[1]);
			if (args[0] == "modname" && args.size() == 2) modName = args[1];
			if (args[0] == "ipport" && args.size() == 3)
			{
				cmdServerIP = args[1];
				cmdServerPort = StringConverter::parseLong(args[2]);
			}
			if (args[0] == "loc" && args.size() == 4)
			{
				spawnLocation = Vector3(StringConverter::parseInt(args[1]), StringConverter::parseInt(args[2]), StringConverter::parseInt(args[3]));
				SETTINGS.setSetting("net spawn location", TOSTRING(spawnLocation));
			}
		}
	}

	if (cmdAction == "regencache") SETTINGS.setSetting("regen-cache-only", "Yes");
	if (cmdAction == "installmod")
	{
		// use modname!
	}

	// check if we enable netmode based on cmdline
	if (!netmode && cmdAction == "joinserver")
		netmode = true;

	// preselected map or truck?
	String preselected_map = SSETTING("Preselected Map", "");
	String preselected_truck = SSETTING("Preselected Truck", "");
	String preselected_truckConfig = SSETTING("Preselected TruckConfig", "");
	bool enterTruck = (BSETTING("Enter Preselected Truck", false));

	if (preselected_map != "") LOG("Preselected Map: " + (preselected_map));
	if (preselected_truck != "") LOG("Preselected Truck: " + (preselected_truck));
	if (preselected_truckConfig != "") LOG("Preselected Truck Config: " + (preselected_truckConfig));

	//LOG("huette debug 1");

	// initiate player colours
	PlayerColours::getSingleton();

	// you always need that, even if you are not using the network
	NetworkStreamManager::getSingleton();

	// new factory for characters, net is INVALID, will be set later
	new CharacterFactory(cam, 0, collisions, hfinder, w, surveyMap, mSceneMgr);
	new ChatSystemFactory(0);

	// notice: all factories must be available before starting the network!
#ifdef USE_SOCKETW
	if (netmode)
	{
		// cmdline overrides config
		std::string sname = SSETTING("Server name", "").c_str();
		if (cmdAction == "joinserver" && !cmdServerIP.empty())
			sname = cmdServerIP;

		long sport = ISETTING("Server port", 1337);
		if (cmdAction == "joinserver" && cmdServerPort)
			sport = cmdServerPort;

		if (sport==0)
		{
			showError(_L("A network error occured"), _L("Bad server port"));
			exit(123);
			return;
		}
		LOG("trying to join server '" + String(sname) + "' on port " + TOSTRING(sport) + "'...");

#ifdef USE_MYGUI
		LoadingWindow::getSingleton().setAutotrack(_L("Trying to connect to server ..."));
#endif // USE_MYGUI
		// important note: all new network code is written in order to allow also the old network protocol to further exist.
		// at some point you need to decide with what type of server you communicate below and choose the correct class

		net = new Network(sname, sport, this);

		bool connres = net->connect();
#ifdef USE_MYGUI
		LoadingWindow::getSingleton().hide();

#ifdef USE_SOCKETW
		new GUI_Multiplayer(net, cam);
		GUI_Multiplayer::getSingleton().update();
#endif //USE_SOCKETW

#endif //USE_MYGUI
		if (!connres)
		{
			LOG("connection failed. server down?");
			showError(_L("Unable to connect to server"), _L("Unable to connect to the server. It is certainly down or you have network problems."));
			//fatal
			exit(1);
		}
		char *terrn = net->getTerrainName();
		bool isAnyTerrain = (terrn && !strcmp(terrn, "any"));
		if (preselected_map.empty() && isAnyTerrain)
		{
			// so show the terrain selection
			preselected_map = "";
		} else if (!isAnyTerrain)
		{
			preselected_map = getASCIIFromCharString(terrn, 255);
		}

		// create person _AFTER_ network, important
		int colourNum = 0;
		if (net->getLocalUserData()) colourNum = net->getLocalUserData()->colournum;
		person = (Character *)CharacterFactory::getSingleton().createLocal(colourNum);

		// network chat stuff
		netChat = ChatSystemFactory::getSingleton().createLocal(colourNum);

#ifdef USE_MYGUI
		Console *c = Console::getSingletonPtrNoCreation();
		if (c)
		{
			c->setVisible(true);
			c->setNetChat(netChat);
			wchar_t tmp[255] = L"";
			UTFString format = _L("Press %ls to start chatting");
			swprintf(tmp, 255, format.asWStr_c_str(), ANSI_TO_WCHAR(INPUTENGINE.getKeyForCommand(EV_COMMON_ENTER_CHATMODE)).c_str());
			c->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, UTFString(tmp), "information.png");
		}
#endif //USE_MYGUI

#ifdef USE_MUMBLE
		new MumbleIntegration();
#endif // USE_MUMBLE

	} else
#endif //SOCKETW
	
	// no network
	person = (Character *)CharacterFactory::getSingleton().createLocal(-1);
	
	// init camera manager after mygui and after we have a character
	new CameraManager(mSceneMgr, mCamera, this, person, ow, dof);

	person->setVisible(false);

	// load guy
	int source=-1;
#ifdef USE_SOCKETW
	if (net)
		source = net->getUserID();
#endif //SOCKETW

	// new beam factory
	new BeamFactory(mSceneMgr, mSceneMgr->getRootSceneNode(), mWindow, net, &mapsizex, &mapsizez, collisions, hfinder, w, mCamera);


	// now continue to load everything...
	if (!preselected_map.empty())
	{
		if (!CACHE.checkResourceLoaded(preselected_map))
		{
			preselected_map  = preselected_map + ".terrn";
			// fallback to old terrain name with .terrn
			if (!CACHE.checkResourceLoaded(preselected_map))
			{
				LOG("Terrain not found: " + preselected_map);
				showError(_L("Terrain loading error"), _L("Terrain not found: ") + preselected_map);
				exit(123);
			}
		}

		// set the terrain cache entry
		Cache_Entry ce = CACHE.getResourceInfo(preselected_map);
		terrainUID = ce.uniqueid;

		loadTerrain(preselected_map);

		if (preselected_truck.empty())
		{
			if (truck_preload_num == 0 && (!netmode || !terrainHasTruckShop))
			{
#ifdef USE_MYGUI
				// show truck selector
				if (w)
				{
					hideMap();
					SelectorWindow::getSingleton().show(SelectorWindow::LT_NetworkWithBoat);
				} else
				{
					hideMap();
					SelectorWindow::getSingleton().show(SelectorWindow::LT_Network);
				}
#endif // USE_MYGUI
			} else
			{
				// init no trucks
				initTrucks(false, preselected_map);
			}
		} else if (!preselected_truck.empty() && preselected_truck != "none")
		{
			// load preselected truck
			loading_state = TERRAIN_LOADED;
			initTrucks(true, preselected_truck, "", &std::vector<String>(1, preselected_truckConfig), enterTruck);
		}
	} else
	{
#ifdef USE_MYGUI
		// show terrain selector
		hideMap();
		SelectorWindow::getSingleton().show(SelectorWindow::LT_Terrain);
#endif // USE_MYGUI
	}

	initialized=true;
}

RoRFrameListener::~RoRFrameListener()
{
#ifdef USE_MYGUI
	LoadingWindow::freeSingleton();
	SelectorWindow::freeSingleton();
#endif //MYGUI

//	if (joy) delete (joy);
#ifdef USE_PAGED
	for(std::vector<paged_geometry_t>::iterator it=pagedGeometry.begin(); it!=pagedGeometry.end(); it++)
	{
		if (it->geom)
		{
			delete(it->geom);
			it->geom=0;
		}
		if (it->loader)
		{
			delete((TreeLoader2D *)it->loader);
			it->loader=0;
		}
	}
#endif
#ifdef USE_SOCKETW
	if (net) delete (net);
#endif //SOCKETW
	//we should destroy OIS here
	//we could also try to destroy SoundScriptManager, but we don't care!
	#ifdef USE_MPLATFORM
	if (mplatform)
	{
		if (mplatform->disconnect()) delete(mplatform);
	}
	#endif
}

void RoRFrameListener::loadNetTerrain(char *preselected_map)
{
	// load preselected map
	char mapname[1024];
	String group="";
	sprintf(mapname, "%s.terrn", preselected_map);
	loadTerrain(mapname);
	//miniature map stuff
	MaterialPtr tmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/Map"));
	//search if mini picture exists
	char ppname[1024];
	sprintf(ppname, "%s-mini.dds", preselected_map);
	group="";
	if (group == "")
	{
		sprintf(ppname, "%s-mini.png", preselected_map);
		try
		{
			group = ResourceGroupManager::getSingleton().findGroupContainingResource(ppname);
		}catch(...)
		{
		}
	}
	if (group == "")
		return;
	FileInfoListPtr files= ResourceGroupManager::getSingleton().findResourceFileInfo(group, ppname);
	if (!files->empty())
	{
		tmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(ppname);
	}
	else
	{
		tmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("unknown.dds");
	}
}


void RoRFrameListener::unloadObject(const char* instancename)
{
	if (loadedObjects.find(std::string(instancename)) == loadedObjects.end())
	{
		LOG("unable to unload object: " + std::string(instancename));
		return;
	}

	loaded_object_t obj = loadedObjects[std::string(instancename)];

	// check if it was already deleted
	if (!obj.enabled)
		return;

	// unload any collision tris
	if (obj.collTris.size() > 0)
	{
		for(std::vector<int>::iterator it = obj.collTris.begin(); it != obj.collTris.end(); it++)
		{
			collisions->removeCollisionTri(*it);
		}
	}
	
	// and any collision boxes
	if (obj.collBoxes.size() > 0)
	{
		for(std::vector<int>::iterator it = obj.collBoxes.begin(); it != obj.collBoxes.end(); it++)
		{
			collisions->removeCollisionBox(*it);
		}
	}

	obj.sceneNode->detachAllObjects();
	obj.sceneNode->setVisible(false);
	obj.enabled = false;
}

void RoRFrameListener::loadObject(const char* name, float px, float py, float pz, float rx, float ry, float rz, SceneNode * bakeNode, const char* instancename, bool enable_collisions, int scripthandler, const char *type, bool uniquifyMaterial)
{
	ScopeLog log("object_"+String(name));
	if (type && !strcmp(type, "grid"))
	{
		// some fast grid object hacks :)
		for(int x=0;x<500;x+=50)
			for(int z=0;z<500;z+=50)
				loadObject(name, px+x, py, pz+z, rx, ry, rz, bakeNode, 0, enable_collisions, scripthandler, 0);
		return;
	}

	// nice idea, but too many random hits
	//if (abs(rx+1) < 0.001) rx = Math::RangeRandom(0, 360);
	//if (abs(ry+1) < 0.001) ry = Math::RangeRandom(0, 360);
	//if (abs(rz+1) < 0.001) rz = Math::RangeRandom(0, 360);

	if (strnlen(name, 250)==0)
		return;

	//FILE *fd;
	char fname[1024] = {};
	char oname[1024] = {};
	char mesh[1024] = {};
	char line[1024] = {};
	char collmesh[1024] = {};
	float scx = 0, scy = 0, scz = 0;
	float lx = 0, hx = 0, ly = 0, hy = 0, lz = 0, hz = 0;
	float srx = 0, sry = 0, srz = 0;
	float drx = 0, dry = 0, drz = 0;
	float fcx = 0, fcy = 0, fcz = 0;
	bool forcecam=false;
	bool ismovable=false;

	int event_filter = EVENT_ALL;
	Quaternion rotation = Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);

	// try to load with UID first!
	String odefgroup = "";
	String odefname = "";
	bool odefFound = false;
	if (terrainUID != "" && !CACHE.stringHasUID(name))
	{
		sprintf(fname,"%s-%s.odef", terrainUID.c_str(), name);
		odefname = String(fname);
		bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(odefname);
		if (exists)
		{	
			odefgroup = ResourceGroupManager::getSingleton().findGroupContainingResource(odefname);
			odefFound = true;
		}
	}

	if (!odefFound)
	{
		sprintf(fname,"%s.odef", name);
		odefname = String(fname);
		bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(odefname);
		if (exists)
		{
			odefgroup = ResourceGroupManager::getSingleton().findGroupContainingResource(odefname);
			odefFound = true;
		}
	}


	if (!CACHE.checkResourceLoaded(odefname, odefgroup))
	if (!odefFound)
	{
		LOG("Error while loading Terrain: could not find required .odef file: " + odefname + ". Ignoring entry.");
		return;
	}

	DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource(odefname, odefgroup);

	ds->readLine(mesh, 1023);
	if (String(mesh) == "LOD")
	{
		// LOD line is obsolete
		ds->readLine(mesh, 1023);
	}

	//scale
	ds->readLine(line, 1023);
	sscanf(line, "%f, %f, %f",&scx,&scy,&scz);
	sprintf(oname,"object%i(%s)", objcounter,name);
	objcounter++;


	SceneNode *tenode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	bool background_loading = BSETTING("Background Loading", false);

	MeshObject *mo = NULL;
	if (String(mesh) != "none")
		mo = new MeshObject(mSceneMgr, mesh, oname, tenode, NULL, background_loading);
	
	//mo->setQueryFlags(OBJECTS_MASK);
	//tenode->attachObject(te);
	tenode->setScale(scx,scy,scz);
	tenode->setPosition(px,py,pz);
	tenode->rotate(rotation);
	tenode->pitch(Degree(-90));
	tenode->setVisible(true);

	// register in map
	loaded_object_t *obj = &loadedObjects[std::string(instancename)];
	obj->instanceName = std::string(instancename);
	obj->loadType     = 0;
	obj->enabled      = true;
	obj->sceneNode    = tenode;
	obj->collTris.clear();


	if (mo && uniquifyMaterial && instancename)
	{
		for(unsigned int i = 0; i < mo->getEntity()->getNumSubEntities(); i++)
		{
			SubEntity *se = mo->getEntity()->getSubEntity(i);
			String matname = se->getMaterialName();
			String newmatname = matname + "/" + String(instancename);
			//LOG("subentity " + TOSTRING(i) + ": "+ matname + " -> " + newmatname);
			se->getMaterial()->clone(newmatname);
			se->setMaterialName(newmatname);
		}
	}

	//String meshGroup = ResourceGroupManager::getSingleton().findGroupContainingResource(mesh);
	//MeshPtr mainMesh = mo->getMesh();

	//collision box(es)
	bool virt=false;
	bool rotating=false;
	bool classic_ref=true;
	// everything is of concrete by default
	ground_model_t *gm = collisions->getGroundModelByString("concrete");
	char eventname[256];
	eventname[0]=0;
	while (!ds->eof())
	{
		size_t ll=ds->readLine(line, 1023);

		// little workaround to trim it
		String lineStr = String(line);
		Ogre::StringUtil::trim(lineStr);

		const char* ptline = lineStr.c_str();
		if (ll==0 || line[0]=='/' || line[0]==';') continue;

		if (!strcmp("end",ptline)) break;
		if (!strcmp("movable", ptline)) {ismovable=true;continue;};
		if (!strcmp("localizer-h", ptline))
		{
			localizers[free_localizer].position=Vector3(px,py,pz);
			localizers[free_localizer].rotation=rotation;
			localizers[free_localizer].type=Autopilot::LOCALIZER_HORIZONTAL;
			free_localizer++;
			continue;
		}
		if (!strcmp("localizer-v", ptline))
		{
			localizers[free_localizer].position=Vector3(px,py,pz);
			localizers[free_localizer].rotation=rotation;
			localizers[free_localizer].type=Autopilot::LOCALIZER_VERTICAL;
			free_localizer++;
			continue;
		}
		if (!strcmp("localizer-ndb", ptline))
		{
			localizers[free_localizer].position=Vector3(px,py,pz);
			localizers[free_localizer].rotation=rotation;
			localizers[free_localizer].type=Autopilot::LOCALIZER_NDB;
			free_localizer++;
			continue;
		}
		if (!strcmp("localizer-vor", ptline))
		{
			localizers[free_localizer].position=Vector3(px,py,pz);
			localizers[free_localizer].rotation=rotation;
			localizers[free_localizer].type=Autopilot::LOCALIZER_VOR;
			free_localizer++;
			continue;
		}
		if (!strcmp("standard", ptline)) {classic_ref=false;tenode->pitch(Degree(90));continue;};
		if (!strncmp("sound", ptline, 5))
		{
#ifdef USE_OPENAL
			if (!SoundScriptManager::getSingleton().isDisabled())
			{
				char tmp[255]="";
				sscanf(ptline, "sound %s", tmp);
				SoundScriptInstance *sound = SoundScriptManager::getSingleton().createInstance(tmp, MAX_TRUCKS+1, tenode);
				sound->setPosition(tenode->getPosition(), Vector3::ZERO);
				sound->start();
			}
#endif //USE_OPENAL
			continue;
		}
		if (!strcmp("beginbox", ptline) || !strcmp("beginmesh", ptline))
		{
			drx=dry=drz=0.0;
			rotating=false;
			virt=false;
			forcecam=false;
			event_filter=EVENT_NONE;
			eventname[0]=0;
			collmesh[0]=0;
			gm = collisions->getGroundModelByString("concrete");
			continue;
		};
		if (!strncmp("boxcoords", ptline, 9))
		{
			sscanf(ptline, "boxcoords %f, %f, %f, %f, %f, %f",&lx,&hx,&ly,&hy,&lz, &hz);
			continue;
		}
		if (!strncmp("mesh", ptline, 4))
		{
			sscanf(ptline, "mesh %s",collmesh);
			continue;
		}
		if (!strncmp("rotate", ptline, 6))
		{
			sscanf(ptline, "rotate %f, %f, %f",&srx, &sry, &srz);
			rotating=true;
			continue;
		}
		if (!strncmp("forcecamera", ptline, 11))
		{
			sscanf(ptline, "forcecamera %f, %f, %f",&fcx, &fcy, &fcz);
			forcecam=true;
			continue;
		}
		if (!strncmp("direction", ptline, 9))
		{
			sscanf(ptline, "direction %f, %f, %f",&drx, &dry, &drz);
			continue;
		}
		if (!strncmp("frictionconfig", ptline, 14) && strlen(ptline) > 15)
		{
			// load a custom friction config
			collisions->loadGroundModelsConfigFile(String(ptline + 15));
			continue;
		}
		if ((!strncmp("stdfriction", ptline, 11) || !strncmp("usefriction", ptline, 11)) && strlen(ptline) > 12)
		{
			String modelName = String(ptline + 12);
			gm = collisions->getGroundModelByString(modelName);
			continue;
		}
		if (!strcmp("virtual", ptline)) {virt=true;continue;};
		if (!strncmp("event", ptline, 5))
		{
			char ts[256];
			ts[0]=0;
			sscanf(ptline, "event %s %s",eventname, ts);
			if (!strncmp(ts, "avatar", 6))
				event_filter=EVENT_AVATAR;
			else if (!strncmp(ts, "truck", 5))
				event_filter=EVENT_TRUCK;
			else if (!strncmp(ts, "airplane", 8))
				event_filter=EVENT_AIRPLANE;
			else if (!strncmp(ts, "delete", 8))
				event_filter=EVENT_DELETE;
			
			if (!strncmp(ts, "shoptruck", 9))
				terrainHasTruckShop=true;

			// fallback
			if (strlen(ts) == 0)
				event_filter=EVENT_ALL;

			continue;
		}
		//resp=sscanf(ptline, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %c",&lx,&hx,&ly, &hy,&lz, &hz, &srx, &sry, &srz,&type);
		if (!strcmp("endbox", ptline))
		{
			if (enable_collisions)
			{
				int boxnum = collisions->addCollisionBox(tenode, rotating, virt,px,py,pz,rx,ry,rz,lx,hx,ly,hy,lz,hz,srx,sry,srz,eventname, instancename, forcecam, Vector3(fcx, fcy, fcz), scx, scy, scz, drx, dry, drz, event_filter, scripthandler);
				obj->collBoxes.push_back((boxnum));
			}
			continue;
		}
		if (!strcmp("endmesh", ptline))
		{
			collisions->addCollisionMesh(collmesh, Vector3(px,py,pz), tenode->getOrientation(), Vector3(scx, scy, scz), gm, &(obj->collTris));
			continue;
		}

		if (!strncmp("particleSystem", ptline, 14) && tenode)
		{
			float x=0, y=0, z=0, scale=0;
			char pname[255]="", sname[255]="";
			int res = sscanf(ptline, "particleSystem %f, %f, %f, %f, %s %s", &scale, &x, &y, &z, pname, sname);
			if (res != 6) continue;

			// hacky: prevent duplicates
			String paname = String(pname);
			while(mSceneMgr->hasParticleSystem(paname))
				paname += "_";

			// create particle system
			ParticleSystem* pParticleSys = mSceneMgr->createParticleSystem(paname, String(sname));
			pParticleSys->setCastShadows(false);
			pParticleSys->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap

			// Some affectors may need its instance name (e.g. for script feedback purposes)
#ifdef USE_ANGELSCRIPT
			unsigned short affCount = pParticleSys->getNumAffectors();
			ParticleAffector* pAff;
			for(unsigned short i = 0; i<affCount; ++i)
			{
				pAff = pParticleSys->getAffector(i);
				if (pAff->getType()=="ExtinguishableFire")
					((ExtinguishableFireAffector*)pAff)->setInstanceName(obj->instanceName);
			}
#endif // USE_ANGELSCRIPT

			SceneNode *sn = tenode->createChildSceneNode();
			sn->attachObject(pParticleSys);
			sn->pitch(Degree(90));
			continue;
		}

		if (!strncmp("setMeshMaterial", ptline, 15))
		{
			char mat[256]="";
			sscanf(ptline, "setMeshMaterial %s", mat);
			if (mo->getEntity() && strnlen(mat,250)>0)
			{
				mo->getEntity()->setMaterialName(String(mat));
				// load it
				//MaterialManager::getSingleton().load(String(mat), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			}
			continue;
		}
		if (!strncmp("generateMaterialShaders", ptline, 23))
		{
			char mat[256]="";
			sscanf(ptline, "generateMaterialShaders %s", mat);
			if (BSETTING("Use RTShader System", false))
			{
				Ogre::RTShader::ShaderGenerator::getSingleton().createShaderBasedTechnique(String(mat), Ogre::MaterialManager::DEFAULT_SCHEME_NAME, Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
				Ogre::RTShader::ShaderGenerator::getSingleton().invalidateMaterial(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, String(mat));
			}

			continue;
		}
		if (!strncmp("playanimation", ptline, 13) && mo)
		{
			char animname[256]="";
			float speedfactorMin = 0, speedfactorMax = 0;
			sscanf(ptline, "playanimation %f, %f, %s", &speedfactorMin, &speedfactorMax, animname);
			if (tenode && mo->getEntity() && strnlen(animname,250)>0)
			{
				AnimationStateSet *s = mo->getEntity()->getAllAnimationStates();
				if (!s->hasAnimationState(String(animname)))
				{
					LOG("ODEF: animation '" + String(animname) + "' for mesh: '" + String(mesh) + "' in odef file '" + String(name) + ".odef' not found!");
					continue;
				}
				animated_object_t ao;
				ao.node = tenode;
				ao.ent = mo->getEntity();
				ao.speedfactor = speedfactorMin;
				if (speedfactorMin != speedfactorMax)
					ao.speedfactor = Math::RangeRandom(speedfactorMin, speedfactorMax);
				ao.anim = 0;
				try
				{
					ao.anim = mo->getEntity()->getAnimationState(String(animname));
				} catch (...)
				{
					ao.anim = 0;
				}
				if (!ao.anim)
				{
					LOG("ODEF: animation '" + String(animname) + "' for mesh: '" + String(mesh) + "' in odef file '" + String(name) + ".odef' not found!");
					continue;
				}
				ao.anim->setEnabled(true);
				animatedObjects.push_back(ao);
			}
			continue;
		}
		if (!strncmp("drawTextOnMeshTexture", ptline, 21) && mo)
		{
			if (!mo->getEntity())
				continue;
			String matName = mo->getEntity()->getSubEntity(0)->getMaterialName();
			MaterialPtr m = MaterialManager::getSingleton().getByName(matName);
			if (m.getPointer() == 0)
			{
				LOG("ODEF: problem with drawTextOnMeshTexture command: mesh material not found: "+String(fname)+" : "+String(ptline));
				continue;
			}
			String texName = m->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName();
			Texture* background = (Texture *)TextureManager::getSingleton().getByName(texName).getPointer();
			if (!background)
			{
				LOG("ODEF: problem with drawTextOnMeshTexture command: mesh texture not found: "+String(fname)+" : "+String(ptline));
				continue;
			}

			static int textureNumber = 0;
			textureNumber++;
			char tmpTextName[256]="", tmpMatName[256]="";
			sprintf(tmpTextName, "TextOnTexture_%d_Texture", textureNumber);
			sprintf(tmpMatName, "TextOnTexture_%d_Material", textureNumber);			// Make sure the texture is not WRITE_ONLY, we need to read the buffer to do the blending with the font (get the alpha for example)
			TexturePtr texture = TextureManager::getSingleton().createManual(tmpTextName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, (Ogre::uint)background->getWidth(), (Ogre::uint)background->getHeight(), MIP_UNLIMITED , PF_X8R8G8B8, Ogre::TU_STATIC|Ogre::TU_AUTOMIPMAP, new ResourceBuffer());
			if (texture.getPointer() == 0)
			{
				LOG("ODEF: problem with drawTextOnMeshTexture command: could not create texture: "+String(fname)+" : "+String(ptline));
				continue;
			}

			float x=0, y=0, w=0, h=0;
			float a=0, r=0, g=0, b=0;
			char fontname[256]="";
			char text[256]="";
			char option='l';
			int res = sscanf(ptline, "drawTextOnMeshTexture %f, %f, %f, %f, %f, %f, %f, %f, %c, %s %s", &x, &y, &w, &h, &r, &g, &b, &a, &option, fontname, text);
			if (res < 11)
			{
				LOG("ODEF: problem with drawTextOnMeshTexture command: "+String(fname)+" : "+String(ptline));
				continue;
			}

			// cehck if we got a template argument
			if (!strncmp(text, "{{argument1}}", 13))
				strncpy(text, instancename, 250);

			// replace '_' with ' '
			char *text_pointer = text;
			while (*text_pointer!=0) {if (*text_pointer=='_') *text_pointer=' ';text_pointer++;};

			Font* font = (Font *)FontManager::getSingleton().getByName(String(fontname)).getPointer();
			if (!font)
			{
				LOG("ODEF: problem with drawTextOnMeshTexture command: font not found: "+String(fname)+" : "+String(ptline));
				continue;
			}


			//Draw the background to the new texture
			texture->getBuffer()->blit(background->getBuffer());

			x = background->getWidth() * x;
			y = background->getHeight() * y;
			w = background->getWidth() * w;
			h = background->getHeight() * h;

			Image::Box box = Image::Box((size_t)x, (size_t)y, (size_t)(x+w), (size_t)(y+h));
			WriteToTexture(String(text), texture, box, font, ColourValue(r, g, b, a), option);

			// we can save it to disc for debug purposes:
			//SaveImage(texture, "test.png");

			m->clone(tmpMatName);
			MaterialPtr mNew = MaterialManager::getSingleton().getByName(tmpMatName);
			mNew->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(tmpTextName);

			mo->getEntity()->setMaterialName(String(tmpMatName));
			continue;
		}

		LOG("ODEF: unknown command in "+String(fname)+" : "+String(ptline));
	}

	//add icons if type is set
#ifdef USE_MYGUI
	String typestr = "";
	if (type && surveyMap)
	{
		typestr = String(type);
		// hack for raceways
		if (!strcmp(name, "chp-checkpoint"))
			typestr = "checkpoint";
		if (!strcmp(name, "chp-start"))
			typestr = "racestart";
		if (!strncmp(name, "road", 4))
			typestr = "road";

		if (typestr != String("") && typestr != String("road") && typestr != String("sign"))
		{
			MapEntity *e = surveyMap->createMapEntity(typestr);
			if (e)
			{
				e->setVisibility(true);
				e->setPosition(px, pz);
				e->setRotation(Radian(ry));

				if (String(name) != String(""))
					e->setDescription(String(instancename));
			}
		}
	}
#endif //USE_MYGUI
}

void updateCruiseControl(Beam* curr_truck, float dt)
{
	if (INPUTENGINE.getEventValue(EV_TRUCK_BRAKE) > 0.05f ||
		INPUTENGINE.getEventValue(EV_TRUCK_MANUAL_CLUTCH) > 0.05f ||
		(curr_truck->cc_target_speed < curr_truck->cc_target_speed_lower_limit) ||
		(curr_truck->parkingbrake && curr_truck->engine->getGear() > 0) ||
		!curr_truck->engine->isRunning() ||
		!curr_truck->engine->hasContact())
	{
		curr_truck->cruisecontrolToggle();
		return;
	}

	if (curr_truck->engine->getGear() > 0)
	{
		// Try to maintain the target speed
		if (curr_truck->cc_target_speed > curr_truck->WheelSpeed)
		{
			float accl = (curr_truck->cc_target_speed - curr_truck->WheelSpeed) * 2.0f;
			accl = std::max(curr_truck->engine->getAcc(), accl);
			accl = std::min(accl, 1.0f);
			curr_truck->engine->setAcc(accl);
		}
	} else if (curr_truck->engine->getGear() == 0) // out of gear
	{
		// Try to maintain the target rpm
		if (curr_truck->cc_target_rpm > curr_truck->engine->getRPM())
		{
			float accl = (curr_truck->cc_target_rpm - curr_truck->engine->getRPM()) * 0.01f;
			accl = std::max(curr_truck->engine->getAcc(), accl);
			accl = std::min(accl, 1.0f);
			curr_truck->engine->setAcc(accl);
		}
	}

	if (INPUTENGINE.getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_ACCL))
	{
		if (curr_truck->engine->getGear() > 0)
		{
			curr_truck->cc_target_speed += 5.0f * dt;
			curr_truck->cc_target_speed  = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
			if (curr_truck->sl_enabled)
			{
				curr_truck->cc_target_speed  = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
			}
		} else if (curr_truck->engine->getGear() == 0) // out of gear
		{
			curr_truck->cc_target_rpm += 1000.0f * dt;
			curr_truck->cc_target_rpm  = std::min(curr_truck->cc_target_rpm, curr_truck->engine->getMaxRPM());
		}
	}
	if (INPUTENGINE.getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_DECL))
	{
		if (curr_truck->engine->getGear() > 0)
		{
			curr_truck->cc_target_speed -= 5.0f * dt;
			curr_truck->cc_target_speed  = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
		} else if (curr_truck->engine->getGear() == 0) // out of gear
		{
			curr_truck->cc_target_rpm -= 1000.0f * dt;
			curr_truck->cc_target_rpm  = std::max(curr_truck->engine->getIdleRPM(), curr_truck->cc_target_rpm);
		}
	}
	if (INPUTENGINE.getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_READJUST))
	{
		if (curr_truck->sl_enabled)
		{
			curr_truck->cc_target_speed = std::min(curr_truck->WheelSpeed, curr_truck->sl_speed_limit);
		}
		curr_truck->cc_target_speed = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
		curr_truck->cc_target_rpm   = curr_truck->engine->getRPM();
	}

	if (curr_truck->cc_can_brake)
	{
		if (curr_truck->WheelSpeed > curr_truck->cc_target_speed + 0.5f && !INPUTENGINE.getEventValue(EV_TRUCK_ACCELERATE))
		{
			float brake = (curr_truck->WheelSpeed - curr_truck->cc_target_speed) * 0.5f;
			brake = std::min(brake, 1.0f);
			curr_truck->brake = curr_truck->brakeforce * brake;
		}
	}
}

void checkSpeedlimit(Beam* curr_truck, float dt)
{
	if (curr_truck->sl_enabled && curr_truck->engine->getGear() != 0)
	{
		float accl = (curr_truck->sl_speed_limit - std::abs(curr_truck->WheelSpeed)) * 2.0f;
		accl = std::max(0.0f, accl);
		accl = std::min(accl, curr_truck->engine->getAcc());
		curr_truck->engine->setAcc(accl);
	}
}

bool RoRFrameListener::updateEvents(float dt)
{
	if (dt==0.0f) return true;

	INPUTENGINE.updateKeyBounces(dt);
	if (!INPUTENGINE.getInputsChanged()) return true;

	bool dirty = false;
	//update joystick readings
	//	joy->UpdateInputState();

	//stick shift general uglyness
	/*
	// no more stickshift, commented out when upgrading to the inputengine
	if (loading_state==ALL_LOADED && current_truck!=-1 && trucks[current_truck]->driveable==TRUCK && trucks[current_truck]->engine->getAutoMode()==MANUAL)
	{
		int gb;
		gb=joy->updateStickShift(true, trucks[current_truck]->engine->getClutch());
		// TODO: FIXME
		//if (gb!=-1) trucks[current_truck]->engine->setGear(gb);
	}
	else joy->updateStickShift(false, 0.0);
	*/

	// update overlays if enabled
	if (ow) ow->update(dt);

	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

#ifdef USE_MYGUI
	if (GUI_Friction::getSingletonPtr() && GUI_Friction::getSingleton().getVisible() && curr_truck)
	{
		// friction GUI active
		ground_model_t *gm = curr_truck->getLastFuzzyGroundModel();
		if (gm)
			GUI_Friction::getSingleton().setActiveCol(gm);
	}
#endif //USE_MYGUI

#ifdef USE_MYGUI
	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) && !hidegui)
	{
		Console *c = Console::getSingletonPtrNoCreation();
		if (c)
		{
			INPUTENGINE.resetKeys();
			c->setVisible(true);
			c->select();
		}
	}
#endif //USE_MYGUI
	// update characters
	if (loading_state==ALL_LOADED && net)
		CharacterFactory::getSingleton().updateCharacters(dt);
	else if (loading_state==ALL_LOADED && !net)
		person->update(dt);

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
	{
		shutdown_final();
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_SCREENSHOT, 0.5f))
	{
		int mNumScreenShots=0;
		String tmpfn = SSETTING("User Path", "") + String("screenshot_") + TOSTRING(++mNumScreenShots) + String(".") + String(screenshotformat);
		while(fileExists(tmpfn.c_str()))
			tmpfn = SSETTING("User Path", "") + String("screenshot_") + TOSTRING(++mNumScreenShots) + String(".") + String(screenshotformat);

#ifdef USE_MYGUI
		MyGUI::PointerManager::getInstance().setVisible(false);
#endif // USE_MYGUI
		if (String(screenshotformat) == "png")
		{


			// add some more data into the image
			AdvancedScreen *as = new AdvancedScreen(mWindow, tmpfn);
			as->addData("terrain_Name", loadedTerrain);
			as->addData("terrain_ModHash", terrainModHash);
			as->addData("terrain_FileHash", terrainFileHash);
			as->addData("Truck_Num", TOSTRING(BeamFactory::getSingleton().getCurrentTruckNumber()));
			if (curr_truck)
			{
				as->addData("Truck_fname", curr_truck->realtruckfilename);
				as->addData("Truck_name", curr_truck->getTruckName());
				as->addData("Truck_beams", TOSTRING(curr_truck->getBeamCount()));
				as->addData("Truck_nodes", TOSTRING(curr_truck->getNodeCount()));
			}
			as->addData("User_NickName", SSETTING("Nickname", "Anonymous"));
			as->addData("User_Language", SSETTING("Language", "English"));
			as->addData("RoR_VersionString", String(ROR_VERSION_STRING));
			as->addData("RoR_VersionSVN", String(SVN_REVISION));
			as->addData("RoR_VersionSVNID", String(SVN_ID));
			as->addData("RoR_ProtocolVersion", String(RORNET_VERSION));
			as->addData("RoR_BinaryHash", SSETTING("BinaryHash", ""));
			as->addData("MP_ServerName", SSETTING("Server name", ""));
			as->addData("MP_ServerPort", SSETTING("Server port", ""));
			as->addData("MP_NetworkEnabled", SSETTING("Network enable", "No"));
			as->addData("Camera_Mode", CameraManager::singletonExists() ? TOSTRING(CameraManager::getSingleton().getCameraBehavior()) : "None");
			as->addData("Camera_Position", TOSTRING(mCamera->getPosition()));

			const RenderTarget::FrameStats& stats = mWindow->getStatistics();
			as->addData("AVGFPS", TOSTRING(stats.avgFPS));

			as->write();
			delete(as);
		} else
		{
			mWindow->update();
			mWindow->writeContentsToFile(tmpfn);
		}

#ifdef USE_MYGUI
		MyGUI::PointerManager::getInstance().setVisible(true);
#endif // USE_MYGUI

		// show new flash message
		String ssmsg = _L("wrote screenshot:") + TOSTRING(mNumScreenShots);
		LOG(ssmsg);
#ifdef USE_MYGUI
		Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "camera.png", 10000, false);
#endif //USE_MYGUI
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCKEDIT_RELOAD, 0.5f) && curr_truck)
	{
		reloadCurrentTruck();
		return true;
	}
/* -- disabled for now ... why we should check for this if it does not call anything?
   -- enable this again when truckToolGUI is available again

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_SHOWTRUCKTOOL, 0.5f) && current_truck != -1)
	{
		//if (truckToolGUI)
		//	truckToolGUI->show();
	}
*/
	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_RELOAD_ROADS, 0.5f))
	{
		if (proceduralManager)
		{
			//proceduralManager->deleteAllObjects();
			proceduralManager->updateAllObjects();
		}
	}


	// position storage
	if (enablePosStor && curr_truck)
	{
		int res = -10, slot=-1;
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS1, 0.5f)) { slot=0; res = curr_truck->savePosition(slot); };
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS2, 0.5f)) { slot=1; res = curr_truck->savePosition(slot); };
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS3, 0.5f)) { slot=2; res = curr_truck->savePosition(slot); };
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS4, 0.5f)) { slot=3; res = curr_truck->savePosition(slot); };
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS5, 0.5f)) { slot=4; res = curr_truck->savePosition(slot); };
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS6, 0.5f)) { slot=5; res = curr_truck->savePosition(slot); };
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS7, 0.5f)) { slot=6; res = curr_truck->savePosition(slot); };
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS8, 0.5f)) { slot=7; res = curr_truck->savePosition(slot); };
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS9, 0.5f)) { slot=8; res = curr_truck->savePosition(slot); };
		if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SAVE_POS10, 0.5f)) { slot=9; res = curr_truck->savePosition(slot); };
#ifdef USE_MYGUI
		if (slot != -1 && !res)
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Position saved under slot ") + TOSTRING(slot+1), "infromation.png");
		else if (slot != -1 && res)
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Error while saving position saved under slot ") + TOSTRING(slot+1), "error.png");
#endif //USE_MYGUI

		if (res == -10)
		{
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS1, 0.5f)) { slot=0; res = curr_truck->loadPosition(slot); };
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS2, 0.5f)) { slot=1; res = curr_truck->loadPosition(slot); };
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS3, 0.5f)) { slot=2; res = curr_truck->loadPosition(slot); };
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS4, 0.5f)) { slot=3; res = curr_truck->loadPosition(slot); };
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS5, 0.5f)) { slot=4; res = curr_truck->loadPosition(slot); };
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS6, 0.5f)) { slot=5; res = curr_truck->loadPosition(slot); };
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS7, 0.5f)) { slot=6; res = curr_truck->loadPosition(slot); };
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS8, 0.5f)) { slot=7; res = curr_truck->loadPosition(slot); };
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS9, 0.5f)) { slot=8; res = curr_truck->loadPosition(slot); };
			if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LOAD_POS10, 0.5f)) { slot=9; res = curr_truck->loadPosition(slot); };
#ifdef USE_MYGUI
			if (slot != -1 && res==0)
				Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Loaded position from slot ") + TOSTRING(slot+1), "infromation.png");
			else if (slot != -1 && res!=0)
				Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Could not load position from slot ") + TOSTRING(slot+1), "error.png");
#endif // USE_MYGUI
		}
	}

	// camera FOV settings
	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_FOV_LESS, 0.1f) || INPUTENGINE.getEventBoolValueBounce(EV_COMMON_FOV_MORE, 0.1f))
	{
		int fov = mCamera->getFOVy().valueDegrees();

		if (INPUTENGINE.getEventBoolValue(EV_COMMON_FOV_LESS))
			fov--;
		else
			fov++;

		if (fov >= 10 && fov <= 160)
		{
			mCamera->setFOVy(Degree(fov));

	#ifdef USE_MYGUI
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: ") + TOSTRING(fov), "camera_edit.png", 2000);
	#endif // USE_MYGUI

			// save the settings
			if (CameraManager::singletonExists() &&
				CameraManager::getSingleton().hasActiveBehavior() &&
				CameraManager::getSingleton().getCameraBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
			{
				SETTINGS.setSetting("FOV Internal", TOSTRING(fov));
			} else
			{
				SETTINGS.setSetting("FOV External", TOSTRING(fov));
			}
		} else
		{
#ifdef USE_MYGUI
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: Limit reached"), "camera_edit.png", 2000);
#endif // USE_MYGUI
		}
	}

	// full screen/windowed screen switching
	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_FULLSCREEN_TOGGLE, 2.0f))
	{
		static int org_width = -1, org_height = -1;
		int width = mWindow->getWidth();
		int height = mWindow->getHeight();
		if (org_width == -1)
			org_width = width;
		if (org_height == -1)
			org_height = height;
		bool mode = mWindow->isFullScreen();
		if (!mode)
		{
			mWindow->setFullscreen(true, org_width, org_height);
			LOG(" ** switched to fullscreen: "+TOSTRING(width)+"x"+TOSTRING(height));
		} else
		{
			mWindow->setFullscreen(false, 640, 480);
			mWindow->setFullscreen(false, org_width, org_height);
			LOG(" ** switched to windowed mode: ");
		}
	}


	if (loading_state==ALL_LOADED)
	{
		if (CameraManager::singletonExists() &&
			(!CameraManager::getSingleton().hasActiveBehavior() ||
			CameraManager::getSingleton().getCameraBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE))
		{
			if (!curr_truck)
			{
				if (person)
				{
					person->setPhysicsEnabled(true);
				}
			} else // we are in a vehicle
			{
				// get commands
				// -- here we should define a maximum numbers per trucks. Some trucks does not have that much commands
				// -- available, so why should we iterate till MAX_COMMANDS?
				for (int i=1; i<=MAX_COMMANDS; i++)
				{
					float oldVal = curr_truck->commandkey[i].commandValue;
					
					curr_truck->commandkey[i].commandValue = 0;
					int eventID = EV_COMMANDS_01 + (i - 1);
					float tmp = INPUTENGINE.getEventValue(eventID);
					if (tmp > 0.0)
					{
						curr_truck->commandkey[i].commandValue = tmp;
					}

					if (tmp > 0.01f && oldVal < 0.01f)
					{
						// just started
						curr_truck->commandkey[i].commandValueState = 1;
					} else if (tmp < 0.01f && oldVal > 0.01f)
					{
						// just stopped
						curr_truck->commandkey[i].commandValueState = -1;
					}

				}

				// replay mode
				if (curr_truck->replaymode)
				{
					if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && curr_truck->replaypos <= 0)
					{
						curr_truck->replaypos++;
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && curr_truck->replaypos > -curr_truck->replaylen)
					{
						curr_truck->replaypos--;
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && curr_truck->replaypos+10 <= 0)
					{
						curr_truck->replaypos+=10;
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && curr_truck->replaypos-10 > -curr_truck->replaylen)
					{
						curr_truck->replaypos-=10;
					}

					if (INPUTENGINE.isKeyDown(OIS::KC_LMENU))
					{
						if (curr_truck->replaypos <= 0 && curr_truck->replaypos >= -curr_truck->replaylen)
						{
							if (INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) || INPUTENGINE.isKeyDown(OIS::KC_RSHIFT))
							{
								curr_truck->replaypos += INPUTENGINE.getMouseState().X.rel * 1.5f;
							} else
							{
								curr_truck->replaypos += INPUTENGINE.getMouseState().X.rel * 0.05f;
							}
							if (curr_truck->replaypos > 0)
							{
								curr_truck->replaypos = 0;
							}
							if (curr_truck->replaypos < -curr_truck->replaylen)
							{
								curr_truck->replaypos = -curr_truck->replaylen;
							}
						}
					}
				}

				if (curr_truck->driveable==TRUCK)
				{
					//road construction stuff
					if (INPUTENGINE.getEventBoolValueBounce(EV_TERRAINEDITOR_SELECTROAD, 0.5f) && curr_truck->editorId>=0 && !curr_truck->replaymode)
					{
						if (road)
							road->reset(curr_truck->nodes[curr_truck->editorId].AbsPosition);
						else
							road=new Road(mSceneMgr, curr_truck->nodes[curr_truck->editorId].AbsPosition);
					}

					//editor stuff
					if (INPUTENGINE.getEventBoolValueBounce(EV_TERRAINEDITOR_TOGGLEOBJECT) && curr_truck->editorId>=0 && !curr_truck->replaymode)
					{
						if (editor)
							editor->toggleType();
						else
							editor=new Editor(mSceneMgr, this);
					}

					//this should not be there
					if (editor && curr_truck->editorId>=0)
					{
						editor->setPos(curr_truck->nodes[curr_truck->editorId].AbsPosition);
					}

					if (!curr_truck->replaymode)
					{
						if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_LEFT))
							curr_truck->leftMirrorAngle-=0.001;

						if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_RIGHT))
							curr_truck->leftMirrorAngle+=0.001;

						if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_LEFT))
							curr_truck->rightMirrorAngle-=0.001;

						if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_RIGHT))
							curr_truck->rightMirrorAngle+=0.001;

						if (INPUTENGINE.getEventBoolValueBounce(EV_TERRAINEDITOR_ROTATELEFT, 0.1f))
						{
							float value = 0.5;
							if (INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) || INPUTENGINE.isKeyDown(OIS::KC_RSHIFT)) value = 4;
							if (road) {road->dturn(+value);}
							else if (editor) {editor->dturn(+1);}
						}

						if (INPUTENGINE.getEventBoolValueBounce(EV_TERRAINEDITOR_ROTATERIGHT, 0.1f))
						{
							float value = 0.5;
							if (INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) || INPUTENGINE.isKeyDown(OIS::KC_RSHIFT)) value = 4;
							if (road) {road->dturn(-value);}
							else if (editor) {editor->dturn(-1);}
						}

						if (INPUTENGINE.getEventBoolValueBounce(EV_TERRAINEDITOR_PITCHBACKWARD, 0.1f))
						{
							float value = 0.5;
							if (INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) || INPUTENGINE.isKeyDown(OIS::KC_RSHIFT))
								value = 4;
							if (road) {road->dpitch(-value);}
							else if (editor) {editor->dpitch(-1);}
						}

						if (INPUTENGINE.getEventBoolValueBounce(EV_TERRAINEDITOR_PITCHFOREWARD, 0.1f))
						{
							float value = 0.5;
							if (INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) || INPUTENGINE.isKeyDown(OIS::KC_RSHIFT))
							value = 4;
							if (road) {road-> dpitch(value);}
							else if (editor) {editor->dpitch(1);}
						}

						if (INPUTENGINE.getEventBoolValueBounce(EV_TERRAINEDITOR_TOGGLEROADTYPE, 0.5f))
						{
							if (road)
								road->toggleType();
						}

						if (INPUTENGINE.getEventBoolValueBounce(EV_TERRAINEDITOR_BUILT, 0.5f))
						{
							if (road)
							{
								if (!editorfd)
								{
									String editorfn = SSETTING("Log Path", "") + "editor_out.txt";
									editorfd = fopen(editorfn.c_str(), "a");
									fprintf(editorfd, " ==== new session\n");
								}
								road->append();
								fprintf(editorfd, "%f, %f, %f, %f, %f, %f, %s\n", road->rpos.x, road->rpos.y, road->rpos.z, road->rrot.x, road->rrot.y, road->rrot.z, road->curtype);
								LOG(TOSTRING(road->rpos.x)+", "+
									TOSTRING(road->rpos.y)+", "+
									TOSTRING(road->rpos.z)+", "+
									TOSTRING(road->rrot.x)+", "+
									TOSTRING(road->rrot.y)+", "+
									TOSTRING(road->rrot.z)+", "+road->curtype);

								loadObject(road->curtype, road->rpos.x, road->rpos.y, road->rpos.z, road->rrot.x, road->rrot.y, road->rrot.z, 0, "generic");
							}

							if (editor)
							{
								if (!editorfd)
								{
									String editorfn = SSETTING("Log Path", "") + "editor_out.txt";
									editorfd = fopen(editorfn.c_str(), "a");
									fprintf(editorfd, " ==== new session\n");
								}
								fprintf(editorfd, "%f, %f, %f, %f, %f, %f, %s\n", editor->ppos.x, editor->ppos.y, editor->ppos.z, 0.0, editor->pturn, editor->ppitch, editor->curtype);
								LOG(TOSTRING(editor->ppos.x)+", "+
									TOSTRING(editor->ppos.y)+", "+
									TOSTRING(editor->ppos.z)+", "+
									TOSTRING(0)+", "+
									TOSTRING(editor->pturn)+", "+
									TOSTRING(editor->ppitch)+", "+editor->curtype);
								loadObject(editor->curtype, editor->ppos.x, editor->ppos.y, editor->ppos.z, 0, editor->pturn, editor->ppitch, 0, "generic", false);
							}
						}
					} // end of (!curr_truck->replaymode) block

					if (!curr_truck->replaymode)
					{
						// steering
						float tmp_left_digital  = INPUTENGINE.getEventValue(EV_TRUCK_STEER_LEFT,  false, InputEngine::ET_DIGITAL);
						float tmp_right_digital = INPUTENGINE.getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_DIGITAL);
						float tmp_left_analog   = INPUTENGINE.getEventValue(EV_TRUCK_STEER_LEFT,  false, InputEngine::ET_ANALOG);
						float tmp_right_analog  = INPUTENGINE.getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_ANALOG);

						float sum = -std::max(tmp_left_digital,tmp_left_analog)+ std::max(tmp_right_digital,tmp_right_analog);

						if (sum < -1) sum = -1;
						if (sum > 1) sum = 1;

						curr_truck->hydrodircommand = sum;

						if ((tmp_left_digital < tmp_left_analog) || (tmp_right_digital < tmp_right_analog))
						{
							curr_truck->hydroSpeedCoupling = false;
						} else
						{
							curr_truck->hydroSpeedCoupling = true;
						}

						if (curr_truck->engine)
						{
							bool arcadeControls = BSETTING("ArcadeControls", false);

							float accval = INPUTENGINE.getEventValue(EV_TRUCK_ACCELERATE);
							float brake  = INPUTENGINE.getEventValue(EV_TRUCK_BRAKE);

							// arcade controls are only working with auto-clutch!
							if (!arcadeControls || curr_truck->engine->getAutoMode() > BeamEngine::SEMIAUTO)
							{
								// classic mode, realistic
								if (curr_truck->engine)
								{
									curr_truck->engine->autoSetAcc(accval);
								}
								curr_truck->brake = brake * curr_truck->brakeforce;
							} else
							{
								// start engine
								if (accval > 0 && curr_truck->engine && curr_truck->engine->hasContact() && !curr_truck->engine->isRunning())
								{
									curr_truck->engine->start();
								}

								// arcade controls: hey - people wanted it x| ... <- and it's convenient
								if (curr_truck->engine->getGear() >= 0)
								{
									// neutral or drive forward, everything is as its used to be: brake is brake and accel. is accel.
									if (curr_truck->engine)
									{
										curr_truck->engine->autoSetAcc(accval);
									}
									curr_truck->brake = brake * curr_truck->brakeforce;
								} else
								{
									// reverse gear, reverse controls: brake is accel. and accel. is brake.
									if (curr_truck->engine)
									{
										curr_truck->engine->autoSetAcc(brake);
									}
									curr_truck->brake = accval * curr_truck->brakeforce;
								}

								// only when the truck really is not moving anymore
								if (fabs(curr_truck->WheelSpeed) <= 0.1f)
								{
									// switching point, does the user want to drive forward from backward or the other way round? change gears?
									if (brake > 0.5f && accval < 0.5f && curr_truck->engine->getGear() >= 0)
									{
										// we are on the brake, jump to reverse gear
										if (curr_truck->engine->getAutoMode() == BeamEngine::AUTOMATIC)
										{
											curr_truck->engine->autoShiftSet(BeamEngine::REAR);
										} else
										{
											curr_truck->engine->setGear(-1);
										}
									} else if (brake < 0.5f && accval > 0.5f && curr_truck->engine->getGear() < 0)
									{
										// we are on the gas pedal, jump to first gear when we were in rear gear
										if (curr_truck->engine->getAutoMode() == BeamEngine::AUTOMATIC)
										{									
											curr_truck->engine->autoShiftSet(BeamEngine::DRIVE);
										} else
										{
											curr_truck->engine->setGear(1);
										}
									}
								}
							}

							// IMI
							// gear management -- it might, should be transferred to a standalone function of Beam or RoRFrameListener
							if (curr_truck->engine->getAutoMode() == BeamEngine::AUTOMATIC)
							{
								if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_UP))
								{
									curr_truck->engine->autoShiftUp();
								}
								if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_DOWN))
								{
									curr_truck->engine->autoShiftDown();
								}
							}

							if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_TOGGLE_CONTACT))
							{
								curr_truck->engine->toggleContact();
							}

							if (INPUTENGINE.getEventBoolValue(EV_TRUCK_STARTER) && curr_truck->engine->hasContact())
							{
								// starter
								curr_truck->engine->setstarter(1);
#ifdef USE_OPENAL
								SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_STARTER);
#endif // OPENAL
							} else
							{
								curr_truck->engine->setstarter(0);
#ifdef USE_OPENAL
								SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_STARTER);
#endif // OPENAL
							}

							if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SWITCH_SHIFT_MODES))
							{
								// toggle Auto shift
								curr_truck->engine->toggleAutoMode();

								// force gui update
								curr_truck->triggerGUIFeaturesChanged();
#ifdef USE_MYGUI
								switch(curr_truck->engine->getAutoMode())
								{
									case BeamEngine::AUTOMATIC:
										Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Automatic shift"), "cog.png", 3000);
										break;
									case BeamEngine::SEMIAUTO:
										Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Manual shift - Auto clutch"), "cog.png", 3000);
										break;
									case BeamEngine::MANUAL:
										Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Fully Manual: sequential shift"), "cog.png", 3000);
										break;
									case BeamEngine::MANUAL_STICK:
										Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Fully manual: stick shift"), "cog.png", 3000);
										break;
									case BeamEngine::MANUAL_RANGES:
										Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Fully Manual: stick shift with ranges"), "cog.png", 3000);
										break;
								}
#endif //USE_MYGUI
							}

							// joy clutch
							float cval = INPUTENGINE.getEventValue(EV_TRUCK_MANUAL_CLUTCH);
							curr_truck->engine->setManualClutch(cval);

							bool gear_changed_rel = false;
							int shiftmode = curr_truck->engine->getAutoMode();

							if (shiftmode <= BeamEngine::MANUAL) // auto, semi auto and sequential shifting
							{
								if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SHIFT_UP))
								{
									if (shiftmode != BeamEngine::AUTOMATIC || curr_truck->engine->getAutoShift() == BeamEngine::DRIVE)
									{
										curr_truck->engine->shift(1);
										gear_changed_rel = true;
									}
								} else if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SHIFT_DOWN))
								{
									if (shiftmode  > BeamEngine::SEMIAUTO ||
										shiftmode == BeamEngine::SEMIAUTO  && curr_truck->engine->getGear() > 0 ||
										shiftmode == BeamEngine::AUTOMATIC && curr_truck->engine->getGear() > 1)
									{
										curr_truck->engine->shift(-1);
										gear_changed_rel = true;
									}
								} else if (shiftmode != BeamEngine::AUTOMATIC && INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SHIFT_NEUTRAL))
								{
									curr_truck->engine->shiftTo(0);
								}
							} else //if (shiftmode > BeamEngine::MANUAL) // h-shift or h-shift with ranges shifting
							{
								bool gear_changed	= false;
								bool found			= false;
								int curgear		    = curr_truck->engine->getGear();
								int curgearrange    = curr_truck->engine->getGearRange();
								int gearoffset      = std::max(0, curgear - curgearrange * 6);

								// one can select range only if in natural
								if (shiftmode==BeamEngine::MANUAL_RANGES && curgear == 0)
								{
									//  maybe this should not be here, but should experiment
									if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SHIFT_LOWRANGE) && curgearrange != 0)
									{
										curr_truck->engine->setGearRange(0);
										gear_changed = true;
#ifdef USE_MYGUI
										Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Low range selected"), "cog.png", 3000);
#endif //USE_MYGUI
									} else if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SHIFT_MIDRANGE)  && curgearrange != 1 && curr_truck->engine->getNumGearsRanges()>1)
									{
										curr_truck->engine->setGearRange(1);
										gear_changed = true;
#ifdef USE_MYGUI
										Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Mid range selected"), "cog.png", 3000);
#endif //USE_MYGUI
									} else if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_SHIFT_HIGHRANGE) && curgearrange != 2 && curr_truck->engine->getNumGearsRanges()>2)
									{
										curr_truck->engine->setGearRange(2);
										gear_changed = true;
#ifdef USE_MYGUI
										Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("High range selected"), "cog.png", 3000);
#endif // USE_MYGUI
									}
								}
//zaxxon
								if (curgear == -1)
								{
									gear_changed = !INPUTENGINE.getEventBoolValue(EV_TRUCK_SHIFT_GEAR_REVERSE);
								} else if (curgear > 0 && curgear < 19)
								{
									if (shiftmode==BeamEngine::MANUAL)
									{
										gear_changed = !INPUTENGINE.getEventBoolValue(EV_TRUCK_SHIFT_GEAR1 + curgear -1);
									} else
									{
										gear_changed = !INPUTENGINE.getEventBoolValue(EV_TRUCK_SHIFT_GEAR1 + gearoffset-1); // range mode
									}
								}

								if (gear_changed || curgear == 0)
								{
									if (INPUTENGINE.getEventBoolValue(EV_TRUCK_SHIFT_GEAR_REVERSE))
									{
										curr_truck->engine->shiftTo(-1);
										found = true;
									} else if (INPUTENGINE.getEventBoolValue(EV_TRUCK_SHIFT_NEUTRAL))
									{
										curr_truck->engine->shiftTo(0);
										found = true;
									} else
									{
										if (shiftmode == BeamEngine::MANUAL_STICK)
										{
											for (int i=1;i<19 && !found;i++)
											{
												if (INPUTENGINE.getEventBoolValue(EV_TRUCK_SHIFT_GEAR1 +i - 1))
												{
													curr_truck->engine->shiftTo(i);
													found = true;
												}
											}
										} else // BeamEngine::MANUALMANUAL_RANGES
										{
											for (int i=1; i < 7 && !found; i++)
											{
												if (INPUTENGINE.getEventBoolValue(EV_TRUCK_SHIFT_GEAR1 +i - 1))
												{
													curr_truck->engine->shiftTo(i + curgearrange * 6);
													found = true;
												}
											}
										}
									}
									if (!found)
									{
										curr_truck->engine->shiftTo(0);
									}
								} // end of if (gear_changed)
							} // end of shitmode > BeamEngine::MANUAL

							// anti roll back in BeamEngine::AUTOMATIC (DRIVE, TWO, ONE) mode
							if (curr_truck->engine->getAutoMode()  == BeamEngine::AUTOMATIC &&
							   (curr_truck->engine->getAutoShift() == BeamEngine::DRIVE ||
							    curr_truck->engine->getAutoShift() == BeamEngine::TWO ||
							    curr_truck->engine->getAutoShift() == BeamEngine::ONE) &&
								curr_truck->WheelSpeed < 0.1f)
							{
								Vector3 dirDiff = (curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition).normalisedCopy();
								Degree pitchAngle = Radian(asin(dirDiff.dotProduct(Vector3::UNIT_Y)));
								float accl = INPUTENGINE.getEventValue(EV_TRUCK_ACCELERATE);

								if (pitchAngle.valueDegrees() > 1.0f)
								{
									curr_truck->brake = curr_truck->brakeforce * (1.0f - accl);
								}
							}

							// anti roll forth in BeamEngine::AUTOMATIC (REAR) mode
							if (curr_truck->engine->getAutoMode()  == BeamEngine::AUTOMATIC &&
							    curr_truck->engine->getAutoShift() == BeamEngine::REAR &&
								curr_truck->WheelSpeed > 0.1f)
							{
								Vector3 dirDiff = (curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition).normalisedCopy();
								Degree pitchAngle = Radian(asin(dirDiff.dotProduct(Vector3::UNIT_Y)));
								float accl = INPUTENGINE.getEventValue(EV_TRUCK_ACCELERATE);

								if (pitchAngle.valueDegrees() < 1.0f)
								{
									curr_truck->brake = curr_truck->brakeforce * (1.0f - accl);
								}
							}
						} // end of ->engine
#ifdef USE_OPENAL
						if (curr_truck->brake > curr_truck->brakeforce / 6.0f)
						{
							SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_BRAKE);
						} else
						{
							SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_BRAKE);
						}
#endif // USE_OPENAL
					} // end of ->replaymode

					if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_TOGGLE_AXLE_LOCK))
					{
						// toggle auto shift
						if (!curr_truck->getAxleLockCount())
						{
#ifdef USE_MYGUI
							Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No differential installed on current vehicle!"), "warning.png", 3000);
#endif // USE_MYGUI
						} else
						{
							curr_truck->toggleAxleLock();
#ifdef USE_MYGUI
							Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Differentials switched to: ") + curr_truck->getAxleLockName(), "cog.png", 3000);
#endif // USE_MYGUI
						}
					}

#ifdef USE_OPENAL
					if (curr_truck->ispolice)
					{
						if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_HORN))
						{
							SoundScriptManager::getSingleton().trigToggle(curr_truck, SS_TRIG_HORN);
						}
					}
					else
					{
						if (INPUTENGINE.getEventBoolValue(EV_TRUCK_HORN) && !curr_truck->replaymode)
						{
							SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_HORN);
						} else
						{
							SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_HORN);
						};
					}
#endif // OPENAL

					if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_PARKING_BRAKE))
					{
						curr_truck->parkingbrakeToggle();
					}

					if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_ANTILOCK_BRAKE))
					{
						if (curr_truck->alb_present && !curr_truck->alb_notoggle)
						{
							curr_truck->antilockbrakeToggle();
						}
					}

					if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_TRACTION_CONTROL))
					{
						if (curr_truck->tc_present && !curr_truck->tc_notoggle) curr_truck->tractioncontrolToggle();
					}

					if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_CRUISE_CONTROL))
					{
						curr_truck->cruisecontrolToggle();
					}
					if (curr_truck->cc_mode)
					{

						updateCruiseControl(curr_truck, dt);
					}
					checkSpeedlimit(curr_truck, dt);
				}
				if (curr_truck->driveable==AIRPLANE)
				{
					//autopilot
					if (curr_truck->autopilot && curr_truck->autopilot->wantsdisconnect)
					{
						curr_truck->disconnectAutopilot();
					}
					//AIRPLANE KEYS
					float commandrate=4.0;
					//float dt=evt.timeSinceLastFrame;
					//turning
					if (curr_truck->replaymode)
					{
						if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && curr_truck->replaypos<=0)
						{
							curr_truck->replaypos++;
						}
						if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && curr_truck->replaypos > -curr_truck->replaylen)
						{
							curr_truck->replaypos--;
						}
						if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && curr_truck->replaypos+10<=0)
						{
							curr_truck->replaypos+=10;
						}
						if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && curr_truck->replaypos-10 > -curr_truck->replaylen)
						{
							curr_truck->replaypos-=10;
						}
					} else
					{
						float tmp_left = INPUTENGINE.getEventValue(EV_AIRPLANE_STEER_LEFT);
						float tmp_right = INPUTENGINE.getEventValue(EV_AIRPLANE_STEER_RIGHT);
						float sum_steer = -tmp_left + tmp_right;
						INPUTENGINE.smoothValue(curr_truck->aileron, sum_steer, dt*commandrate);
						curr_truck->hydrodircommand = curr_truck->aileron;
						curr_truck->hydroSpeedCoupling = !(INPUTENGINE.isEventAnalog(EV_AIRPLANE_STEER_LEFT) && INPUTENGINE.isEventAnalog(EV_AIRPLANE_STEER_RIGHT));
					}

					//pitch
					float tmp_pitch_up = INPUTENGINE.getEventValue(EV_AIRPLANE_ELEVATOR_UP);
					float tmp_pitch_down = INPUTENGINE.getEventValue(EV_AIRPLANE_ELEVATOR_DOWN);
					float sum_pitch = tmp_pitch_down - tmp_pitch_up;
					INPUTENGINE.smoothValue(curr_truck->elevator, sum_pitch, dt*commandrate);

					//rudder
					float tmp_rudder_left = INPUTENGINE.getEventValue(EV_AIRPLANE_RUDDER_LEFT);
					float tmp_rudder_right = INPUTENGINE.getEventValue(EV_AIRPLANE_RUDDER_RIGHT);
					float sum_rudder = tmp_rudder_left - tmp_rudder_right;
					INPUTENGINE.smoothValue(curr_truck->rudder, sum_rudder, dt*commandrate);

					//brake
					if (!curr_truck->replaymode && !curr_truck->parkingbrake)
					{
						curr_truck->brake=0.0;
						float brakevalue = INPUTENGINE.getEventValue(EV_AIRPLANE_BRAKE);
						curr_truck->brake=curr_truck->brakeforce*0.66*brakevalue;
					};
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_PARKING_BRAKE))
					{
						curr_truck->parkingbrakeToggle();
						if (ow)
						{
							if (curr_truck->parkingbrake)
								OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-on");
							else
								OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-off");
						}
					}
					//reverse
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_REVERSE))
					{
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->toggleReverse();
					}

					// toggle engines
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_TOGGLE_ENGINES))
					{
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->flipStart();
					}

					//flaps
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_FLAPS_NONE))
					{
						if (curr_truck->flap>0)
							curr_truck->flap=0;
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_FLAPS_FULL))
					{
						if (curr_truck->flap<5)
							curr_truck->flap=5;
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_FLAPS_LESS))
					{
						if (curr_truck->flap>0)
							curr_truck->flap=(curr_truck->flap)-1;
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_FLAPS_MORE))
					{
						if (curr_truck->flap<5)
							curr_truck->flap=(curr_truck->flap)+1;
					}

					//airbrakes
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_NONE))
					{
						if (curr_truck->airbrakeval>0)
							curr_truck->airbrakeval=0;
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_FULL))
					{
						if (curr_truck->airbrakeval<5)
							curr_truck->airbrakeval=5;
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_LESS))
					{
						if (curr_truck->airbrakeval>0)
							curr_truck->airbrakeval=(curr_truck->airbrakeval)-1;
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_MORE))
					{
						if (curr_truck->airbrakeval<5)
							curr_truck->airbrakeval=(curr_truck->airbrakeval)+1;
					}

					//throttle
					float tmp_throttle = INPUTENGINE.getEventBoolValue(EV_AIRPLANE_THROTTLE);
					if (tmp_throttle > 0)
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(tmp_throttle);

					if (INPUTENGINE.isEventDefined(EV_AIRPLANE_THROTTLE_AXIS))
					{
						float f = INPUTENGINE.getEventValue(EV_AIRPLANE_THROTTLE_AXIS);
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(f);
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_DOWN, 0.1f))
					{
						//throttle down
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(curr_truck->aeroengines[i]->getThrottle()-0.05);
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_UP, 0.1f))
					{
						//throttle up
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(curr_truck->aeroengines[i]->getThrottle()+0.05);
					}

					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_NO, 0.1f))
					{
						// no throttle
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(0);
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_FULL, 0.1f))
					{
						// full throttle
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(1);
					}
					if (curr_truck->autopilot)
					{
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(curr_truck->autopilot->getThrottle(curr_truck->aeroengines[i]->getThrottle(), dt));
					}


				}
				if (curr_truck->driveable==BOAT)
				{
					//BOAT SPECIFICS

					//throttle

					if (INPUTENGINE.isEventDefined(EV_BOAT_THROTTLE_AXIS))
					{
						float f = INPUTENGINE.getEventValue(EV_BOAT_THROTTLE_AXIS);
						// use negative values also!
						f = f * 2 - 1;
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setThrottle(-f);
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_BOAT_THROTTLE_DOWN, 0.1f))
					{
						//throttle down
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setThrottle(curr_truck->screwprops[i]->getThrottle()-0.05);
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_BOAT_THROTTLE_UP, 0.1f))
					{
						//throttle up
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setThrottle(curr_truck->screwprops[i]->getThrottle()+0.05);
					}


					// steer
					float tmp_steer_left = INPUTENGINE.getEventValue(EV_BOAT_STEER_LEFT);
					float tmp_steer_right = INPUTENGINE.getEventValue(EV_BOAT_STEER_RIGHT);
					float stime = INPUTENGINE.getEventBounceTime(EV_BOAT_STEER_LEFT) + INPUTENGINE.getEventBounceTime(EV_BOAT_STEER_RIGHT);
					float sum_steer = (tmp_steer_left - tmp_steer_right) * 0.06;
					// do not center the rudder!
					if (fabs(sum_steer)>0 && stime <= 0)
					{
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setRudder(curr_truck->screwprops[i]->getRudder() + sum_steer);
					}
					if (INPUTENGINE.isEventDefined(EV_BOAT_STEER_LEFT_AXIS) && INPUTENGINE.isEventDefined(EV_BOAT_STEER_RIGHT_AXIS))
					{
						tmp_steer_left = INPUTENGINE.getEventValue(EV_BOAT_STEER_LEFT_AXIS);
						tmp_steer_right = INPUTENGINE.getEventValue(EV_BOAT_STEER_RIGHT_AXIS);
						sum_steer = (tmp_steer_left - tmp_steer_right);
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setRudder(sum_steer);
					}
					if (INPUTENGINE.getEventBoolValueBounce(EV_BOAT_CENTER_RUDDER, 0.1f))
					{
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setRudder(0);
					}

					if (INPUTENGINE.getEventBoolValueBounce(EV_BOAT_REVERSE))
					{
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->toggleReverse();
					}
				}
				//COMMON KEYS

				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TRUCK_REMOVE))
				{
					BeamFactory::getSingleton().removeCurrentTruck();
				}
				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_ROPELOCK))
				{
					curr_truck->ropeToggle(-1);
				}
				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_LOCK))
				{
					curr_truck->hookToggle(-1, HOOK_TOGGLE, -1);
					//SlideNodeLock
					curr_truck->toggleSlideNodeLock();
				}
				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_AUTOLOCK))
				{
					//unlock all autolocks
					curr_truck->hookToggle(-2, HOOK_UNLOCK, -1);
				}
				//strap
				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_SECURE_LOAD))
				{
					curr_truck->tieToggle(-1);
				}
				if (INPUTENGINE.getEventBoolValue(EV_COMMON_RESET_TRUCK) && !curr_truck->replaymode)
				{
					// stop any races
					stopTimer();
					// init
					curr_truck->reset();
				}
				if (INPUTENGINE.getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
				{
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().trigOnce(curr_truck, SS_TRIG_REPAIR);
#endif //OPENAL
					curr_truck->reset(true);
				}
				//replay mode
				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TOGGLE_REPLAY_MODE))
				{
					stopTimer();
					curr_truck->setReplayMode(!curr_truck->replaymode);
				}

				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TOGGLE_CUSTOM_PARTICLES))
				{
					curr_truck->toggleCustomParticles();
				}

				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_SHOW_SKELETON))
				{
					if (curr_truck->skeleton)
						curr_truck->hideSkeleton(true);
					else
						curr_truck->showSkeleton(true, true);

					curr_truck->updateVisual();
				}

				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_LIGHTS))
				{
					curr_truck->lightsToggle();
				}

				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_BEACONS))
				{
					curr_truck->beaconsToggle();
				}

				//camera mode
				if (INPUTENGINE.getEventBoolValue(EV_COMMON_PRESSURE_LESS) && curr_truck)
				{
					if (ow) ow->showPressureOverlay(true);
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_AIR);
#endif // OPENAL
					curr_truck->addPressure(-dt*10.0);
					pressure_pressed=true;
				}
				else if (INPUTENGINE.getEventBoolValue(EV_COMMON_PRESSURE_MORE))
				{
					if (ow) ow->showPressureOverlay(true);
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_AIR);
#endif // OPENAL
					curr_truck->addPressure(dt*10.0);
					pressure_pressed=true;
				} else if (pressure_pressed)
				{
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_AIR);
#endif // OPENAL
					pressure_pressed=false;
					if (ow) ow->showPressureOverlay(false);
				}

				if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK, 0.5f) && !netmode && curr_truck->driveable != AIRPLANE)
				{
					if (!BeamFactory::getSingleton().enterRescueTruck())
					{
#ifdef USE_MYGUI
						Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No rescue truck found!"), "warning.png");
#endif // USE_MYGUI
					}
				}

				if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_BLINK_LEFT))
				{
					if (curr_truck->getBlinkType() == BLINK_LEFT)
						curr_truck->setBlinkType(BLINK_NONE);
					else
						curr_truck->setBlinkType(BLINK_LEFT);
				}

				if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_BLINK_RIGHT))
				{
					if (curr_truck->getBlinkType() == BLINK_RIGHT)
						curr_truck->setBlinkType(BLINK_NONE);
					else
						curr_truck->setBlinkType(BLINK_RIGHT);
				}

				if (INPUTENGINE.getEventBoolValueBounce(EV_TRUCK_BLINK_WARN))
				{
					if (curr_truck->getBlinkType() == BLINK_WARN)
						curr_truck->setBlinkType(BLINK_NONE);
					else
						curr_truck->setBlinkType(BLINK_WARN);
				}

			} // end of truck!=-1
		}

#ifdef USE_CAELUM
		if (SSETTING("Sky effects", "Caelum (best looking, slower)") == "Caelum (best looking, slower)")
		{
			Ogre::Real time_factor = 1000.0f;
			Ogre::Real multiplier = 10;
			bool update_time = false;

			if (INPUTENGINE.getEventBoolValue(EV_CAELUM_INCREASE_TIME) && SkyManager::getSingletonPtr())
			{
				update_time = true;
			}
			else if (INPUTENGINE.getEventBoolValue(EV_CAELUM_INCREASE_TIME_FAST) && SkyManager::getSingletonPtr())
			{
				time_factor *= multiplier;
				update_time = true;
			}
			else if (INPUTENGINE.getEventBoolValue(EV_CAELUM_DECREASE_TIME) && SkyManager::getSingletonPtr())
			{
				time_factor = -time_factor;
				update_time = true;
			}
			else if (INPUTENGINE.getEventBoolValue(EV_CAELUM_DECREASE_TIME_FAST) && SkyManager::getSingletonPtr())
			{
				time_factor *= -multiplier;
				update_time = true;
			}
			else
			{
				time_factor = 1.0f;
				update_time = SkyManager::getSingleton().getTimeFactor() != 1.0f;
			}

			if ( update_time )
			{
				SkyManager::getSingleton().setTimeFactor(time_factor);
#ifdef USE_MYGUI
				Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Time set to ") + SkyManager::getSingleton().getPrettyTime(), "weather_sun.png", 1000);
#endif // USE_MYGUI
			}
		}
#endif // USE_CAELUM

		if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TOGGLE_RENDER_MODE, 0.5f))
		{
			static int mSceneDetailIndex;
			mSceneDetailIndex = (mSceneDetailIndex + 1) % 3;
			switch (mSceneDetailIndex)
			{
			case 0:
				mCamera->setPolygonMode(Ogre::PM_SOLID);
				break;
			case 1:
				mCamera->setPolygonMode(Ogre::PM_WIREFRAME);
				break;
			case 2:
				mCamera->setPolygonMode(Ogre::PM_POINTS);
				break;
			}
		}

#ifdef USE_MYGUI
		if (surveyMap)
		{
			static float alphaValue = 1.0f;
			float velocity = 0.0f;
			if (curr_truck)
			{
				velocity = curr_truck->nodes[0].Velocity.length();
			}
			if (INPUTENGINE.getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE_VIEW))
			{
				surveyMapMode = (surveyMapMode + 1) % SURVEY_MAP_END;

				if (surveyMapMode == SURVEY_MAP_BIG && (velocity > 5.0f ||
					(CameraManager::singletonExists() &&
					CameraManager::getSingleton().hasActiveBehavior() &&
					CameraManager::getSingleton().getCameraBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)))
				{
					surveyMapMode = (surveyMapMode + 1) % SURVEY_MAP_END;
				}

				if (surveyMapMode == SURVEY_MAP_NONE)
				{
					surveyMap->setVisibility(false);
				} else
				{
					if (surveyMapMode == SURVEY_MAP_SMALL)
					{
						surveyMap->setPosition(-1, 1, 0.3f, mWindow);
					} else if (surveyMapMode == SURVEY_MAP_BIG)
					{
						surveyMap->setPosition(0, 0, 0.98f, mWindow);
					}
					surveyMap->setAlpha(alphaValue);
					surveyMap->setVisibility(true);
				}
			}
			if (INPUTENGINE.getEventBoolValueBounce(EV_SURVEY_MAP_ALPHA))
			{
				if (surveyMap->getAlpha() > 0.51f)
				{
					surveyMap->setAlpha(0.5f);
				}
				else if (surveyMap->getAlpha() >= 0.21f && surveyMap->getAlpha() <= 0.51f)
				{
					surveyMap->setAlpha(0.2f);
				}
				else if (surveyMap->getAlpha() < 0.21f)
				{
					surveyMap->setAlpha(1.0f);
				}
				alphaValue = surveyMap->getAlpha();
			}
			if (curr_truck &&
				surveyMapMode == SURVEY_MAP_BIG &&
				CameraManager::singletonExists() &&
				CameraManager::getSingleton().hasActiveBehavior() &&
				CameraManager::getSingleton().getCameraBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE)
			{
				if (velocity > 7.5f || CameraManager::getSingleton().getCameraBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
				{
					surveyMap->setPosition(-1, 1, 0.3f, mWindow);
					surveyMap->setAlpha(alphaValue);
					surveyMapMode = SURVEY_MAP_SMALL;
				} else
				{
					surveyMap->setAlpha(1.0f / sqrt(std::max(1.0f, velocity - 1.0f)));
				}
			}
		}
#endif //USE_MYGUI
		
		if (INPUTENGINE.getEventBoolValue(EV_COMMON_ENTER_OR_EXIT_TRUCK) && mTimeUntilNextToggle <= 0)
		{
			mTimeUntilNextToggle = 0.5; //Some delay before trying to re-enter(exit) truck
			//perso in/out
			int current_truck = BeamFactory::getSingleton().getCurrentTruckNumber();
			int free_truck    = BeamFactory::getSingleton().getTruckCount();
			Beam **trucks     = BeamFactory::getSingleton().getTrucks();
			if (current_truck == -1)
			{
				//find the nearest truck
				float mindist   =  1000.0;
				int   minindex  = -1;
				for (int i=0; i<free_truck; i++)
				{
					if (!trucks[i]) continue;
					if (!trucks[i]->driveable) continue;
					if (trucks[i]->cinecameranodepos[0]==-1)
					{
						LOG("cinecam missing, cannot enter truck!");
						continue;
					}
					float len = (trucks[i]->nodes[trucks[i]->cinecameranodepos[0]].AbsPosition-(person->getPosition()+Vector3(0.0, 2.0, 0.0))).length();
					if (len < mindist)
					{
						mindist=len;
						minindex=i;
					}
				}
				if (mindist < 20.0)
				{
					BeamFactory::getSingleton().setCurrentTruck(minindex);
				}
			}
			else if (curr_truck->nodes[curr_truck->cinecameranodepos[0]].Velocity.length()<1)
			{
				BeamFactory::getSingleton().setCurrentTruck(-1);
			}
			else
			{
				curr_truck->brake    = curr_truck->brakeforce * 0.66;
				mTimeUntilNextToggle = 0.0; //No delay in this case: the truck must brake like braking normally
			}
		}
	} else
	{
		//no terrain or truck loaded

		// in embedded mode we wont show that loading stuff
		/*
		if (isEmbedded)
		{
			loading_state=ALL_LOADED;
#ifdef USE_MYGUI
			LoadingWindow::getSingleton().hide();
#endif //USE_MYGUI
		}
		*/
		//uiloader->updateEvents(dt);


#ifdef USE_MYGUI
		if (SelectorWindow::getSingleton().isFinishedSelecting())
		{
			if (loading_state==NONE_LOADED)
			{
				Cache_Entry *sel = SelectorWindow::getSingleton().getSelection();
				Skin *skin = SelectorWindow::getSingleton().getSelectedSkin();
				if (sel)
				{
					terrainUID = sel->uniqueid;
					loadTerrain(sel->fname);

					// no trucks loaded?
					if (truck_preload_num == 0)
					{
						// show truck selector
						if (w)
						{
							hideMap();
							SelectorWindow::getSingleton().show(SelectorWindow::LT_NetworkWithBoat);
						}
						else
						{
							hideMap();
							SelectorWindow::getSingleton().show(SelectorWindow::LT_Network);
						}
					} else
					{
						// init no trucks
						initTrucks(false, sel->fname, "", 0, false, skin);
					}
				}
			} else if (loading_state==TERRAIN_LOADED)
			{
				Cache_Entry *selection = SelectorWindow::getSingleton().getSelection();
				Skin *skin = SelectorWindow::getSingleton().getSelectedSkin();
				std::vector<Ogre::String> config = SelectorWindow::getSingleton().getTruckConfig();
				std::vector<Ogre::String> *configptr = &config;
				if (config.size() == 0) configptr = 0;
				if (selection)
					initTrucks(true, selection->fname, selection->fext, configptr, false, skin);
				else
					initTrucks(false, "");

			} else if (loading_state==RELOADING)
			{
				Cache_Entry *selection = SelectorWindow::getSingleton().getSelection();
				Skin *skin = SelectorWindow::getSingleton().getSelectedSkin();
				Beam *localTruck = 0;
				if (selection)
				{
					//we load an extra truck
					String selected = selection->fname;
					std::vector<Ogre::String> config = SelectorWindow::getSingleton().getTruckConfig();
					std::vector<Ogre::String> *configptr = &config;
					if (config.size() == 0) configptr = 0;

					localTruck = BeamFactory::getSingleton().createLocal(reload_pos, reload_dir, selected, reload_box, false, flaresMode, configptr, skin, freeTruckPosition);
					freeTruckPosition=false; // reset this, only to be used once
				}

				if (surveyMap && localTruck)
				{
					MapEntity *e = surveyMap->createNamedMapEntity("Truck"+TOSTRING(localTruck->trucknum), MapControl::getTypeByDriveable(localTruck->driveable));
					if (e)
					{
						e->setState(DESACTIVATED);
						e->setVisibility(true);
						e->setPosition(reload_pos);
						e->setRotation(-Radian(localTruck->getHeadingDirectionAngle()));
						// create a map icon
						//createNamedMapEntity();
					}
				}

				SelectorWindow::getSingleton().hide();
				loading_state=ALL_LOADED;

				GUIManager::getSingleton().unfocus();

				if (localTruck && localTruck->driveable)
				{
					//we are supposed to be in this truck, if it is a truck
					if (localTruck->engine)
						localTruck->engine->start();
					BeamFactory::getSingleton().setCurrentTruck(localTruck->trucknum);
				} else
				{
					// if it is a load or trailer, than stay in person mode
					// but relocate to the new position, so we dont spawn the dialog again
					//personode->setPosition(reload_pos);
					person->move(Vector3(3.0, 0.2, 0.0)); //bad, but better
					//BeamFactory::getSingleton().setCurrentTruck(-1);
				}
			}

		}
#endif //MYGUI
	}



	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TRUCK_INFO) && curr_truck)
	{
		mTruckInfoOn = ! mTruckInfoOn;
		dirty=true;
		if (ow) ow->truckhud->show(mTruckInfoOn);
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_HIDE_GUI))
	{
		hidegui = !hidegui;
		hideGUI(hidegui);
		dirty=true;
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TOGGLE_STATS) && loading_state == ALL_LOADED)
	{
		dirty=true;
		if (mStatsOn==0)
			mStatsOn=1;
		else if (mStatsOn==1)
			mStatsOn=0;
		else if (mStatsOn==2)
			mStatsOn=0;

		if (ow) ow->showDebugOverlay(mStatsOn);
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_TOGGLE_MAT_DEBUG))
	{
		if (mStatsOn==0)
			mStatsOn=2;
		else if (mStatsOn==1)
			mStatsOn=2;
		else if (mStatsOn==2)
			mStatsOn=0;
		dirty=true;
		if (ow) ow->showDebugOverlay(mStatsOn);
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_COMMON_OUTPUT_POSITION) && loading_state == ALL_LOADED)
	{
		Vector3 position(Vector3::ZERO);
		Radian rotation(0);
		if (BeamFactory::getSingleton().getCurrentTruckNumber() == -1)
		{
			position = person->getPosition();
			rotation = person->getRotation() + Radian(Math::PI);
		} else
		{
			position = curr_truck->getPosition();
			Vector3 idir = curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
			rotation = atan2(idir.dotProduct(Vector3::UNIT_X), idir.dotProduct(-Vector3::UNIT_Z));
		}
		LOG("Position: " + TOSTRING(position.x) + ", "+ TOSTRING(position.y) + ", " + TOSTRING(position.z) + ", 0, " + TOSTRING(rotation.valueDegrees()) + ", 0");
	}

	//update window
	if (!mWindow->isAutoUpdated())
	{
#ifdef HAS_EDITOR
		if ((trucked && trucked->dirty)||dirty) {trucked->dirty=false;mWindow->update();}
#else
		if (dirty) {mWindow->update();}
#endif
		else
			sleepMilliSeconds(10);
	}
	// Return true to continue rendering
	return true;
}

void RoRFrameListener::shutdown_final()
{
	LOG(" ** Shutdown preparation");
	//GUIManager::getSingleton().shutdown();
#ifdef USE_SOCKETW
	if (net) net->disconnect();
#endif //SOCKETW
	loading_state=EXITING;
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().setEnabled(false);
#endif // OPENAL
#ifdef USE_OIS_G27
	//logitech G27 LEDs tachometer
	if (leds)
	{
		leds->play(0, 10, 20);//stop the LEDs
	}
#endif //OIS_G27

	LOG(" ** Shutdown final");
	if (editorfd) fclose(editorfd);
	if (w) w->prepareShutdown();
	if (dashboard) dashboard->prepareShutdown();
	if (envmap) envmap->prepareShutdown();
	if (heathaze) heathaze->prepareShutdown();

	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if (curr_truck) curr_truck->prepareShutdown();
	INPUTENGINE.prepareShutdown();

	RigsOfRods *ror = RigsOfRods::getSingletonPtr();
	if (ror) ror->tryShutdown();

	shutdownall=true;
	//terrainmaterial->getBestTechnique()->getPass(0)->getTextureUnitState(0)->setTextureName(terrainoriginalmaterial);
}

void RoRFrameListener::hideMap()
{
#ifdef USE_MYGUI
	if (surveyMap)
		surveyMap->setVisibility(false);
#endif //USE_MYGUI
}


void RoRFrameListener::initializeCompontents()
{
	// load map
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().setProgress(0, _L("Loading Terrain"));
	bool disableMap = (BSETTING("disableOverViewMap", false));

	// map must be loaded before the script engine
	// init the map
	if (!disableMap)
	{
		surveyMap = new MapControl(mapsizex, mapsizey, mapsizez);
	}
#endif //USE_MYGUI

	collisions=new Collisions(this, mSceneMgr, debugCollisions);

	// load AS
#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().setCollisions(collisions);
	if (!netmode)
	{
		if (ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(loadedTerrain+".as"))
		{
			ScriptEngine::getSingleton().loadScript(loadedTerrain+".as");
		} else
		{
			// load a default script that does the most basic things
			ScriptEngine::getSingleton().loadScript("default.as");
		}
	} else
	{
		// load the default stscriptuff so spawners will work in multiplayer
		ScriptEngine::getSingleton().loadScript("default.as");
	}

	// finally activate AS logging, so we dont spam the users screen with initialization messages
	ScriptEngine::getSingleton().activateLogging();
#endif

	// update icollisions instance in factory
	BeamFactory::getSingleton().icollisions = collisions;

	if (person) person->setCollisions(collisions);
#ifdef USE_MYGUI
	if (GUI_Friction::getSingletonPtr())
		GUI_Friction::getSingleton().setCollisions(collisions);
#endif //MYGUI

	// advanced camera collision tools
	mCollisionTools = new MOC::CollisionTools(mSceneMgr);
	// set how far we want the camera to be above ground
	mCollisionTools->setHeightAdjust(0.2f);

}


void RoRFrameListener::loadTerrain(String terrainfile)
{
	ScopeLog log("terrain_"+terrainfile);

	// check if the resource is loaded
	if (!CACHE.checkResourceLoaded(terrainfile))
	{
		// fallback for terrains, add .terrn if not found and retry
		terrainfile  = terrainfile + ".terrn";
		if (!CACHE.checkResourceLoaded(terrainfile))
		{
			LOG("Terrain not found: " + terrainfile);
			showError(_L("Terrain loading error"), _L("Terrain not found: ") + terrainfile);
			exit(123);
		}
	}

	// set the terrain hash
	{
		Cache_Entry ce = CACHE.getResourceInfo(terrainfile);
		char hash_result[250];
		memset(hash_result, 0, 249);
		RoR::CSHA1 sha1;
		String fn;
		if (ce.type == "Zip")
			fn = ce.dirname;
		else if (ce.type == "FileSystem")
			fn = ce.dirname + SSETTING("dirsep", "\\") + ce.fname;
		sha1.HashFile(const_cast<char*>(fn.c_str()));
		sha1.Final();
		sha1.ReportHash(hash_result, RoR::CSHA1::REPORT_HEX_SHORT);
		terrainModHash = String(hash_result);
	}

	loadedTerrain = terrainfile;

	initializeCompontents();

	if (terrainfile.find(".terrn") != String::npos)
	{
		LOG("Loading classic terrain format: " + terrainfile);
		loadClassicTerrain(terrainfile);
	} else
	{
		// exit on unknown terrain handler
		LOG("Terrain not supported, unknown format: " + terrainfile);
		showError(_L("Terrain loading error"), _L("Terrain not supported, unknown format: ") + terrainfile);
		exit(1);
	}

#ifdef USE_MYGUI
	if (!BSETTING("REPO_MODE", false))
	{
		// hide loading window
		LoadingWindow::getSingleton().hide();
		// hide wallpaper
		MyGUI::Window *w = MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("wallpaper");
		if (w) w->setVisibleSmooth(false);
	}
#endif // USE_MYGUI

	if (person) person->setVisible(true);
}

void RoRFrameListener::loadClassicTerrain(String terrainfile)
{
	//we load a classic terrain
	//FILE *fd;
	char geom[1024];
	char sandstormcubemap[256]="";
	char line[1024];
	float r,g,b;
	float cx,cy,cz;
	String group = "";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(terrainfile);
	} catch(...) {}
	if (group == "")
	{
		// we need to do a bit more here, since this can also happen on joining a MP server, in that case the user should get a better error message ...
		LOG("Terrain not found: " + String(terrainfile));
		showError(_L("Terrain loading error"), _L("Terrain not found: ") + terrainfile);
		exit(125);
	}

	// set the terrain cache entry
	//loaded_terrain = CACHE.getResourceInfo(terrainfile);

	terrainFileName = terrainfile;

	DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource(terrainfile, group);

	// now generate the hash of it
	{
		// copy whole file into a string
		String code = "";
		ds->seek(0); // from start
		code.resize(ds->size());
		ds->read(&code[0], ds->size());

		// and build the hash over it
		char hash_result[250];
		memset(hash_result, 0, 249);
		RoR::CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)code.c_str(), (uint32_t)code.size());
		sha1.Final();
		sha1.ReportHash(hash_result, RoR::CSHA1::REPORT_HEX_SHORT);
		terrainFileHash = String(hash_result);
		ds->seek(0); // to start
	}

	ds->readLine(line, 1023);
	terrainName = String(line);

	//geometry
	ds->readLine(geom, 1023);

	// otc = ogre terrain config
	if (String(geom).find(".otc") != String::npos)
	{
		LOG("new terrain mode enabled");
		SETTINGS.setSetting("new Terrain Mode", "Yes");
	}


	//colour
	ds->readLine(line, 1023);
	//water stuff
	float waterline=-9999;
	if (line[0]=='w')
	{
		sscanf(line+1, "%f", &waterline);
		//fscanf(fd," %[^\n\r]",line);
		ds->readLine(line, 1023);
	};
	//Caelum maps
	if (!strncmp(line,"caelum", 6))
	{
		// deprecated
		ds->readLine(line, 1023);
	};

	sscanf(line,"%f, %f, %f",&r,&g,&b);
	//coordinates
	//fscanf(fd," %[^\n\r]",line);
	ds->readLine(line, 1023);
	sscanf(line, "%f, %f, %f, %f, %f, %f, %f, %f, %f",&truckx,&trucky,&truckz,&cx,&cy,&cz, &persostart.x, &persostart.y, &persostart.z);
	spawn_location_t spl;
	memset(&spl, 0, sizeof(spl));
	spl.pos = Vector3(truckx, trucky, truckz);
	spl.rot = Quaternion::ZERO;
	netSpawnPos["truck"] = spl;
	netSpawnPos["airplane"] = spl;
	netSpawnPos["boat"] = spl;
	netSpawnPos["car"] = spl;

#if OGRE_VERSION>0x010602
	Vector4 splitPoints;
#endif

	//shadows
	new ShadowManager(mSceneMgr, mWindow, mCamera);
	ShadowManager::getSingleton().loadConfiguration();

	ColourValue fadeColour(r,g,b);

	terrainxsize=1000;
	terrainzsize=1000;

	bool disableTetrrain=false;

	{
	    ConfigFile config;
		ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
		String group = "";
		try
		{
			group = ResourceGroupManager::getSingleton().findGroupContainingResource(geom);
		} catch(...) {}
		if (group == "")
		{
			LOG("Geometry not found: " + String(terrainfile));
			showError(_L("Geometry loading error"), _L("Geometry not found: ") + String(geom));
			exit(125);
		}
		DataStreamPtr stream=rgm.openResource(geom, group);
		config.load( stream );
	    String val;
		float fval=0.0f;
		val = config.getSetting("PageWorldX");
		if ( !val.empty() )
			fval = atof( val.c_str() );
		terrainxsize=fval;
		val = config.getSetting("PageWorldZ");
		if ( !val.empty() )
			fval = atof( val.c_str() );
		terrainzsize=fval;
		disableTetrrain = (config.getSetting("disable") != "");
	}

	//mCamera->setNearClipDistance (0.01);
	//mCamera->setFarClipDistance( farclip*1.733 );
	int farclip = ISETTING("SightRange", 2000);
	bool inifite_farclip = false;
	
	if (farclip == 5000 && mRoot->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_INFINITE_FAR_PLANE))
	{
		mCamera->setFarClipDistance(0);   // enable infinite far clip distance if we can
		inifite_farclip = true;
	} else
	{
		farclip = std::min((float)farclip, terrainzsize * 1.8f);
		mCamera->setFarClipDistance(farclip);
	}

	Light *mainLight = 0;
	mSceneMgr->setFog(FOG_NONE);

#ifdef USE_CAELUM
	//Caelum skies
	bool useCaelum = SSETTING("Sky effects", "Caelum (best looking, slower)")=="Caelum (best looking, slower)";
	if (useCaelum)
	{
		new SkyManager();

		SkyManager::getSingleton().init(mSceneMgr, mWindow, mCamera);
		mainLight = SkyManager::getSingleton().getMainLight();
	} else
#endif //CAELUM
	{
		// Create a light
		mainLight = mSceneMgr->createLight("MainLight");
		//directional light for shadow
		mainLight->setType(Light::LT_DIRECTIONAL);
		mainLight->setDirection(0.785, -0.423, 0.453);

		mainLight->setDiffuseColour(fadeColour);
		mainLight->setSpecularColour(fadeColour);

		//mSceneMgr->setSkyBox(true, sandstormcubemap, farclip);
		//mSceneMgr->setSkyDome(true, "Examples/CloudySky", 5, 8);
		
		if (!inifite_farclip)
			mSceneMgr->setFog(FOG_LINEAR, fadeColour,  0, farclip * 0.7, farclip * 0.9);
	}
	mCamera->getViewport()->setBackgroundColour(fadeColour);
	

#ifdef USE_CAELUM
	// load caelum config
	if (SSETTING("Sky effects", "Caelum (best looking, slower)")=="Caelum (best looking, slower)")
	{
		String cfn = terrainfile + ".os";
		if (ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(cfn))
			SkyManager::getSingleton().loadScript(cfn);
		else
			SkyManager::getSingleton().loadScript("ror_default_sky");
	}
#endif //USE_CAELUM


	bool newTerrainMode = (BSETTING("new Terrain Mode", false));

	MaterialPtr terMat = (MaterialPtr)(MaterialManager::getSingleton().getByName("TerrainSceneManager/Terrain"));
	{
		// load configuration from STM (sizes)
		ConfigFile cfg;
		String group="";
		try
		{
			group = ResourceGroupManager::getSingleton().findGroupContainingResource(String(geom));
		}catch(...)
		{
		}
		if (group == "")
			return;
		DataStreamPtr ds_config = ResourceGroupManager::getSingleton().openResource(String(geom), group);
		cfg.load(ds_config, "\t:=", false);

		// X, Y and Z scale
		mapsizex = StringConverter::parseInt(cfg.getSetting("PageWorldX"));
		mapsizey = StringConverter::parseInt(cfg.getSetting("MaxHeight"));
		mapsizez = StringConverter::parseInt(cfg.getSetting("PageWorldZ"));

#ifdef USE_MYGUI
		if (surveyMap)
		{
			surveyMap->setWorldSize(mapsizex, mapsizey, mapsizez);
		}
#endif //MYGUI

		if (!newTerrainMode && !disableTetrrain)
		{
			// classic mode
			mSceneMgr->setWorldGeometry(geom);
		} else if (newTerrainMode && !disableTetrrain)
		{
			terrainManager = new TerrainManager(mSceneMgr);
			terrainManager->loadTerrain(geom);
		}
	}


	// get vegetation mode
	int pagedMode = 0; //None
	float pagedDetailFactor = 0;
	String vegetationMode = SSETTING("Vegetation", "None (fastest)");
	if     (vegetationMode == "None (fastest)")
	{
		pagedMode = 0;
		pagedDetailFactor = 0.001;
	}
	else if (vegetationMode == "20%")
	{
		pagedMode = 1;
		pagedDetailFactor = 0.2;
	}
	else if (vegetationMode == "50%")
	{
		pagedMode = 2;
		pagedDetailFactor = 0.5;
	}
	else if (vegetationMode == "Full (best looking, slower)")
	{
		pagedMode = 3;
		pagedDetailFactor = 1;
	}

	// Define the required skyplane
	Plane plane;
	// 1000 world units from the camera
	plane.d = 200;
	// Above the camera, facing down
	plane.normal = -Vector3::UNIT_Y;


	if (!terMat.isNull())
		terrainmaterial = terMat.get();
	if (terrainmaterial)
		LOG("using Terrain Material '"+terrainmaterial->getName()+"'");

	//create sky material
	//			MaterialPtr skmat=(MaterialPtr)(MaterialManager::getSingleton().create("Skycol", "Standard"));
	//			Technique* sktechnique = skmat->getTechnique(0);
	//			Pass* skpass = sktechnique->getPass(0);
	//			skpass->setDepthWriteEnabled(false);
	//			skpass->setLightingEnabled(false);
	//			TextureUnitState* sktunit=skpass->createTextureUnitState();
	//			sktunit->setColourOperationEx(LBX_MODULATE, LBS_MANUAL, LBS_CURRENT, fadeColour);

	//		mSceneMgr->setSkyPlane(true, plane, "tracks/skyplanecol", 1000, 1, true, 0.5, 100, 100);
	//		mSceneMgr->setSkyBox(true, "tracks/skycol", 1000);

	//bloom effect
	/*
	// replaced by HDR
	if (BSETTING("Bloom"))
	{
		CompositorManager::getSingleton().addCompositor(mCamera->getViewport(),"Bloom");
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Bloom", true);
	}*/

	// first compositor: HDR!
	// HDR if wished
	if (BSETTING("HDR", false))
	{
		initHDR();
	}

	if (BSETTING("Glow", false))
	{
		CompositorManager::getSingleton().addCompositor(mCamera->getViewport(), "Glow");
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Glow", true);
		GlowMaterialListener *gml = new GlowMaterialListener();
		Ogre::MaterialManager::getSingleton().addListener(gml);
	}

	// Motion blur stuff :)
	if (BSETTING("Motion blur", false))
	{
		/// Motion blur effect
		CompositorPtr comp3 = CompositorManager::getSingleton().create(
			"MotionBlur", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
			);
		{
			CompositionTechnique *t = comp3->createTechnique();
			{
				CompositionTechnique::TextureDefinition *def = t->createTextureDefinition("scene");
				def->width = 0;
				def->height = 0;
#if OGRE_VERSION>0x010602
				def->formatList.push_back(PF_R8G8B8);
#else
				def->format = PF_R8G8B8;
#endif //OGRE_VERSION
			}
			{
				CompositionTechnique::TextureDefinition *def = t->createTextureDefinition("sum");
				def->width = 0;
				def->height = 0;
#if OGRE_VERSION>0x010602
				def->formatList.push_back(PF_R8G8B8);
#else
				def->format = PF_R8G8B8;
#endif //OGRE_VERSION
			}
			{
				CompositionTechnique::TextureDefinition *def = t->createTextureDefinition("temp");
				def->width = 0;
				def->height = 0;
#if OGRE_VERSION>0x010602
				def->formatList.push_back(PF_R8G8B8);
#else
				def->format = PF_R8G8B8;
#endif //OGRE_VERSION
			}
			/// Render scene
			{
				CompositionTargetPass *tp = t->createTargetPass();
				tp->setInputMode(CompositionTargetPass::IM_PREVIOUS);
				tp->setOutputName("scene");
			}
			/// Initialisation pass for sum texture
			{
				CompositionTargetPass *tp = t->createTargetPass();
				tp->setInputMode(CompositionTargetPass::IM_PREVIOUS);
				tp->setOutputName("sum");
				tp->setOnlyInitial(true);
			}
			/// Do the motion blur
			{
				CompositionTargetPass *tp = t->createTargetPass();
				tp->setInputMode(CompositionTargetPass::IM_NONE);
				tp->setOutputName("temp");
				{ CompositionPass *pass = tp->createPass();
				pass->setType(CompositionPass::PT_RENDERQUAD);
				pass->setMaterialName("Ogre/Compositor/Combine");
				pass->setInput(0, "scene");
				pass->setInput(1, "sum");
				}
			}
			/// Copy back sum texture
			{
				CompositionTargetPass *tp = t->createTargetPass();
				tp->setInputMode(CompositionTargetPass::IM_NONE);
				tp->setOutputName("sum");
				{ CompositionPass *pass = tp->createPass();
				pass->setType(CompositionPass::PT_RENDERQUAD);
				pass->setMaterialName("Ogre/Compositor/Copyback");
				pass->setInput(0, "temp");
				}
			}
			/// Display result
			{
				CompositionTargetPass *tp = t->getOutputTargetPass();
				tp->setInputMode(CompositionTargetPass::IM_NONE);
				{ CompositionPass *pass = tp->createPass();
				pass->setType(CompositionPass::PT_RENDERQUAD);
				pass->setMaterialName("Ogre/Compositor/MotionBlur");
				pass->setInput(0, "sum");
				}
			}
		}
		CompositorManager::getSingleton().addCompositor(mCamera->getViewport(),"MotionBlur");
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "MotionBlur", true);
	}
	// End of motion blur :(

	//SUNBURN
	if (BSETTING("Sunburn", false))
	{
		CompositorManager::getSingleton().addCompositor(mCamera->getViewport(),"Sunburn");
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Sunburn", true);
	}



	//hack
	// now with extensive error checking
	if (CompositorManager::getSingleton().hasCompositorChain(mCamera->getViewport()))
	{
	//	//CompositorManager::getSingleton().getCompositorChain(mCamera->getViewport())->getCompositor(0)->getTechnique()->getOutputTargetPass()->getPass(0)->setClearColour(fadeColour);
		CompositorInstance *co = CompositorManager::getSingleton().getCompositorChain(mCamera->getViewport())->_getOriginalSceneCompositor();
		if (co)
		{
			CompositionTechnique *ct = co->getTechnique();
			if (ct)
			{
				CompositionTargetPass *ctp = ct->getOutputTargetPass();
				if (ctp)
				{
					CompositionPass *p = ctp->getPass(0);
					if (p)
						p->setClearColour(fadeColour);
				}
			}
		}
	}


	// not such ugly anymore: read .raw heightmap filename out of the terrain .cfg file
	{
		ConfigFile config;
		ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
		String group="";
		try
		{
			group = ResourceGroupManager::getSingleton().findGroupContainingResource(String(geom));
		}catch(...)
		{
		}
		if (group == "")
			return;
		DataStreamPtr stream=rgm.openResource(geom, group);
		config.load(stream);
		String val = config.getSetting("Heightmap.image");
		if (!val.empty())
		{
			strcpy(terrainmap, val.c_str());
		} else
		{
			// bad ugly hack
			strcpy(terrainmap, geom);
			terrainmap[strlen(terrainmap)-3]='r';
			terrainmap[strlen(terrainmap)-2]='a';
			terrainmap[strlen(terrainmap)-1]='w';
		}
		//ensure file gets closed again using this braces
	}

	//set terrain map
	//MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/BigMap"));
	//if (terrainmaterial && bigMap)
	//mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(terrainmaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName());
	//	bigMap->setBackground(terrainmaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName());

	mCamera->setPosition(Vector3(cx,cy,cz));

	//water!
	if (waterline != -9999)
	{
		bool usewaves=(BSETTING("Waves", false));

		// disable waves in multiplayer
		if (net)
			usewaves=false;

		String waterSettingsString = SSETTING("Water effects", "Reflection + refraction (speed optimized)");

		if      (waterSettingsString == "None")
			w = 0;
		else if (waterSettingsString == "Basic (fastest)")
			w = new WaterOld(WaterOld::WATER_BASIC, mCamera, mSceneMgr, mWindow, waterline, &mapsizex, &mapsizez, usewaves);
		else if (waterSettingsString == "Reflection")
			w = new WaterOld(WaterOld::WATER_REFLECT, mCamera, mSceneMgr, mWindow, waterline, &mapsizex, &mapsizez, usewaves);
		else if (waterSettingsString == "Reflection + refraction (speed optimized)")
			w = new WaterOld(WaterOld::WATER_FULL_SPEED, mCamera, mSceneMgr, mWindow, waterline, &mapsizex, &mapsizez, usewaves);
		else if (waterSettingsString == "Reflection + refraction (quality optimized)")
			w = new WaterOld(WaterOld::WATER_FULL_QUALITY, mCamera, mSceneMgr, mWindow, waterline, &mapsizex, &mapsizez, usewaves);
	}
	if (w) w->setFadeColour(fadeColour);
	if (person) person->setWater(w);
	BeamFactory::getSingleton().w = w;
	DustManager::getSingleton().setWater(w);

	//environment map
	if (!BSETTING("Envmapdisable", false))
	{
		envmap = new Envmap(mSceneMgr, mWindow, mCamera, BSETTING("Envmap", false), ISETTING("EnvmapUpdateRate", 1));
	}

	//dashboard
	dashboard = new Dashboard(mSceneMgr);

	//setup heightfinder
	float wheight = 0.0f;
	if (w)
	{
		wheight = w->getHeight() - 30.0f;
	}

	// we choose the heightfinder depending on whether we use the classical
	// terrain or the new one
	if (newTerrainMode)
		hfinder = new NTHeightFinder(terrainManager, Vector3::ZERO);
	else
		hfinder = new TSMHeightFinder(geom, terrainmap, wheight);

	collisions->setHfinder(hfinder);
	if (person)
	{
		person->setHFinder(hfinder);
	}

	// update hfinder instance in factory
	BeamFactory::getSingleton().mfinder = hfinder;

	// set camera to some nice spot, overviewing the terrain, showing the loading progress
	if (spl.pos != Vector3::ZERO)    mCamera->setPosition(spl.pos);
	if (spl.rot != Quaternion::ZERO) mCamera->setOrientation(spl.rot);

#ifdef USE_MYGUI
	if (surveyMap)
	{
		mtc = new MapTextureCreator(mSceneMgr, mCamera, surveyMap);
		surveyMap->setMapTexture(mtc->getRTName());
	}
#endif //USE_MYGUI

	// fix the person starting position
	if (persostart.isZeroLength() && !spl.pos.isZeroLength())
	{
		if (hfinder)
			persostart = Vector3(spl.pos.x, hfinder->getHeightAt(spl.pos.x, spl.pos.z), spl.pos.z);
		else
			persostart = spl.pos;
	}

	//prepare road2
	proceduralManager = new ProceduralManager(mSceneMgr, hfinder, collisions);

	Vector3 r2lastpos=Vector3::ZERO;
	Quaternion r2lastrot=Quaternion::IDENTITY;

	//prepare for baking
	SceneNode *bakeNode=mSceneMgr->getRootSceneNode()->createChildSceneNode();

#ifdef USE_PAGED
	treeLoader = 0;
	Entity *curTree = 0;
	String treename = "";
#endif

	ProceduralObject po;
	po.loadingState = -1;
	int r2oldmode = 0;
	int lastprogress = -1;
	bool proroad = false;

	while (!ds->eof())
	{
		int progress = ((float)(ds->tell()) / (float)(ds->size())) * 100.0f;
		if (progress-lastprogress > 20)
		{
#ifdef USE_MYGUI
			LoadingWindow::getSingleton().setProgress(progress, _L("Loading Terrain"));
#endif //MYGUI
			lastprogress = progress;
		}

		char oname[1024];
		char type[256];
		char name[256];
		int r;
		float ox, oy, oz;
		float rx, ry, rz;
		//fscanf(fd," %[^\n\r]",line);
		size_t ll=ds->readLine(line, 1023);
		if (line[0]=='/' || line[0]==';' || ll==0) continue; //comments
		if (!strcmp("end",line)) break;
		
		if (!strncmp(line,"collision-tris", 14))
		{
			long amount = Collisions::MAX_COLLISION_TRIS;
			sscanf(line, "collision-tris %ld", &amount);
			collisions->resizeMemory(amount);
		}

		if (!strncmp(line,"grid", 4))
		{
			float px=0,py=0,pz=0;
			sscanf(line, "grid %f, %f, %f", &px, &py, &pz);
			Vector3 pos = Vector3(px,py,pz);

			Ogre::ColourValue BackgroundColour = Ogre::ColourValue::White;//Ogre::ColourValue(0.1337f, 0.1337f, 0.1337f, 1.0f);
			Ogre::ColourValue GridColour = Ogre::ColourValue(0.2f, 0.2f, 0.2f, 1.0f);
		
			Ogre::ManualObject *mReferenceObject = new Ogre::ManualObject("ReferenceGrid");

			mReferenceObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST);

			Ogre::Real step = 1.0f;
			unsigned int count = 50;
			unsigned int halfCount = count / 2;
			Ogre::Real full = (step * count);
			Ogre::Real half = full / 2;
			Ogre::Real y = 0;
			Ogre::ColourValue c;
			for (unsigned i=0;i < count+1;i++)
			{
				if (i == halfCount)
					c = Ogre::ColourValue(1,0,0,1.0f);
				else
					c = GridColour;

				mReferenceObject->position(-half,y,-half+(step*i));
				mReferenceObject->colour(BackgroundColour);
				mReferenceObject->position(0,y,-half+(step*i));
				mReferenceObject->colour(c);
				mReferenceObject->position(0,y,-half+(step*i));
				mReferenceObject->colour(c);
				mReferenceObject->position(half,y,-half+(step*i));
				mReferenceObject->colour(BackgroundColour);

				if (i == halfCount)
					c = Ogre::ColourValue(0,0,1,1.0f);
				else
					c = GridColour;

				mReferenceObject->position(-half+(step*i),y,-half);
				mReferenceObject->colour(BackgroundColour);
				mReferenceObject->position(-half+(step*i),y,0);
				mReferenceObject->colour(c);
				mReferenceObject->position(-half+(step*i),y,0);
				mReferenceObject->colour(c);
				mReferenceObject->position(-half+(step*i),y, half);
				mReferenceObject->colour(BackgroundColour);
			}

			mReferenceObject->end();
			mReferenceObject->setCastShadows(false);
			SceneNode *n = mSceneMgr->getRootSceneNode()->createChildSceneNode();
			n->setPosition(pos);
			n->attachObject(mReferenceObject);
			n->setVisible(true);
		}

		if (!strncmp("mpspawn", line, 7))
		{
			spawn_location_t spl;
			memset(&spl, 0, sizeof(spawn_location_t));

			char tmp[256]="";
			float x=0,y=0,z=0, rx=0, ry=0, rz=0;
			int res = sscanf(line, "mpspawn %s %f %f %f %f %f %f", tmp, &x, &y, &z, &rx, &ry, &rz);
			if (res < 7)
			{
				LOG("error reading mpspawn command!");
				continue;
			}
			spl.pos = Vector3(x, y, z);
			spl.rot = Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);
			netSpawnPos[String(tmp)] = spl;
			continue;
		}

		//sandstorm cube texture
		if (!strncmp(line,"sandstormcubemap", 16))
		{
			sscanf(line, "sandstormcubemap %s", sandstormcubemap);
		}
		if (!strncmp("landuse-config", line, 14))
		{
			collisions->setupLandUse(line+15);
			continue;
		}
		if (!strncmp("gravity", line, 7))
		{
			int res = sscanf(line, "gravity %f", &gravity);
			if (res < 1)
			{
				LOG("error reading gravity command!");
			}
			continue;
		}
		//ugly stuff to parse map size
		if (!strncmp("mapsize", line, 7))
		{
			// this is deprecated!!! (replaced by direct .cfg reads)
			//sscanf(line, "mapsize %f, %f", &mapsizex, &mapsizez);
			continue;
		}
#ifdef USE_PAGED
		//ugly stuff to parse trees :)
		if (!strncmp("trees", line, 5))
		{
			if (pagedMode == 0) continue;
			char ColorMap[256] = {};
			char DensityMap[256] = {};
			char treemesh[256] = {};
			char treeCollmesh[256] = {};
			float gridspacing = 0.0f;
			float yawfrom = 0.0f, yawto = 0.0f;
			float scalefrom = 0.0f, scaleto = 0.0f;
			float highdens = 1.0f;
			int minDist = 90, maxDist = 700;
			sscanf(line, "trees %f, %f, %f, %f, %f, %d, %d, %s %s %s %f %s", &yawfrom, &yawto, &scalefrom, &scaleto, &highdens, &minDist, &maxDist, treemesh, ColorMap, DensityMap, &gridspacing, treeCollmesh);
			if (strnlen(ColorMap, 3) == 0)
			{
				LOG("tree ColorMap map zero!");
				continue;
			}
			if (strnlen(DensityMap, 3) == 0)
			{
				LOG("tree DensityMap zero!");
				continue;
			}
			Forests::DensityMap *densityMap = Forests::DensityMap::load(DensityMap, CHANNEL_COLOR);
			if (!densityMap)
			{
				LOG("could not load densityMap: "+String(DensityMap));
				continue;
			}
			densityMap->setFilter(Forests::MAPFILTER_BILINEAR);
			//densityMap->setMapBounds(TRect(0, 0, mapsizex, mapsizez));

			paged_geometry_t paged;
			paged.geom = new PagedGeometry();
			paged.geom->setTempDir(SSETTING("User Path", "") + "cache" + SSETTING("dirsep", "\\"));
			paged.geom->setCamera(mCamera);
			paged.geom->setPageSize(50);
			paged.geom->setInfinite();
			Ogre::TRect<Ogre::Real> bounds = TBounds(0, 0, mapsizex, mapsizez);
			//trees->setBounds(bounds);

			//Set up LODs
			//trees->addDetailLevel<EntityPage>(50);
			float min = minDist * pagedDetailFactor;
			if (min<10) min = 10;
			paged.geom->addDetailLevel<BatchPage>(min, min/2);
			float max = maxDist * pagedDetailFactor;
			if (max<10) max = 10;
			paged.geom->addDetailLevel<ImpostorPage>(max, max/10);
			TreeLoader2D *treeLoader = new TreeLoader2D(paged.geom, TBounds(0, 0, mapsizex, mapsizez));
			paged.geom->setPageLoader(treeLoader);
			treeLoader->setHeightFunction(&getTerrainHeight);
			if (String(ColorMap) != "none")
			{
				treeLoader->setColorMap(ColorMap);
			}

			curTree = mSceneMgr->createEntity(String("paged_")+treemesh+TOSTRING(pagedGeometry.size()), treemesh);

			if (gridspacing > 0)
			{
				// grid style
				for(float x=0; x < mapsizex; x += gridspacing)
				{
					for(float z=0; z < mapsizez; z += gridspacing)
					{
						float density = densityMap->_getDensityAt_Unfiltered(x, z, bounds);
						if (density < 0.8f) continue;
						float nx = x + gridspacing * 0.5f;
						float nz = z + gridspacing * 0.5f;
						float yaw = Math::RangeRandom(yawfrom, yawto);
						float scale = Math::RangeRandom(scalefrom, scaleto);
						Vector3 pos = Vector3(nx, 0, nz);
						treeLoader->addTree(curTree, pos, Degree(yaw), (Ogre::Real)scale);
						if (strlen(treeCollmesh))
						{
							pos.y = hfinder->getHeightAt(pos.x, pos.z);
							scale *= 0.1f;
							collisions->addCollisionMesh(String(treeCollmesh), pos, Quaternion(Degree(yaw), Vector3::UNIT_Y), Vector3(scale, scale, scale));
						}
					}
				}

			} else
			{
				float gridsize = 10;
				if (gridspacing < 0 && gridspacing != 0)
				{
					gridsize = -gridspacing;
				}
				float hd = highdens;
				// normal style, random
				for(float x=0; x < mapsizex; x += gridsize)
				{
					for(float z=0; z < mapsizez; z += gridsize)
					{
						if (highdens < 0) hd = Math::RangeRandom(0, -highdens);
						float density = densityMap->_getDensityAt_Unfiltered(x, z, bounds);
						int numTreesToPlace = (int)((float)(hd) * density * pagedDetailFactor);
						float nx=0, nz=0;
						while(numTreesToPlace-->0)
						{
							nx = Math::RangeRandom(x, x + gridsize);
							nz = Math::RangeRandom(z, z + gridsize);
							float yaw = Math::RangeRandom(yawfrom, yawto);
							float scale = Math::RangeRandom(scalefrom, scaleto);
							Vector3 pos = Vector3(nx, 0, nz);
							treeLoader->addTree(curTree, pos, Degree(yaw), (Ogre::Real)scale);
							if (strlen(treeCollmesh))
							{
								pos.y = hfinder->getHeightAt(pos.x, pos.z);
								collisions->addCollisionMesh(String(treeCollmesh),pos, Quaternion(Degree(yaw), Vector3::UNIT_Y), Vector3(scale, scale, scale));
							}
						}
					}
				}
			}
			paged.loader = (void*)treeLoader;
			pagedGeometry.push_back(paged);
		}

		//ugly stuff to parse grass :)
		if (!strncmp("grass", line, 5) || !strncmp("grass2", line, 6))
		{
			// is paged geometry disabled by configuration?
			if (pagedMode == 0) continue;
			int range = 80;
			float SwaySpeed=0.5, SwayLength=0.05, SwayDistribution=10.0, minx=0.2, miny=0.2, maxx=1, maxy=0.6, Density=0.6, minH=-9999, maxH=9999;
			char grassmat[256]="";
			char ColorMap[256]="";
			char DensityMap[256]="";
			int growtechnique = 0;
			int techn = GRASSTECH_CROSSQUADS;
			if (!strncmp("grass2", line, 6))
				sscanf(line, "grass2 %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %d, %s %s %s", &range, &SwaySpeed, &SwayLength, &SwayDistribution, &Density, &minx, &miny, &maxx, &maxy, &growtechnique, &minH, &maxH, &techn, grassmat, ColorMap, DensityMap);
			else if (!strncmp("grass", line, 5))
				sscanf(line, "grass %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %s %s %s", &range, &SwaySpeed, &SwayLength, &SwayDistribution, &Density, &minx, &miny, &maxx, &maxy, &growtechnique, &minH, &maxH, grassmat, ColorMap, DensityMap);

			//Initialize the PagedGeometry engine
			try
			{
				paged_geometry_t paged;
				PagedGeometry *grass = new PagedGeometry(mCamera, 30);
				//Set up LODs

				grass->addDetailLevel<GrassPage>(range * pagedDetailFactor); // original value: 80

				//Set up a GrassLoader for easy use
				GrassLoader *grassLoader = new GrassLoader(grass);
				grass->setPageLoader(grassLoader);
				grassLoader->setHeightFunction(&getTerrainHeight);

				// render grass at first
				grassLoader->setRenderQueueGroup(RENDER_QUEUE_MAIN-1);

				GrassLayer* grassLayer = grassLoader->addLayer(grassmat);
				grassLayer->setHeightRange(minH, maxH);
				//grassLayer->setLightingEnabled(true);

				grassLayer->setAnimationEnabled((SwaySpeed>0));
				grassLayer->setSwaySpeed(SwaySpeed);
				grassLayer->setSwayLength(SwayLength);
				grassLayer->setSwayDistribution(SwayDistribution);

				grassdensityTextureFilename = String(DensityMap);

				grassLayer->setDensity(Density * pagedDetailFactor);
				if (techn>10)
					grassLayer->setRenderTechnique(static_cast<GrassTechnique>(techn-10), true);
				else
					grassLayer->setRenderTechnique(static_cast<GrassTechnique>(techn), false);

				grassLayer->setMapBounds(TBounds(0, 0, mapsizex, mapsizez));

				if (strcmp(ColorMap,"none") != 0)
				{
					grassLayer->setColorMap(ColorMap);
					grassLayer->setColorMapFilter(MAPFILTER_BILINEAR);
				}

				if (strcmp(DensityMap,"none") != 0)
				{
					grassLayer->setDensityMap(DensityMap);
					grassLayer->setDensityMapFilter(MAPFILTER_BILINEAR);
				}

				//grassLayer->setMinimumSize(0.5,0.5);
				//grassLayer->setMaximumSize(1.0, 1.0);

				grassLayer->setMinimumSize(minx, miny);
				grassLayer->setMaximumSize(maxx, maxy);

				// growtechnique
				if (growtechnique == 0)
					grassLayer->setFadeTechnique(FADETECH_GROW);
				else if (growtechnique == 1)
					grassLayer->setFadeTechnique(FADETECH_ALPHAGROW);
				else if (growtechnique == 2)
					grassLayer->setFadeTechnique(FADETECH_ALPHA);
				paged.geom = grass;
				paged.loader = (void*)grassLoader;
				pagedGeometry.push_back(paged);
			} catch(...)
			{
				LOG("error loading grass!");
			}

			continue;
		}
#endif //USE_PAGED

		{ // ugly stuff to parse procedural roads
			if (!strncmp("begin_procedural_roads", line, 22))
			{
				po = ProceduralObject();
				po.loadingState = 1;
				r2oldmode = 1;
				proroad = true;
				continue;
			}
			if (!strncmp("end_procedural_roads", line, 20))
			{
				if (r2oldmode)
				{
					if (proceduralManager)
						proceduralManager->addObject(po);
					po = ProceduralObject();
				}
				proroad = false;
				continue;
			}
			if (proroad)
			{
				float rwidth, bwidth, bheight;
				//position x,y,z rotation rx,ry,rz, width, border width, border height, type
				r=sscanf(line, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %s",&ox,&oy,&oz, &rx, &ry, &rz, &rwidth, &bwidth, &bheight, oname);
				Vector3 pos=Vector3(ox, oy, oz);
				Quaternion rotation = Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);
				int roadtype=Road2::ROAD_AUTOMATIC;
				int pillartype = 0;
				if (!strcmp(oname, "flat")) roadtype=Road2::ROAD_FLAT;
				if (!strcmp(oname, "left")) roadtype=Road2::ROAD_LEFT;
				if (!strcmp(oname, "right")) roadtype=Road2::ROAD_RIGHT;
				if (!strcmp(oname, "both")) roadtype=Road2::ROAD_BOTH;
				if (!strcmp(oname, "bridge")) {roadtype=Road2::ROAD_BRIDGE;pillartype=1;}
				if (!strcmp(oname, "monorail")) {roadtype=Road2::ROAD_MONORAIL;pillartype=2;}
				if (!strcmp(oname, "monorail2")) {roadtype=Road2::ROAD_MONORAIL;pillartype=0;}
				if (!strcmp(oname, "bridge_no_pillars")) {roadtype=Road2::ROAD_BRIDGE;pillartype=0;}

				if (r2oldmode)
				{
					//fill object
					ProceduralPoint pp;
					pp.bheight = bheight;
					pp.bwidth = bwidth;
					pp.pillartype = pillartype;
					pp.position = pos;
					pp.rotation = rotation;
					pp.type = roadtype;
					pp.width = rwidth;

					po.points.push_back(pp);
				}
				continue;
			}
		} //end of the ugly (somewhat)

		strcpy(name, "generic");
		memset(oname, 0, 255);
		memset(type, 0, 255);
		memset(name, 0, 255);
		r=sscanf(line, "%f, %f, %f, %f, %f, %f, %s %s %s",&ox,&oy,&oz, &rx, &ry, &rz, oname, type, name);
		if (r<6)
			continue;
		if ((!strcmp(oname, "truck")) || (!strcmp(oname, "load") || (!strcmp(oname, "machine")) || (!strcmp(oname, "boat")) || (!strcmp(oname, "truck2")) ))
		{
			bool newFormat = (!strcmp(oname, "truck2"));

			if (!strcmp(oname, "boat") && !w)
				// no water so do not load boats!
				continue;
			String group="";
			String truckname=String(type);
			if (!CACHE.checkResourceLoaded(truckname, group))
			{
				LOG("Error while loading Terrain: truck " + String(type) + " not found. ignoring.");
				continue;
			}
			//this is a truck or load declaration
			truck_preload[truck_preload_num].px=ox;
			truck_preload[truck_preload_num].py=oy;
			truck_preload[truck_preload_num].pz=oz;
			truck_preload[truck_preload_num].freePosition = newFormat;
			truck_preload[truck_preload_num].ismachine=(!strcmp(oname, "machine"));
			truck_preload[truck_preload_num].rotation=Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);
			//truck_preload[truck_preload_num].ry=ry;
			strcpy(truck_preload[truck_preload_num].name, truckname.c_str());
			truck_preload_num++;
			continue;
		}
		if (   !strcmp(oname, "road")
			|| !strcmp(oname, "roadborderleft")
			|| !strcmp(oname, "roadborderright")
			|| !strcmp(oname, "roadborderboth")
			|| !strcmp(oname, "roadbridgenopillar")
			|| !strcmp(oname, "roadbridge"))
		{
			int pillartype = !(strcmp(oname, "roadbridgenopillar") == 0);
			//okay, this is a job for roads2
			int roadtype=Road2::ROAD_AUTOMATIC;
			if (!strcmp(oname, "road")) roadtype=Road2::ROAD_FLAT;
			Vector3 pos=Vector3(ox, oy, oz);
			Quaternion rotation;
			rotation=Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);
			if ((pos-r2lastpos).length()>20.0)
			{
				//break the road
				if (r2oldmode!=0)
				{
					//fill object
					ProceduralPoint pp;
					pp.bheight = 0.2;
					pp.bwidth = 1.4;
					pp.pillartype = pillartype;
					pp.position = r2lastpos+r2lastrot*Vector3(10.0,0,0);
					pp.rotation = r2lastrot;
					pp.type = roadtype;
					pp.width = 8;
					po.points.push_back(pp);

					// finish it and start new object
					if (proceduralManager)
						proceduralManager->addObject(po);
					po = ProceduralObject();
					r2oldmode=1;
				}
				r2oldmode=1;
				// beginning of new
				ProceduralPoint pp;
				pp.bheight = 0.2;
				pp.bwidth = 1.4;
				pp.pillartype = pillartype;
				pp.position = pos;
				pp.rotation = rotation;
				pp.type = roadtype;
				pp.width = 8;
				po.points.push_back(pp);
			}
			else
			{
				//fill object
				ProceduralPoint pp;
				pp.bheight = 0.2;
				pp.bwidth = 1.4;
				pp.pillartype = pillartype;
				pp.position = pos;
				pp.rotation = rotation;
				pp.type = roadtype;
				pp.width = 8;
				po.points.push_back(pp);
			}
			r2lastpos=pos;
			r2lastrot=rotation;


			continue;
		}
		loadObject(oname, ox, oy, oz, rx, ry, rz, bakeNode, name, true, -1, type);
	}
	//fclose(fd);

	// ds closes automatically, so do not close it explicitly here:
	//ds->close();

	// finish the last road
	if (r2oldmode != 0)
	{
		//fill object
		ProceduralPoint pp;
		pp.bheight = 0.2;
		pp.bwidth = 1.4;
		pp.pillartype = 1;
		pp.position = r2lastpos+r2lastrot*Vector3(10.0,0,0);
		pp.rotation = r2lastrot;
		pp.type = Road2::ROAD_AUTOMATIC;
		pp.width = 8;
		po.points.push_back(pp);

		// finish it and start new object
		if (proceduralManager)
			proceduralManager->addObject(po);
	}


	// okay, now bake everything
	bakesg = mSceneMgr->createStaticGeometry("bakeSG");
	bakesg->setCastShadows(true);
	bakesg->addSceneNode(bakeNode);
	bakesg->setRegionDimensions(Vector3(farclip/2.0, 10000.0, farclip/2.0));
	bakesg->setRenderingDistance(farclip);
	try
	{
		bakesg->build();
		bakeNode->detachAllObjects();
		// crash under linux:
		//bakeNode->removeAndDestroyAllChildren();
	}catch(...)
	{
		LOG("error while baking roads. ignoring.");

	}

#ifdef USE_MYGUI
	// tell this the map, so it can change the drawing distance !
	if (mtc)
	{
		mtc->setStaticGeometry(bakesg);
	}
#endif //USE_MYGUI

	collisions->printStats();
	loading_state=TERRAIN_LOADED;

	// we set the sky this late, so the user can configure it ...
	if (SSETTING("Sky effects", "Caelum (best looking, slower)")!="Caelum (best looking, slower)")
	{
		if (strlen(sandstormcubemap)>0)
		{
			// use custom
			mSceneMgr->setSkyBox(true, sandstormcubemap, 100, true);
		} else
		{
			// use default
			mSceneMgr->setSkyBox(true, "tracks/skyboxcol", 100, true);
		}
	}

	if (debugCollisions)
		collisions->createCollisionDebugVisualization();

	// bake the decals
	//finishTerrainDecal();

#ifdef USE_MYGUI
	LoadingWindow::getSingleton().hide();
#endif //MYGUI

#if 0
	// put some trucks in a fancy circle
	// not functional purpose, just for screenshots
	{
		float dg = 0, rad = 10;
		for(int i=0;i < 200; i++)
		{
			dg += (Ogre::Math::PI * 2) / (10+rad);
			rad += 0.4f;
			if (dg > 2 * Ogre::Math::PI)
			{
				//rad += 10;
				dg -= 2 * Ogre::Math::PI;
			}
			Vector3 pos(cx + cos(dg) * rad, cy, cz + sin(dg) * rad);
			BeamFactory::getSingleton().createLocal(pos, Quaternion(Radian(-dg), Vector3::UNIT_Y), "semi.truck");
		}
		BeamFactory::getSingleton().sendAllTrucksSleeping();
	}
#endif // 0

	collisions->finishLoadingTerrain();
}

void RoRFrameListener::initTrucks(bool loadmanual, Ogre::String selected, Ogre::String selectedExtension, std::vector<Ogre::String> *truckconfig, bool enterTruck, Skin *skin)
{
	//we load truck
	char *selectedchr = const_cast< char *> (selected.c_str());

	if (loadmanual)
	{
		Beam *b = 0;
		Vector3 spawnpos = Vector3(truckx, trucky, truckz);
		Quaternion spawnrot = Quaternion::ZERO;

		if (net)
		{
			if (selectedExtension.size() > 0)
			{
				String nsp = SSETTING("net spawn location", "");
				if (!nsp.empty())
				{
					// override-able by cmd line
					spawnpos = Ogre::StringConverter::parseVector3(nsp);
					spawnrot = Quaternion::ZERO;
				} else
				{
					// classical, search start points
					try
					{
						spawnpos = netSpawnPos[selectedExtension].pos;
						spawnrot = netSpawnPos[selectedExtension].rot;
					} catch(...)
					{
						spawnpos = Vector3(truckx, trucky, truckz);
						spawnrot = Quaternion::ZERO;
					}
				}
			}
		}

		if (net)
			b = BeamFactory::getSingleton().createLocal(spawnpos, spawnrot, selectedchr, 0, false, flaresMode, truckconfig, skin);
		else
			b = BeamFactory::getSingleton().createLocal(Vector3(truckx, trucky, truckz), Quaternion::ZERO, selectedchr, 0, false, flaresMode, truckconfig, skin);

		if (enterTruck)
		{
			if (b)
				BeamFactory::getSingleton().setCurrentTruck(b->trucknum);
			else
				BeamFactory::getSingleton().setCurrentTruck(-1);
		}

#ifdef USE_MYGUI
		if (b && surveyMap)
		{
			MapEntity *e = surveyMap->createNamedMapEntity("Truck"+TOSTRING(b->trucknum), MapControl::getTypeByDriveable(b->driveable));
			if (e)
			{
				e->setState(DESACTIVATED);
				e->setVisibility(true);
				e->setPosition(truckx, truckz);
				e->setRotation(-Radian(b->getHeadingDirectionAngle()));
			}
		}
#endif //USE_MYGUI

		if (b && b->engine)
		{
			b->engine->start();
		}
	}
	
	// load the rest in SP
	// in netmode, don't load other trucks!
	if (!netmode)
	{
		for (int i=0; i<truck_preload_num; i++)
		{
			Beam *b = BeamFactory::getSingleton().createLocal(Vector3(truck_preload[i].px, truck_preload[i].py, truck_preload[i].pz), truck_preload[i].rotation, truck_preload[i].name, 0, truck_preload[i].ismachine, flaresMode, truckconfig, 0, truck_preload[i].freePosition);
#ifdef USE_MYGUI
			if (b && surveyMap)
			{
				MapEntity *e = surveyMap->createNamedMapEntity("Truck"+TOSTRING(b->trucknum), MapControl::getTypeByDriveable(b->driveable));
				if (e)
				{
					e->setState(DESACTIVATED);
					e->setVisibility(true);
					e->setPosition(truck_preload[i].px, truck_preload[i].pz);
					e->setRotation(-Radian(b->getHeadingDirectionAngle()));
				}
			}
#endif //USE_MYGUI
		}

	}
	LOG("EFL: beam instanciated");

	if (!enterTruck)
	{
		BeamFactory::getSingleton().setCurrentTruck(-1);
	}

	// fix for problem on loading
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

	if (enterTruck && curr_truck && curr_truck->free_node == 0)
	{
		BeamFactory::getSingleton().setCurrentTruck(-1);
	}

	//force perso start
	if (persostart != Vector3(0,0,0))
	{
		person->setPosition(persostart);
	}

	loading_state = ALL_LOADED;

	if (ISETTING("OutGauge Mode", 0) > 0)
	{
		new OutProtocol();
	}

	LOG("initTrucks done");

#ifdef USE_MYGUI
	GUIManager::getSingleton().unfocus();
	
	if (mtc)
	{
		Vector3 position = Vector3(mapsizex / 2.0f, 0.0f, mapsizez / 2.0f);
		mtc->setMapCenter(position);
		mtc->update();
	}
#endif //USE_MYGUI
		
	if (BSETTING("REPO_MODE", false))
	{
		PreviewRenderer r;
		r.render();
		exit(0);
	}
}

void RoRFrameListener::changedCurrentTruck(Beam *previousTruck, Beam *currentTruck)
{
	// hide any old dashes
	if (previousTruck && previousTruck->dash)
	{
		previousTruck->dash->setVisible3d(false);
	}
	// show new
	if (currentTruck && currentTruck->dash)
	{
		currentTruck->dash->setVisible3d(true);
	}
	
	// normal workflow
	if (!currentTruck)
	{
		// get out
		if (previousTruck && person)
		{
			person->setPosition(previousTruck->getPosition());
			person->updateCharacterRotation();
		}

		// detach person from truck
		if (person)
		{
			person->setBeamCoupling(false);
		}

		//force feedback
		if (forcefeedback) forcefeedback->setEnabled(false);
		//LEDs
#ifdef USE_OIS_G27
		//logitech G27 LEDs tachometer
		if (leds)
		{
			leds->play(0, 10, 20);//stop the LEDs
		}
#endif //OIS_G27

		// hide truckhud
		if (ow) ow->truckhud->show(false);

		//getting outside
		Vector3 position = Vector3::ZERO;
		if (previousTruck)
		{
			previousTruck->prepareInside(false);

			if (previousTruck->dash)
				previousTruck->dash->setVisible(false);

			// this workaround enables trucks to spawn that have no cinecam. required for cmdline options
			if (previousTruck->cinecameranodepos[0] != -1 && previousTruck->cameranodepos[0] != -1 && previousTruck->cameranoderoll[0] != -1)
			{
				// truck has a cinecam
				position=previousTruck->nodes[previousTruck->cinecameranodepos[0]].AbsPosition;
				position+=-2.0*((previousTruck->nodes[previousTruck->cameranodepos[0]].RelPosition-previousTruck->nodes[previousTruck->cameranoderoll[0]].RelPosition).normalisedCopy());
				position+=Vector3(0.0, -1.0, 0.0);
			}
			else
			{
				// truck has no cinecam
				position=previousTruck->nodes[0].AbsPosition;
			}
		}
		//position.y=hfinder->getHeightAt(position.x,position.z);
		if (person && position != Vector3::ZERO)
		{
			person->setPosition(position);
			person->updateCharacterRotation();
			//person->setVisible(true);
		}
		if (ow) ow->showDashboardOverlays(false, currentTruck);
		if (ow) ow->showEditorOverlay(false);
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStop(previousTruck, SS_TRIG_AIR);
		SoundScriptManager::getSingleton().trigStop(previousTruck, SS_TRIG_PUMP);
#endif // OPENAL

		int free_truck = BeamFactory::getSingleton().getTruckCount();
		Beam **trucks =  BeamFactory::getSingleton().getTrucks();
		int t;
		for (t=0; t<free_truck; t++)
		{
			if (!trucks[t]) continue;
			trucks[t]->sleepcount=9;
		} //make trucks synchronous

		TRIGGER_EVENT(SE_TRUCK_EXIT, previousTruck?previousTruck->trucknum:-1);
	}
	else
	{
		//getting inside
		currentTruck->desactivate();


		//person->setVisible(false);
		if (ow &&!hidegui)
		{
			ow->showDashboardOverlays(true, currentTruck);
			ow->showEditorOverlay(currentTruck->editorId>=0);
		}

		currentTruck->activate();
		//if (trucks[current_truck]->engine->running) trucks[current_truck]->audio->playStart();
		//hide unused items
		if (ow && currentTruck->free_active_shock==0)
			(OverlayManager::getSingleton().getOverlayElement("tracks/rollcorneedle"))->hide();
		//					rollcorr_node->setVisible((trucks[current_truck]->free_active_shock>0));
		//help panel
		//force feedback
		if (forcefeedback) forcefeedback->setEnabled(currentTruck->driveable==TRUCK); //only for trucks so far
		//LEDs
#ifdef USE_OIS_G27
		//logitech G27 LEDs tachometer
		if (leds && currentTruck->driveable!=TRUCK)
		{
			leds->play(0, 10, 20);//stop the LEDs
		}
#endif //OIS_G27


		// attach person to truck
		if (person)
		{
			person->setBeamCoupling(true, currentTruck);
		}

		if (ow)
		{
			try
			{
				// we wont crash for help panels ...
				if (currentTruck->hashelp)
				{
					OverlayManager::getSingleton().getOverlayElement("tracks/helppanel")->setMaterialName(currentTruck->helpmat);
					OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName(currentTruck->helpmat);
				}
				else
				{
					OverlayManager::getSingleton().getOverlayElement("tracks/helppanel")->setMaterialName("tracks/black");
					OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName("tracks/black");
				}
			} catch(...)
			{
			}
			// enable gui mods
			if (currentTruck->speedomat != String(""))
				OverlayManager::getSingleton().getOverlayElement("tracks/speedo")->setMaterialName(currentTruck->speedomat);
			else
				OverlayManager::getSingleton().getOverlayElement("tracks/speedo")->setMaterialName("tracks/Speedo");

			if (currentTruck->tachomat != String(""))
				OverlayManager::getSingleton().getOverlayElement("tracks/tacho")->setMaterialName(currentTruck->tachomat);
			else
				OverlayManager::getSingleton().getOverlayElement("tracks/tacho")->setMaterialName("tracks/Tacho");
		}
		
		TRIGGER_EVENT(SE_TRUCK_ENTER, currentTruck?currentTruck->trucknum:-1);
	}
}

bool RoRFrameListener::updateAnimatedObjects(float dt)
{
	if (animatedObjects.size() == 0)
		return true;
	std::vector<animated_object_t>::iterator it;
	for(it=animatedObjects.begin(); it!=animatedObjects.end(); it++)
	{
		if (it->anim && it->speedfactor != 0)
		{
			Real time = dt * it->speedfactor;
			it->anim->addTime(time);
		}
	}
	return true;
}

bool RoRFrameListener::updateTruckMirrors(float dt)
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if (!curr_truck) return false;

	for(std::vector<VideoCamera *>::iterator it=curr_truck->vidcams.begin(); it!=curr_truck->vidcams.end(); it++)
	{
		(*it)->update(dt);
	}
	return true;
}

// Override frameStarted event to process that (don't care about frameEnded)
bool RoRFrameListener::frameStarted(const FrameEvent& evt)
{
	float dt=evt.timeSinceLastFrame;
	if (dt==0) return true;
	if (dt>1.0/20.0) dt=1.0/20.0;
	rtime+=dt; //real time
	if (mWindow->isClosed())
		return false;

	// update GUI
	INPUTENGINE.Capture();

	//if (collisions) 	printf("> ground model used: %s\n", collisions->last_used_ground_model->name);

	// exit frame started method when just displaying the GUI
#ifdef USE_MYGUI
	if (LoadingWindow::getSingleton().getFrameForced())
		return true;
#endif //MYGUI

	if (shutdownall)
		return false;

	if (shutdownall) // shortcut: press ESC in credits
	{
		parentState->exit();
		return false;
	}

	// update OutProtocol
	if (OutProtocol::getSingletonPtr())
		OutProtocol::getSingleton().update(dt);

	// the truck we use got deleted D:
	//if (current_truck != -1 && trucks[current_truck] == 0)
	//	BeamFactory::getSingleton().setCurrentTruck(-1);

	// update animated objects
	updateAnimatedObjects(dt);

	// update network gui if required, at most every 2 seconds
	if (net)
	{
		netcheckGUITimer += dt;
		if (netcheckGUITimer > 2)
		{
			checkRemoteStreamResultsChanged();
			netcheckGUITimer=0;
		}

#ifdef USE_SOCKETW
#ifdef USE_MYGUI
		// update net quality icon
		if (getNetQualityChanged())
		{
			GUI_Multiplayer::getSingleton().update();
		}
#endif // USE_MYGUI
#endif // USE_SOCKETW

		// now update mumble 3d audio things
#ifdef USE_MUMBLE
		MumbleIntegration::getSingleton().update(mCamera->getPosition(), person->getPosition() + Vector3(0, 1.8f, 0));
#endif // USE_MUMBLE
	}

	if (CameraManager::singletonExists() && loading_state == ALL_LOADED)
	{
		CameraManager::getSingleton().update(dt);
	}

#ifdef USE_OPENAL
	// update audio listener position
	static Vector3 lastCameraPosition;
	Vector3 cameraSpeed = (mCamera->getPosition() - lastCameraPosition) / dt;
	lastCameraPosition = mCamera->getPosition();

	SoundScriptManager::getSingleton().setCamera(mCamera->getPosition(), mCamera->getDirection(), mCamera->getUp(), cameraSpeed);
#endif // USE_OPENAL

	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

	// environment map
	if (envmap)
	{
		if (curr_truck)
		{
			envmap->update(curr_truck->getPosition(), curr_truck);
		} else
		{
			envmap->update(Vector3(terrainxsize / 2.0f, hfinder->getHeightAt(terrainxsize / 2.0f, terrainzsize / 2.0f ) + 50.0f, terrainzsize / 2.0f));
		}
	}

	// water
	if (w)
	{
		if (curr_truck)
			w->moveTo(mCamera, w->getHeightWaves(curr_truck->getPosition()));
		else
			w->moveTo(mCamera, w->getHeight());
	}

	// update mirrors after moving the camera as we use the camera position in mirrors
	updateTruckMirrors(dt);

	// update water after the camera!
	if (loading_state==ALL_LOADED && w) w->framestep(dt);

	//update visual - antishaking
	if (loading_state==ALL_LOADED)
	{
		BeamFactory::getSingleton().updateVisual(dt);

		// add some example AI
		if (loadedTerrain == "simple.terrn")
			BeamFactory::getSingleton().updateAI(dt);
	}

	if (!updateEvents(dt))
	{
		LOG("exiting...");
		return false;
	}

	// update gui 3d arrow
	if (ow && dirvisible && loading_state==ALL_LOADED)
	{
		dirArrowNode->lookAt(dirArrowPointed, Node::TS_WORLD,Vector3::UNIT_Y);
		char tmp[256];
		Real distance = 0;
		if (curr_truck && curr_truck->state == ACTIVATED)
			distance = curr_truck->getPosition().distance(dirArrowPointed);
		else
			distance = person->getPosition().distance(dirArrowPointed);
		sprintf(tmp,"%0.1f meter", distance);
		ow->directionArrowDistance->setCaption(tmp);
	}

	// one of the input modes is immediate, so setup what is needed for immediate mouse/key movement
	if (mTimeUntilNextToggle >= 0)
		mTimeUntilNextToggle -= dt;

	// one of the input modes is immediate, so update the movement vector
	if (loading_state==ALL_LOADED)
	{
#ifdef USE_PAGED
		// paged geometry
		for(std::vector<paged_geometry_t>::iterator it=pagedGeometry.begin();it!=pagedGeometry.end();it++)
		{
			if (it->geom) it->geom->update();
		}
#endif //USE_PAGED
		
		BeamFactory::getSingleton().checkSleepingState();

		//we simulate one truck, it will take care of the others (except networked ones)

		BeamFactory::getSingleton().calcPhysics(dt);

		updateIO(dt);

		updateGUI(dt);
	}


	// TODO: check if all wheels are on a certain event id
	// wheels[nodes[i].wheelid].lastEventHandler

#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().framestep(dt);
#endif

	// update network labels
	if (net)
	{
		CharacterFactory::getSingleton().updateLabels();
	}

	return true;
}

bool RoRFrameListener::frameEnded(const FrameEvent& evt)
{
	// TODO: IMPROVE STATS
	if (ow && mStatsOn) ow->updateStats();

	//		moveCamera();

	// workaround to be able to show a single waiting sign before working on the files
	//if (uiloader && uiloader->hasWork())
	//	uiloader->dowork();

	if (heathaze)
	{
		heathaze->update();
	}

#ifdef USE_SOCKETW
	if (net)
	{
		// process all packets and streams received
		NetworkStreamManager::getSingleton().update();
	}
#endif //SOCKETW

	return true;
}

void RoRFrameListener::showLoad(int type, char* instance, char* box)
{
	int free_truck    = BeamFactory::getSingleton().getTruckCount();
	Beam **trucks     = BeamFactory::getSingleton().getTrucks();

	// first, test if the place if clear, BUT NOT IN MULTIPLAYER
	if (!net)
	{
		collision_box_t *spawnbox = collisions->getBox(instance, box);
		for (int t=0; t < free_truck; t++)
		{
			if (!trucks[t]) continue;
			for (int i=0; i < trucks[t]->free_node; i++)
			{
				if (collisions->isInside(trucks[t]->nodes[i].AbsPosition, spawnbox))
				{
#ifdef USE_MYGUI
					Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Please clear the place first"), "error.png");
#endif // USE_MYGUI
					collisions->clearEventCache();
					return;
				}
			}
		}
	}
	reload_pos = collisions->getPosition(instance, box);
	reload_dir = collisions->getDirection(instance, box);
	reload_box = collisions->getBox(instance, box);
	loading_state = RELOADING;
	hideMap();
#ifdef USE_MYGUI
	SelectorWindow::getSingleton().show(SelectorWindow::LoaderType(type));
#endif //USE_MYGUI
}

void RoRFrameListener::setDirectionArrow(char *text, Vector3 position)
{
	if (!ow) return;
	if (!text)
	{
		dirArrowNode->setVisible(false);
		dirvisible = false;
		dirArrowPointed = Vector3::ZERO;
		ow->directionOverlay->hide();
	}
	else
	{
		ow->directionOverlay->show();
		ow->directionArrowText->setCaption(String(text));
		//LOG("*** new pointed position: " + TOSTRING(position));
		ow->directionArrowDistance->setCaption("");
		dirvisible = true;
		dirArrowPointed = position;
		dirArrowNode->setVisible(true);
	}

}

void RoRFrameListener::netDisconnectTruck(int number)
{
	// we will remove the truck completely
	// TODO: fix that below!
	//removeTruck(number);
#ifdef USE_MYGUI
	if (surveyMap)
	{
		MapEntity *e = surveyMap->getEntityByName("Truck"+TOSTRING(number));
		if (e)
			e->setVisibility(false);
	}
#endif //USE_MYGUI
}


/* --- Window Events ------------------------------------------ */
void RoRFrameListener::windowResized(RenderWindow* rw)
{
	if (!rw)
		return;
	LOG("*** windowResized");

	// Update mouse screen width/height
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);
	screenWidth = width;
	screenHeight = height;

	if (ow) ow->windowResized(rw);
	if (surveyMap) surveyMap->windowResized(rw);

	//update mouse area
	INPUTENGINE.windowResized(rw);
}

//Unattach OIS before window shutdown (very important under Linux)
void RoRFrameListener::windowClosed(RenderWindow* rw)
{
	LOG("*** windowClosed");
}

void RoRFrameListener::windowMoved(RenderWindow* rw)
{
	LOG("*** windowMoved");
}

void RoRFrameListener::windowFocusChange(RenderWindow* rw)
{
	LOG("*** windowFocusChange");
	INPUTENGINE.resetKeys();
}

void RoRFrameListener::pauseSim(bool value)
{
	// TODO: implement this (how to do so?)
	static int savedmode = -1;
	if (value && loading_state == EDITOR_PAUSE)
		// already paused
		return;
	if (value)
	{
		savedmode = loading_state;
		loading_state = EDITOR_PAUSE;
		LOG("** pausing game");
	} else if (!value && savedmode != -1)
	{
		loading_state = savedmode;
		LOG("** unpausing game");
	}
}

void RoRFrameListener::initHDR()
{
	Viewport *vp = mCamera->getViewport();
	Ogre::CompositorInstance *instance = Ogre::CompositorManager::getSingleton().addCompositor(vp, "HDR", 0);
	Ogre::CompositorManager::getSingleton().setCompositorEnabled(vp, "HDR", true);

	// HDR needs a special listener
	hdrListener = new HDRListener();
	instance->addListener(hdrListener);
	hdrListener->notifyViewportSize(vp->getActualWidth(), vp->getActualHeight());
	hdrListener->notifyCompositor(instance);
}

void RoRFrameListener::hideGUI(bool visible)
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	
#ifdef USE_MYGUI
	Console *c = Console::getSingletonPtrNoCreation();
	if (c) c->setVisible(!visible);
#endif // USE_MYGUI

	if (visible)
	{
		if (ow) ow->showDashboardOverlays(false, curr_truck);
		if (ow) ow->showEditorOverlay(false);
		if (ow) ow->truckhud->show(false);
		//if (bigMap) bigMap->setVisibility(false);
#ifdef USE_MYGUI
#ifdef USE_SOCKETW
		if (net) GUI_Multiplayer::getSingleton().setVisible(false);
#endif // USE_SOCKETW
#endif // USE_MYGUI
	}
	else
	{
		if (curr_truck
			&& CameraManager::singletonExists()
			&& CameraManager::getSingleton().hasActiveBehavior()
			&& CameraManager::getSingleton().getCameraBehavior() != CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
		{
			if (ow) ow->showDashboardOverlays(true, curr_truck);
			//if (bigMap) bigMap->setVisibility(true);
		}
#ifdef USE_SOCKETW
#ifdef USE_MYGUI
		if (net) GUI_Multiplayer::getSingleton().setVisible(true);
#endif // USE_MYGUI
#endif // USE_SOCKETW
	}
}

// show/hide all particle systems
void RoRFrameListener::showspray(bool s)
{
	DustManager::getSingleton().setVisible(s);
}

void RoRFrameListener::setLoadingState(int value)
{
	loading_state = value;
}

void RoRFrameListener::setNetPointToUID(int uid)
{
	// TODO: setup arrow
	netPointToUID = uid;
}


void RoRFrameListener::checkRemoteStreamResultsChanged()
{
#ifdef USE_MYGUI
#ifdef USE_SOCKETW
	if (BeamFactory::getSingleton().checkStreamsResultsChanged())
		GUI_Multiplayer::getSingleton().update();
#endif // USE_SOCKETW
#endif // USE_MYGUI
}


void RoRFrameListener::setNetQuality(int q)
{
	MUTEX_LOCK(&mutex_data);
	net_quality = q;
	net_quality_changed = true;
	MUTEX_UNLOCK(&mutex_data);
}

int RoRFrameListener::getNetQuality(bool ack)
{
	int res = 0;
	MUTEX_LOCK(&mutex_data);
	res = net_quality;
	if (ack) net_quality_changed=false;
	MUTEX_UNLOCK(&mutex_data);
	return res;
}

bool RoRFrameListener::getNetQualityChanged()
{
	bool res = false;
	MUTEX_LOCK(&mutex_data);
	res = net_quality_changed;
	MUTEX_UNLOCK(&mutex_data);
	return res;
}

bool RoRFrameListener::RTSSgenerateShadersForMaterial(Ogre::String curMaterialName, Ogre::String normalTextureName)
{
	if (!BSETTING("Use RTShader System", false)) return false;
	bool success;

	// Create the shader based technique of this material.
	success = Ogre::RTShader::ShaderGenerator::getSingleton().createShaderBasedTechnique(curMaterialName,
			 			MaterialManager::DEFAULT_SCHEME_NAME,
			 			RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
	if (!success)
		return false;

	// Setup custom shader sub render states according to current setup.
	MaterialPtr curMaterial = MaterialManager::getSingleton().getByName(curMaterialName);


	// Grab the first pass render state.
	// NOTE: For more complicated samples iterate over the passes and build each one of them as desired.
	RTShader::RenderState* renderState = Ogre::RTShader::ShaderGenerator::getSingleton().getRenderState(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, curMaterialName, 0);

	// Remove all sub render states.
	renderState->reset();


#ifdef RTSHADER_SYSTEM_BUILD_CORE_SHADERS
	// simple vertex lightning
	/*
	{
		RTShader::SubRenderState* perPerVertexLightModel = Ogre::RTShader::ShaderGenerator::getSingleton().createSubRenderState(RTShader::FFPLighting::Type);
		renderState->addTemplateSubRenderState(perPerVertexLightModel);
	}
	*/
#endif
#ifdef RTSHADER_SYSTEM_BUILD_EXT_SHADERS
	if (normalTextureName.empty())
	{
		// SSLM_PerPixelLighting
		RTShader::SubRenderState* perPixelLightModel = Ogre::RTShader::ShaderGenerator::getSingleton().createSubRenderState(RTShader::PerPixelLighting::Type);

		renderState->addTemplateSubRenderState(perPixelLightModel);
	} else
	{
		// SSLM_NormalMapLightingTangentSpace
		RTShader::SubRenderState* subRenderState = Ogre::RTShader::ShaderGenerator::getSingleton().createSubRenderState(RTShader::NormalMapLighting::Type);
		RTShader::NormalMapLighting* normalMapSubRS = static_cast<RTShader::NormalMapLighting*>(subRenderState);

		normalMapSubRS->setNormalMapSpace(RTShader::NormalMapLighting::NMS_TANGENT);
		normalMapSubRS->setNormalMapTextureName(normalTextureName);

		renderState->addTemplateSubRenderState(normalMapSubRS);
	}
	// SSLM_NormalMapLightingObjectSpace
	/*
	{
		// Apply normal map only on main entity.
		if (entity->getName() == MAIN_ENTITY_NAME)
		{
			RTShader::SubRenderState* subRenderState = mShaderGenerator->createSubRenderState(RTShader::NormalMapLighting::Type);
			RTShader::NormalMapLighting* normalMapSubRS = static_cast<RTShader::NormalMapLighting*>(subRenderState);

			normalMapSubRS->setNormalMapSpace(RTShader::NormalMapLighting::NMS_OBJECT);
			normalMapSubRS->setNormalMapTextureName("Panels_Normal_Obj.png");

			renderState->addTemplateSubRenderState(normalMapSubRS);
		}

		// It is secondary entity -> use simple per pixel lighting.
		else
		{
			RTShader::SubRenderState* perPixelLightModel = mShaderGenerator->createSubRenderState(RTShader::PerPixelLighting::Type);
			renderState->addTemplateSubRenderState(perPixelLightModel);
		}
	} */

#endif

	// Invalidate this material in order to re-generate its shaders.
	Ogre::RTShader::ShaderGenerator::getSingleton().invalidateMaterial(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, curMaterialName);
	return true;
}

void RoRFrameListener::RTSSgenerateShaders(Entity* entity, Ogre::String normalTextureName)
{
	if (!BSETTING("Use RTShader System", false)) return;
	for (unsigned int i=0; i < entity->getNumSubEntities(); ++i)
	{
		SubEntity* curSubEntity = entity->getSubEntity(i);
		const String& curMaterialName = curSubEntity->getMaterialName();

		RTSSgenerateShadersForMaterial(curMaterialName, normalTextureName);
	}
}

void RoRFrameListener::reloadCurrentTruck()
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if (!curr_truck) return;

	// try to load the same truck again
	Beam *newBeam = BeamFactory::getSingleton().createLocal(reload_pos, reload_dir, curr_truck->realtruckfilename);

	if (!newBeam)
	{
#ifdef USE_MYGUI
		Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("unable to load new truck: limit reached. Please restart RoR"), "error.png");
#endif // USE_MYGUI
		return;
	}

	// remove the old truck
	curr_truck->state = RECYCLE;

	// enter the new truck
	BeamFactory::getSingleton().setCurrentTruck(newBeam->trucknum);

	// copy over the most basic info
	if (curr_truck->free_node == newBeam->free_node)
	{
		for(int i=0;i<curr_truck->free_node;i++)
		{
			// copy over nodes attributes if the amount of them didnt change
			newBeam->nodes[i].AbsPosition = curr_truck->nodes[i].AbsPosition;
			newBeam->nodes[i].RelPosition = curr_truck->nodes[i].RelPosition;
			newBeam->nodes[i].Velocity    = curr_truck->nodes[i].Velocity;
			newBeam->nodes[i].Forces      = curr_truck->nodes[i].Forces;
			newBeam->nodes[i].iPosition   = curr_truck->nodes[i].iPosition;
			newBeam->nodes[i].smoothpos   = curr_truck->nodes[i].smoothpos;
		}
	}

	// TODO:
	// * copy over the engine infomation
	// * commands status
	// * other minor stati

	// notice the user about the amount of possible reloads
	String msg = TOSTRING(newBeam->trucknum) + String(" of ") + TOSTRING(MAX_TRUCKS) + String(" possible reloads.");
#ifdef USE_MYGUI
	Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, msg, "information.png");
#endif //USE_MYGUI

	// dislocate the old truck, so its out of sight
	curr_truck->resetPosition(100000, 100000, false, 100000);
	// note: in some point in the future we would delete the truck here,
	// but since this function is buggy we don't do it yet.
}

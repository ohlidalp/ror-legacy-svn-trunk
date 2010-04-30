/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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


#include <float.h>
#include "Beam.h"
#include "rormemory.h"

#include "engine.h"
#include "SoundScriptManager.h"
#include "heightfinder.h"
#include "DustManager.h"
#include "water.h"
#include "DustPool.h"
#include "errorutils.h"
#include "Replay.h"
#include "mirrors.h"
#include "autopilot.h"
#include "ScopeLog.h"
#include "network.h"
#include "NetworkStreamManager.h"
#include "skinmanager.h"
#include "FlexMesh.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "FlexAirfoil.h"
#include "MovableText.h"
#include "turboprop.h"
#include "turbojet.h"
#include "screwprop.h"
#include "buoyance.h"
#include "collisions.h"
#include "airbrake.h"
#include "FlexBody.h"
#include "materialFunctionMapper.h"
#include "TorqueCurve.h"
#include "Settings.h"
#include "network.h"
#include "PointColDetector.h"
#ifdef FEAT_TIMING
#include "BeamStats.h"
#endif
#include "Skidmark.h"
#include "CmdKeyInertia.h"
#include "ColoredTextAreaOverlayElement.h"
#ifdef USE_ANGELSCRIPT
#include "ScriptEngine.h"
#endif

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

#ifdef USE_CRASHRPT
# include "crashrpt.h"
#endif

float mrtime;

//threads and mutexes, see also at the bottom
int thread_mode=0;

void *threadstart(void* vid);


class Beam;
Beam* threadbeam[MAX_TRUCKS];
int free_tb=0;

Beam::~Beam()
{
	// TODO: IMPROVE below: delete/destroy prop entities, etc

	deleting = true;

	// Very Important: remove this truck out of the trucks array, otherwise segfault
	if (eflsingleton) eflsingleton->removeBeam(this);


	// hide all meshes, prevents deleting stuff while drawing
	this->setMeshVisibility(false);

	//block until all threads done
	if (thread_mode==THREAD_HT)
	{
		pthread_mutex_lock(&done_count_mutex);
		while (done_count>0)
			pthread_cond_wait(&done_count_cv, &done_count_mutex);
		pthread_mutex_unlock(&done_count_mutex);
	}

	// delete all classes we might have constructed

	// destruct and remove every tiny bit of stuff we created :-|
	if(nettimer) delete nettimer; nettimer=0;
	if(engine) delete engine; engine=0;
	if(buoyance) delete buoyance; buoyance=0;
	if(autopilot) delete autopilot;
	if(fuseAirfoil) delete fuseAirfoil;
	if(cabMesh) delete cabMesh;
	if(materialFunctionMapper) delete materialFunctionMapper;
	if(replay) delete replay;

	std::vector<Ogre::SceneNode*> deletion_sceneNodes;
	std::vector<Ogre::Entity *> deletion_Entities;

	// remove all scene nodes
	if(deletion_sceneNodes.size() > 0)
	{
		int size = deletion_sceneNodes.size();
		for(int i=0;i<size; i++)
		{
			if(!deletion_sceneNodes[i]) continue;
			deletion_sceneNodes[i]->removeAndDestroyAllChildren();
			tsm->destroySceneNode(deletion_sceneNodes[i]);

			deletion_sceneNodes[i]=0;
		}
	}
	// delete skidmarks as well?!

	// delete wings
	for(int i=0; i<free_wing;i++)
	{
		// flexAirfoil, airfoil
		if(wings[i].fa) delete wings[i].fa; wings[i].fa=0;
		if(wings[i].cnode)
		{
			wings[i].cnode->removeAndDestroyAllChildren();
			tsm->destroySceneNode(wings[i].cnode);
		}
	}

	// delete aeroengines
	for(int i=0; i<free_aeroengine;i++)
	{
		if(aeroengines[i]) delete aeroengines[i];
	}

	// delete screwprops
	for(int i=0; i<free_screwprop;i++)
	{
		if(screwprops[i]) delete screwprops[i];
	}

	// delete airbrakes
	for(int i=0; i<free_airbrake;i++)
	{
		if(airbrakes[i]) delete airbrakes[i];
	}

	// delete flexbodies
	for(int i=0; i<free_flexbody;i++)
	{
		if(flexbodies[i]) delete flexbodies[i];
	}


	// delete meshwheels
	for(int i=0; i<free_wheel;i++)
	{
		if(vwheels[i].fm) delete vwheels[i].fm;
		if(vwheels[i].cnode)
		{
			vwheels[i].cnode->removeAndDestroyAllChildren();
			tsm->destroySceneNode(vwheels[i].cnode);
		}
	}

	// delete props
	for(int i=0; i<free_prop;i++)
	{
		if(props[i].bbsnode[0])
		{
			props[i].bbsnode[0]->removeAndDestroyAllChildren();
			tsm->destroySceneNode(props[i].bbsnode[0]);
		}
		if(props[i].bbsnode[1])
		{
			props[i].bbsnode[1]->removeAndDestroyAllChildren();
			tsm->destroySceneNode(props[i].bbsnode[1]);
		}
		if(props[i].bbsnode[2])
		{
			props[i].bbsnode[2]->removeAndDestroyAllChildren();
			tsm->destroySceneNode(props[i].bbsnode[2]);
		}
		if(props[i].bbsnode[3])
		{
			props[i].bbsnode[3]->removeAndDestroyAllChildren();
			tsm->destroySceneNode(props[i].bbsnode[3]);
		}
		if(props[i].snode)
		{
			props[i].snode->removeAndDestroyAllChildren();
			tsm->destroySceneNode(props[i].snode);
		}
		if(props[i].wheel)
		{
			props[i].wheel->removeAndDestroyAllChildren();
			tsm->destroySceneNode(props[i].wheel);
		}
		if(props[i].light[0]) tsm->destroyLight(props[i].light[0]);
		if(props[i].light[1]) tsm->destroyLight(props[i].light[1]);
		if(props[i].light[2]) tsm->destroyLight(props[i].light[2]);
		if(props[i].light[3]) tsm->destroyLight(props[i].light[3]);
	}

	// delete flares
	for (int i=0; i<free_flare; i++)
	{
		if(flares[i].snode)
		{
			flares[i].snode->removeAndDestroyAllChildren();
			tsm->destroySceneNode(flares[i].snode);
		}
		if(flares[i].light) tsm->destroyLight(flares[i].light);

	}

	// delete exhausts
	for(std::vector < exhaust_t >::iterator it=exhausts.begin(); it!=exhausts.end(); it++)
	{
		if(it->smokeNode)
		{
			it->smokeNode->removeAndDestroyAllChildren();
			tsm->destroySceneNode(it->smokeNode);
		}
		if(it->smoker)
		{
			it->smoker->removeAllAffectors();
			it->smoker->removeAllEmitters();
			tsm->destroyParticleSystem(it->smoker);
		}
	}

	// delete cparticles
	for (int i=0; i<free_cparticle; i++)
	{
		if(cparticles[free_cparticle].snode)
		{
			cparticles[free_cparticle].snode->removeAndDestroyAllChildren();
			tsm->destroySceneNode(cparticles[free_cparticle].snode);
		}
		if(cparticles[free_cparticle].psys)
		{
			cparticles[free_cparticle].psys->removeAllAffectors();
			cparticles[free_cparticle].psys->removeAllEmitters();
			tsm->destroyParticleSystem(cparticles[free_cparticle].psys);
		}

	}

	// delete beams
	for (int i=0; i<free_beam; i++)
	{
		if(beams[i].mEntity)    beams[i].mEntity->setVisible(false);
		if(beams[i].mSceneNode)
		{
			beams[i].mSceneNode->removeAndDestroyAllChildren();
			//tsm->destroySceneNode(beams[i].mSceneNode);
		}
	}

	// delete Rails
	for(std::vector< RailGroup* >::iterator it = mRailGroups.begin(); it != mRailGroups.end(); it++)
	{
		// signal to the Rail that
		(*it)->cleanUp();
		delete (*it);
	}

	if(netMT)
	{
		netMT->setVisible(false);
		delete netMT;
		netMT = 0;
	}

}

Beam::Beam(int tnum, SceneManager *manager, SceneNode *parent, RenderWindow* win, Network *_net, float *_mapsizex, float *_mapsizez, Real px, Real py, Real pz, Quaternion rot, const char* fname, Collisions *icollisions, HeightFinder *mfinder, Water *w, Camera *pcam, Mirrors *mmirror, bool networked, bool networking, collision_box_t *spawnbox, bool ismachine, int _flaresMode, std::vector<Ogre::String> *_truckconfig, SkinPtr skin, bool freeposition) : \
	deleting(false)
{
	net=_net;
	if(net && !networking) networking = true; // enable networking if some network class is existing

	tsteps=100;
	driverSeat=0;
	networkUsername = String();
	networkAuthlevel = 0;
	beambreakdebug = (SETTINGS.getSetting("Beam Break Debug") == "Yes");
	freePositioned = freeposition;
	free_axle=0;
	slideNodesConnectInstantly=false;
	replayTimer=0;
	minCameraRadius=0;
	last_net_time=0;
	lastFuzzyGroundModel=0;
	patchEngineTorque=false;
	usedSkin = skin;
	LogManager::getSingleton().logMessage("BEAM: loading new truck: " + String(fname));
	trucknum=tnum;
	currentScale=1;
	// copy truck config
	if(_truckconfig && _truckconfig->size())
		for(std::vector<String>::iterator it = _truckconfig->begin(); it!=_truckconfig->end();it++)
			truckconfig.push_back(*it);
#ifdef USE_OPENAL
	ssm=SoundScriptManager::getSingleton();
#endif //OPENAL
	materialFunctionMapper = new MaterialFunctionMapper();
	cmdInertia   = new CmdKeyInertia(MAX_COMMANDS);
	hydroInertia = new CmdKeyInertia(MAX_HYDROS);
	rotaInertia  = new CmdKeyInertia(MAX_ROTATORS);
	free_soundsource=0;
	nodedebugstate=-1;
	debugVisuals=0;
	netMT = 0;
	beam_creak=BEAM_CREAK_DEFAULT;
	dynamicMapMode=0;
	meshesVisible=true;
	disable_default_sounds=false;
#ifdef FEAT_TIMING
	statistics = BES.getClient(tnum);
#endif
	//truckScript = new TruckCommandScheduler();
	flaresMode = _flaresMode;
	int i;
	airbrakeval=0;
	origin=Vector3::ZERO;
	cameranodeacc=Vector3::ZERO;
	cameranodecount=0;
	label=tnum; //convenient, but set overwise elsewhere for a good cause
	this->networking=networking;
	mapsizex = _mapsizex;
	mapsizez = _mapsizez;
	floating_origin_enable=true;
	lockSkeletonchange=false;
	reset_requested=0;
	mrtime=0.0;
	free_flexbody=0;
	netLabelNode=0;
	free_rigidifier=0;
	autopilot=0;
	free_rotator=0;
	free_cparticle=0;
	free_airbrake=0;
	cparticle_mode=false;
	cparticle_enabled=false;
	mousenode=-1;
	mousemoveforce=0.0f;
	mousepos=Vector3::ZERO;
	ispolice=false;
	cablight=0;
	cablightNode=0;
	brakeforce=30000.0;
	hbrakeforce = 2*brakeforce;
	hasposlights=false;
	disableDrag=false;
	advanced_drag=false;
	fuseAirfoil=0;
	fadeDist=150.0;
	fusedrag=Vector3::ZERO;
	elevator=0;
	minimass=50.0;
	rudder=0;
	aileron=0;
	flap=0;
	free_aeroengine=0;
	free_screwprop=0;
	free_wing=0;
	refpressure=50.0;
	free_pressure_beam=0;
	default_break=BEAM_BREAK;
	default_deform=BEAM_DEFORM;
	default_beam_diameter=DEFAULT_BEAM_DIAMETER;
	skeleton_beam_diameter=BEAM_SKELETON_DIAMETER;
	default_spring=DEFAULT_SPRING;
	default_damp=DEFAULT_DAMP;
	default_node_friction=NODE_FRICTION_COEF_DEFAULT;
	default_node_volume=NODE_VOLUME_COEF_DEFAULT;
	default_node_surface=NODE_SURFACE_COEF_DEFAULT;
	default_node_loadweight=NODE_LOADWEIGHT_DEFAULT;
	default_plastic_coef=0.0f;
	strcpy(default_beam_material, "tracks/beam");
	driversseatfound=false;
	mirror=mmirror;
	leftMirrorAngle=0.52;
	rightMirrorAngle=-0.52;
	if(ismachine)
		driveable=MACHINE;
	else
		driveable=NOT_DRIVEABLE;
	previousGear = 0;
	previousCrank = 0.0f;
	animTimer = 0.0f;
	engine=0;
	truckversion=-1;
	editorId=-1;
	hasEmissivePass=0;
	isInside=false;
	beacon=false;
	realtruckfilename = String(fname);
	sprintf(truckname, "t%i", tnum);
	simpleSkeletonManualObject=0;
	simpleSkeletonInitiated=false;
	loading_finished=0;
	strcpy(uniquetruckid,"-1");
	categoryid=-1;
	truckversion=-1;
	tsm=manager;
	simpleSkeletonNode = tsm->getRootSceneNode()->createChildSceneNode(String(truckname)+"_simpleskeleton_node");
	deletion_sceneNodes.push_back(simpleSkeletonNode);

	tdt=0.1;
	ttdt=0.1;
	//load terrain altitudes
	//			hfinder=new HeightFinder(terrainmap, tsm);
	mCamera=pcam;
	hfinder=mfinder;
	mWindow=win;
	beamsRoot=parent->createChildSceneNode();
	deletion_sceneNodes.push_back(netLabelNode);

	parentNode=parent;
	//			float gears[]={6.0, 3.0, 1.5, 1.0, 0.75};
	//	engine=new BeamEngine(1000.0,2000.0, 1870.0, 4.0, 5, gears, 3.42, audio);
	//engine=new BeamEngine(1000.0,2000.0, 4000.0, 4.0, 5, gears, 3.42, audio);

	//init
	water=w;
	detailLevel=0;
	//			state=DESACTIVATED;
	//			sleepcount=9;
	state=SLEEPING;
	if (networked) state=NETWORKED; //required for proper loading
	sleepcount=0;
	freecinecamera=0;
	currentcamera=0;
	cinecameranodepos[0]=-1;
	cameranodepos[0]=-1;
	cameranodedir[0]=-1;
	cameranoderoll[0]=-1;
	rescuer=false;
	freecamera=0;
	requires_wheel_contact=false;
	wheel_contact_requested=false;
	subisback[0]=0;
	canwork=1;
	hashelp=0;
	totalmass=0;
	parkingbrake=0;
	lights=1;
	free_node=0;
	free_beam=0;
	free_contacter=0;
	free_hydro=0;
	free_wheel=0;
	free_sub=0;
	free_texcoord=0;
	free_cab=0;
	free_collcab=0;
	collrange=0.02f;
	free_buoycab=0;
	buoyance=0;
	free_shock=0;
	free_flare=0;
	free_prop=0;
	forwardcommands=0;
	importcommands=0;
	masscount=0;
	lastwspeed=0.0;
	stabcommand=0;
	stabratio=0.0;
	stabsleep=0.0;
	cabMesh=NULL;
	smokeNode=NULL;
	smokeId=0;
	smokeRef=0;
	smoker=NULL;
	brake=0.0;
	abs_timer=0.0;
	abs_state=false;
	blinktreshpassed=false;
	blinkingtype=BLINK_NONE;
	netCustomLightArray[0] = -1;
	netCustomLightArray[1] = -1;
	netCustomLightArray[2] = -1;
	netCustomLightArray[3] = -1;
	netCustomLightArray_counter = 0;
	mTimeUntilNextToggle = 0;
	netBrakeLight = false;
	netReverseLight = false;
	tachomat="";
	speedomat="";
	speedoMax=140;
	useMaxRPMforGUI=false;
	skeleton=0;
	proped_wheels=0;
	braked_wheels=0;
	fasted=1;
	slowed=1;
	hydrodircommand=0;
	hydrodirstate=0;
	hydrodirwheeldisplay=0.0;
	hydroaileroncommand=0;
	hydroaileronstate=0;
	hydroruddercommand=0;
	hydrorudderstate=0;
	hydroelevatorcommand=0;
	hydroelevatorstate=0;
	replaymode=false;
	replaypos=0;
	replayPrecision=0;
	oldreplaypos=-1;
	watercontact=0;
	watercontactold=0;
	//			lastdt=0.1;
	//for (i=0; i<MAX_COMMANDS; i++) {commandkey[i].bfree=0;commandkey[i].rotfree=0;commandkey[i].kpressed=0;};
	hascommands=0;
	ipy=py;
	position=Vector3(px,py,pz);
	lastposition=Vector3(px,py,pz);
	lastlastposition=Vector3(px,py,pz);
//	aposition=Vector3(px,py,pz);

	cabFadeMode = 0;
	cabFadeTimer=0;
	cabFadeTime=0.3;

	// skidmark stuff
	useSkidmarks = (SETTINGS.getSetting("Skidmarks") == "Yes");

	// always init skidmarks with 0
	for(int i=0; i<MAX_WHEELS*2; i++) skidtrails[i] = 0;

	collisions=icollisions;
	pointCD = new PointColDetector();

	disable_smoke=(SETTINGS.getSetting("Engine smoke")=="No");
	heathaze=(SETTINGS.getSetting("HeatHaze")=="Yes");
	if(heathaze && disable_smoke)
		//no heathaze without smoke!
		heathaze=false;

	debugVisuals=(SETTINGS.getSetting("DebugBeams")=="Yes");

	enable_wheel2=(SETTINGS.getSetting("Enhanced wheels")=="Yes");
	if (networked || networking) enable_wheel2=false;

	cparticle_enabled=(SETTINGS.getSetting("Custom Particles")=="Yes");
	if(strnlen(fname,200) > 0)
		if(loadTruck(fname, manager, parent, px, py, pz, rot, spawnbox))
			return;

	//            printf("%i nodes, %i beams\n", free_node, free_beam);

	// setup replay mode
	bool enablereplay = (SETTINGS.getSetting("Replay mode")=="Yes");
	replay=0;
	replaylen = 10000;
	if(enablereplay && state != NETWORKED && !networking)
	{
		String rpl = SETTINGS.getSetting("Replay length");
		if(!rpl.empty())
			replaylen = StringConverter::parseInt(rpl);
		replay = new Replay(this, replaylen);

		rpl = SETTINGS.getSetting("Replay Steps per second");
		int steps = 0;
		if(!rpl.empty())
			steps = StringConverter::parseInt(rpl);
		if(steps <= 0)
			replayPrecision = 0.0f;
		else
			replayPrecision = 1.0f / ((float)steps);
	}

	// add storage
	posStorage=0;
	bool enablePosStor = (SETTINGS.getSetting("Position Storage")=="Yes");
	if(enablePosStor)
	{
		posStorage = new PositionStorage(free_node, 10);
	}

	//search first_wheel_node
	first_wheel_node=free_node;
	for (i=0; i<free_node; i++)
	{
		if (nodes[i].iswheel)
		{
			first_wheel_node=i;
			break;
		}
	}
	nodebuffersize=4*3+(first_wheel_node-1)*2*3;
	netbuffersize=nodebuffersize+free_wheel*4;
	updateVisual();
	//stop lights
	lightsToggle(0, 0);

	updateFlares(0);
	updateProps();
	if (engine) engine->offstart();
	//pressurize tires
	addPressure(0.0);
	//thread start
	//get parameters
	if (SETTINGS.getSetting("Threads")=="1 (Standard CPU)")thread_mode=THREAD_MONO;
	if (SETTINGS.getSetting("Threads")=="2 (Hyper-Threading or Dual core CPU)") thread_mode=THREAD_HT;
	if (SETTINGS.getSetting("Threads")=="3 (multi core CPU, one thread per beam)") thread_mode=THREAD_HT2;

	checkBeamMaterial();

	//init mutexes
	pthread_mutex_init(&work_mutex, NULL);
	pthread_cond_init(&work_cv, NULL);

	if (thread_mode == THREAD_HT)
		done_count=thread_mode;//for ready test
	else if (thread_mode == THREAD_HT2)
		done_count=1;//for ready test

	pthread_mutex_init(&done_count_mutex, NULL);
	pthread_cond_init(&done_count_cv, NULL);

	threadbeam[free_tb]=this;
	free_tb++;

	//starting threads
	if (thread_mode == THREAD_HT)
	{
		for (i=0; i<thread_mode; i++)
		{
			int rc;
			rc=pthread_create(&threads[i], NULL, threadstart, (void*)(free_tb-1));
			if (rc) LogManager::getSingleton().logMessage("BEAM: Can not start a thread");
		}

		//we must wait the threads to be ready
		pthread_mutex_lock(&done_count_mutex);
		while (done_count>0)
			pthread_cond_wait(&done_count_cv, &done_count_mutex);
		pthread_mutex_unlock(&done_count_mutex);
	} else if (thread_mode == THREAD_HT2)
	{
		// just create ONE thread for this beam
		int rc;
		rc=pthread_create(&threads[i], NULL, threadstart, (void*)(free_tb-1));
		if (rc) LogManager::getSingleton().logMessage("BEAM: Can not start a thread");

		//we must wait the threads to be ready
		pthread_mutex_lock(&done_count_mutex);
		while (done_count>0)
			pthread_cond_wait(&done_count_cv, &done_count_mutex);
		pthread_mutex_unlock(&done_count_mutex);
	}
	//all finished? so start network stuff
	if (networked)
	{
		state=NETWORKED;
		//alloc memory
		oob1=(oob_t*)ror_malloc(sizeof(oob_t));
		oob2=(oob_t*)ror_malloc(sizeof(oob_t));
		oob3=(oob_t*)ror_malloc(sizeof(oob_t));
		netb1=(char*)ror_malloc(netbuffersize);
		netb2=(char*)ror_malloc(netbuffersize);
		netb3=(char*)ror_malloc(netbuffersize);
		nettimer = new Timer();
		net_toffset=0;
		netcounter=0;
		//init mutex
		pthread_mutex_init(&net_mutex, NULL);
		if (engine) engine->start();
	}

	if(networking)
	{
		sendStreamSetup();
	}

	//updateDebugOverlay();
}

// This method scales trucks. Stresses should *NOT* be scaled, they describe
// the material type and they do not depend on length or scale.
void Beam::scaleTruck(float value)
{
	if(value<0) return;
	currentScale *= value;
	// scale beams
	for(int i=0;i<free_beam;i++)
	{
		//beams[i].k *= value;
		beams[i].d *= value;
		beams[i].L *= value;
		beams[i].refL *= value;
		beams[i].Lhydro *= value;
		beams[i].hydroRatio *= value;

		beams[i].diameter *= value;
		beams[i].lastforce *= value;
	}
	// scale nodes
	Vector3 refpos = nodes[0].AbsPosition;
	Vector3 relpos = nodes[0].RelPosition;
	Vector3 smopos = nodes[0].smoothpos;
	for(int i=1;i<free_node;i++)
	{
		nodes[i].iPosition = refpos + (nodes[i].iPosition-refpos) * value;
		nodes[i].AbsPosition = refpos + (nodes[i].AbsPosition-refpos) * value;
		nodes[i].RelPosition = relpos + (nodes[i].RelPosition-relpos) * value;
		nodes[i].smoothpos = smopos + (nodes[i].smoothpos-smopos) * value;
		nodes[i].Velocity *= value;
		nodes[i].Forces *= value;
		nodes[i].lockedPosition *= value;
		nodes[i].lockedVelocity *= value;
		nodes[i].lockedForces *= value;
		nodes[i].mass *= value;
	}
	updateSlideNodePositions();

	// props and stuff
	// TOFIX: care about prop positions as well!
	for(int i=0;i<free_prop;i++)
	{
		if(props[i].snode) props[i].snode->scale(value, value, value);
		if(props[i].wheel) props[i].wheel->scale(value, value, value);
		if(props[i].wheel) props[i].wheelpos = relpos + (props[i].wheelpos-relpos) * value;
		if(props[i].bbsnode[0]) props[i].bbsnode[0]->scale(value, value, value);
		if(props[i].bbsnode[1]) props[i].bbsnode[1]->scale(value, value, value);
		if(props[i].bbsnode[2]) props[i].bbsnode[2]->scale(value, value, value);
		if(props[i].bbsnode[3]) props[i].bbsnode[3]->scale(value, value, value);
	}
	// tell the cabmesh that resizing is ok, and they dont need to break ;)
	if(cabMesh) cabMesh->scale(value);
	// update engine values
	if(engine)
	{
		//engine->maxRPM *= value;
		//engine->iddleRPM *= value;
		engine->engineTorque *= value;
		//engine->stallRPM *= value;
		//engine->brakingTorque *= value;
	}
	// todo: scale flexbody
	for(int i=0;i<free_flexbody;i++)
	{
		flexbodies[i]->getSceneNode()->scale(value, value, value);
	}
	// todo: fix meshwheels
	//for(int i=0;i<free_wheel;i++)
	//{
		//if(vwheels[i].cnode) vwheels[i].cnode->scale(value, value, value);
		//if(vwheels[i].fm && vwheels[i].cnode) vwheels[i].cnode->scale(value, value, value);
	//}

}

void Beam::initSimpleSkeleton()
{
	// create
	simpleSkeletonManualObject =  tsm->createManualObject(String(truckname)+"_simpleskeleton");

	simpleSkeletonManualObject->estimateIndexCount(free_beam*2);
	simpleSkeletonManualObject->setCastShadows(false);
	simpleSkeletonManualObject->setDynamic(true);
	simpleSkeletonManualObject->setRenderingDistance(300);
	for(int i=0; i < free_beam; i++)
	{
		simpleSkeletonManualObject->begin("mat-beam-0", Ogre::RenderOperation::OT_LINE_LIST);
		simpleSkeletonManualObject->position(beams[i].p1->smoothpos);
		simpleSkeletonManualObject->position(beams[i].p2->smoothpos);
		simpleSkeletonManualObject->end();
	}
	simpleSkeletonNode->attachObject(simpleSkeletonManualObject);
	simpleSkeletonNode->setVisible(false);
	simpleSkeletonInitiated=true;
}

void Beam::updateSimpleSkeleton()
{
	if(!simpleSkeletonInitiated)
		initSimpleSkeleton();
	// just update
	for(int i=0; i < (int)simpleSkeletonManualObject->getNumSections(); i++)
	{
		if(i >= free_beam)
			break;

		int scale=(int)(beams[i].scale * 100);
		if(scale>100) scale=100;
		if(scale<-100) scale=-100;
		char bname[255];
		sprintf(bname, "mat-beam-%d", scale);

		simpleSkeletonManualObject->setMaterialName(i, bname);
		simpleSkeletonManualObject->beginUpdate(i);
		simpleSkeletonManualObject->position(beams[i].p1->smoothpos);
		// remove broken beams
		if(beams[i].broken || beams[i].disabled)
			simpleSkeletonManualObject->position(beams[i].p1->smoothpos);
		else
			simpleSkeletonManualObject->position(beams[i].p2->smoothpos);
		simpleSkeletonManualObject->end();
	}
}

void Beam::moveOrigin(Vector3 offset)
{
	changeOrigin(origin+offset);
}
//beware in multithreaded mode!
void Beam::changeOrigin(Vector3 newOrigin)
{
	Vector3 odiff=origin-newOrigin;
	origin=newOrigin;
	for (int i=0; i<free_node; i++)
	{
		nodes[i].RelPosition+=odiff;
	}
}

Vector3 Beam::getPosition()
{
	return position; //the position is already in absolute position
}

node_t *Beam::addNode(Vector3 pos)
{
	init_node(free_node, pos.x, pos.y, pos.z, NODE_NORMAL, 100, 0, 0, free_node);
	node_t *n = &nodes[free_node];

	// we must map the actual poitition back to init position
	n->iPosition = nodes[0].iPosition + (pos - nodes[0].AbsPosition);;
	free_node++;
	return n;
}

beam_t *Beam::addBeam(int id1, int id2)
{
	int type=BEAM_NORMAL;
	if (id1>=free_node || id2>=free_node)
	{
		LogManager::getSingleton().logMessage("Error: unknown node number in beams section ("
			+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
		exit(3);
	};
	//skip if a beam already exists
	LogManager::getSingleton().logMessage(StringConverter::toString(nodes[id1].AbsPosition)+" -> "+StringConverter::toString(nodes[id2].AbsPosition));
	int i;
	for (i=0; i<free_beam; i++)
	{
		if ((beams[i].p1==&nodes[id1] && beams[i].p2==&nodes[id2]) || (beams[i].p1==&nodes[id2] && beams[i].p2==&nodes[id1]))
		{
			LogManager::getSingleton().logMessage("Skipping duplicate beams: from node "+StringConverter::toString(id1)+" to node "+StringConverter::toString(id2));
			return NULL;
		}
	}

	int pos=add_beam(&nodes[id1], &nodes[id2], tsm, \
			  beamsRoot, type, default_break, default_spring, \
			  default_damp, -1, -1, -1, 1, \
			  default_beam_diameter);

	beams[pos].type=BEAM_NORMAL;
	return &beams[pos];
}

void Beam::checkBeamMaterial()
{
	if(MaterialManager::getSingleton().resourceExists("mat-beam-0"))
		return;
	int i = 0;
	char bname[255];
	for(i=-100;i<=100;i++)
	{
		//register a material for skeleton view
		sprintf(bname, "mat-beam-%d", i);
		MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().create(bname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));
		float f = fabs(((float)i)/100);
		if(i<=0)
			mat->getTechnique(0)->getPass(0)->createTextureUnitState()->setColourOperationEx(LBX_MODULATE, LBS_MANUAL, LBS_CURRENT, ColourValue(0.2f, 2.0f*(1.0f-f), f*2.0f, 0.8f));
		else
			mat->getTechnique(0)->getPass(0)->createTextureUnitState()->setColourOperationEx(LBX_MODULATE, LBS_MANUAL, LBS_CURRENT, ColourValue(f*2.0f, 2.0f*(1.0f-f), 0.2f, 0.8f));
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(TFO_ANISOTROPIC);
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
		mat->setLightingEnabled(false);
		mat->setReceiveShadows(false);
	}
}

void Beam::activate()
{
	if (state!=NETWORKED && state!=RECYCLE) state=ACTIVATED;
}

void Beam::desactivate()
{
	if (state!=NETWORKED && state!=RECYCLE)
	{
		state=DESACTIVATED;
		sleepcount=0;
	}
}

#if 0
// old netforce code
void Beam::pushNetForce(int node_id, Vector3 force)
{
	for (int i=0; i<MAX_NETFORCE; i++)
	{
		if (netforces[i].used && netforces[i].node==node_id)
		{
			netforces[i].force=force;
			netforces[i].birthdate=mrtime;
			return;
		}
	}
	//its a new one
	for (int i=0; i<MAX_NETFORCE; i++)
	{
		if (!netforces[i].used)
		{
			netforces[i].used=true;
			netforces[i].node=node_id;
			netforces[i].force=force;
			netforces[i].birthdate=mrtime;
			return;
		}
	}
}

void Beam::expireNetForce()
{
	for (int i=0; i<MAX_NETFORCE; i++)
	{
		if (netforces[i].used && mrtime-netforces[i].birthdate>0.5) netforces[i].used=false;
	}
}
#endif //0

inline bool Beam::inRange(float num, float min, float max)
{
	return (num <= max && num >= min);
}

//called by the network thread
void Beam::pushNetwork(char* data, int size)
{
	if(!oob3) return;
	if ((unsigned int)size==(netbuffersize+sizeof(oob_t)))
	{
		memcpy((char*)oob3, data, sizeof(oob_t));
		memcpy((char*)netb3, data+sizeof(oob_t), nodebuffersize);
		//take care of the wheels
		for (int i=0; i<free_wheel; i++) wheels[i].rp3=*(float*)(data+sizeof(oob_t)+nodebuffersize+i*4);
	} else
	{
		LogManager::getSingleton().logMessage("WRONG network size: we expected " + StringConverter::toString(netbuffersize+sizeof(oob_t)) + " but got " + StringConverter::toString(size) + " for vehicle " + String(truckname));
		state=SLEEPING;
		return;
	};
	//okay, the big switch
	pthread_mutex_lock(&net_mutex);
	oob_t *ot;
	ot=oob1;
	oob1=oob2;
	oob2=oob3;
	oob3=ot;
	char *ft;
	ft=netb1;
	netb1=netb2;
	netb2=netb3;
	netb3=ft;
	for (int i=0; i<free_wheel; i++)
	{
		float rp;
		rp=wheels[i].rp1;
		wheels[i].rp1=wheels[i].rp2;
		wheels[i].rp2=wheels[i].rp3;
		wheels[i].rp3=rp;
	}
	netcounter++;
	pthread_mutex_unlock(&net_mutex);
}

void Beam::calcNetwork()
{
	Vector3 apos=Vector3::ZERO;
	if (netcounter<4) return;
	//we must update Nodes positions from available network informations
	//we must lock as long as we use oob1, oob2, netb1, netb2
	pthread_mutex_lock(&net_mutex);
	int i;
	int tnow=nettimer->getMilliseconds();
	//adjust offset to match remote time
	int rnow=tnow+net_toffset;
	//if we receive older data from the future, we must correct the offset
	if (oob1->time>rnow) {net_toffset=oob1->time-tnow; rnow=tnow+net_toffset;}
	//if we receive last data from the past, we must correct the offset
	if (oob2->time<rnow) {net_toffset=oob2->time-tnow; rnow=tnow+net_toffset;}
	float tratio=(float)(rnow-oob1->time)/(float)(oob2->time-oob1->time);
	//LogManager::getSingleton().logMessage(" network time diff: "+ StringConverter::toString(net_toffset));
	Vector3 p1ref = Vector3::ZERO;
	Vector3 p2ref = Vector3::ZERO;
	short *sp1=(short*)(netb1+4*3);
	short *sp2=(short*)(netb2+4*3);

	for (i=0; i<first_wheel_node; i++)
	{
		Vector3 p1;
		Vector3 p2;
		//linear interpolation
		if (i==0)
		{
			p1.x=((float*)netb1)[0];
			p1.y=((float*)netb1)[1];
			p1.z=((float*)netb1)[2];
			p1ref=p1;
			p2.x=((float*)netb2)[0];
			p2.y=((float*)netb2)[1];
			p2.z=((float*)netb2)[2];
			p2ref=p2;
		}
		else
		{
			p1.x=(float)(sp1[(i-1)*3])/300.0;
			p1.y=(float)(sp1[(i-1)*3+1])/300.0;
			p1.z=(float)(sp1[(i-1)*3+2])/300.0;
			p1=p1+p1ref;
			p2.x=(float)(sp2[(i-1)*3])/300.0;
			p2.y=(float)(sp2[(i-1)*3+1])/300.0;
			p2.z=(float)(sp2[(i-1)*3+2])/300.0;
			p2=p2+p2ref;
		}
		nodes[i].AbsPosition=p1+tratio*(p2-p1);
		nodes[i].smoothpos=nodes[i].AbsPosition;
		nodes[i].RelPosition=nodes[i].AbsPosition-origin;
		apos+=nodes[i].AbsPosition;
	}
	position=apos/first_wheel_node;
	//the wheels
	for (i=0; i<free_wheel; i++)
	{
		float rp=wheels[i].rp1+tratio*(wheels[i].rp2-wheels[i].rp1);
		//compute ideal positions
		Vector3 axis=wheels[i].refnode1->RelPosition-wheels[i].refnode0->RelPosition;
		axis.normalise();
		Plane pplan=Plane(axis, wheels[i].refnode0->AbsPosition);
		Vector3 ortho=-pplan.projectVector(wheels[i].near_attach->AbsPosition)-wheels[i].refnode0->AbsPosition;
		Vector3 ray=ortho.crossProduct(axis);
		ray.normalise();
		ray=ray*wheels[i].radius;
		float drp=2.0*3.14159/(wheels[i].nbnodes/2);
		for (int j=0; j<wheels[i].nbnodes/2; j++)
		{
			Vector3 uray=Quaternion(Radian(rp-drp*j), axis)*ray;
			wheels[i].nodes[j*2]->AbsPosition=wheels[i].refnode0->AbsPosition+uray;
			wheels[i].nodes[j*2]->smoothpos=wheels[i].nodes[j*2]->AbsPosition;
			wheels[i].nodes[j*2]->RelPosition=wheels[i].nodes[j*2]->AbsPosition-origin;

			wheels[i].nodes[j*2+1]->AbsPosition=wheels[i].refnode1->AbsPosition+uray;
			wheels[i].nodes[j*2+1]->smoothpos=wheels[i].nodes[j*2+1]->AbsPosition;
			wheels[i].nodes[j*2+1]->RelPosition=wheels[i].nodes[j*2+1]->AbsPosition-origin;
		}
	}
	//give some slack to the mutex
	float engspeed=oob1->engine_speed+tratio*(oob2->engine_speed-oob1->engine_speed);
	float engforce=oob1->engine_force+tratio*(oob2->engine_force-oob1->engine_force);
	unsigned int flagmask=oob1->flagmask;

	pthread_mutex_unlock(&net_mutex);
	if (engine)
	{
#ifdef USE_OPENAL
		ssm->modulate(trucknum, SS_MOD_ENGINE, engspeed);
#endif //OPENAL
	}
	if(free_aeroengine>0)
	{
#ifdef USE_OPENAL
		ssm->modulate(trucknum, SS_MOD_AEROENGINE1, engspeed);
		ssm->modulate(trucknum, SS_MOD_AEROENGINE2, engspeed);
		ssm->modulate(trucknum, SS_MOD_AEROENGINE3, engspeed);
		ssm->modulate(trucknum, SS_MOD_AEROENGINE4, engspeed);
#endif //OPENAL
	}
	if (engine) engine->netForceSettings(engspeed, engforce); //for smoke


	// set particle cannon
	if (((flagmask&NETMASK_PARTICLE)!=0) != cparticle_mode)
		toggleCustomParticles();

	// set lights
	if (((flagmask&NETMASK_LIGHTS)!=0) != lights)
		lightsToggle(0,0);
	if (((flagmask&NETMASK_BEACONS)!=0) != beacon)
		beaconsToggle();

	blinktype btype = BLINK_NONE;
	if ((flagmask&NETMASK_BLINK_LEFT)!=0)
		btype = BLINK_LEFT;
	else if ((flagmask&NETMASK_BLINK_RIGHT)!=0)
		btype = BLINK_RIGHT;
	else if ((flagmask&NETMASK_BLINK_WARN)!=0)
		btype = BLINK_WARN;
	setBlinkType(btype);

	setCustomLightVisible(0, ((flagmask&NETMASK_CLIGHT1)>0));
	setCustomLightVisible(1, ((flagmask&NETMASK_CLIGHT2)>0));
	setCustomLightVisible(2, ((flagmask&NETMASK_CLIGHT3)>0));
	setCustomLightVisible(3, ((flagmask&NETMASK_CLIGHT4)>0));

#ifdef USE_OPENAL
	if (flagmask&NETMASK_HORN)
		ssm->trigStart(trucknum, SS_TRIG_HORN);
	else
		ssm->trigStop(trucknum, SS_TRIG_HORN);
#endif //OPENAL
	netBrakeLight = ((flagmask&NETMASK_BRAKES)!=0);
	netReverseLight = ((flagmask&NETMASK_REVERSE)!=0);

#ifdef USE_OPENAL
	if(netReverseLight)
		ssm->trigStart(trucknum, SS_TRIG_REVERSE_GEAR);
	else
		ssm->trigStop(trucknum, SS_TRIG_REVERSE_GEAR);
#endif //OPENAL

}

void Beam::addPressure(float v)
{
	refpressure+=v;
	if (refpressure<0) refpressure=0;
	if (refpressure>100) refpressure=100;
	for (int i=0; i<free_pressure_beam; i++)
	{
		beams[pressure_beams[i]].k=10000+refpressure*10000;
	}
}

float Beam::getPressure()
{
	if (free_pressure_beam) return refpressure;
	return 0;
}

void Beam::calc_masses2(Real total, bool reCalc)
{

	bool debugMass=(SETTINGS.getSetting("Debug Truck Mass")=="Yes");


	int i;
	Real len=0.0;
	//reset
	for (i=0; i<free_node; i++)
	{
		if (!nodes[i].iswheel)
		{
			if (!nodes[i].masstype==NODE_LOADED)
				nodes[i].mass=0;
			else
			{
				if (nodes[i].overrideMass)
					// we set the mass before already!
					continue;
				else
					nodes[i].mass=loadmass/(float)masscount;
			}
		}
	}
	//average linear density
	for (i=0; i<free_beam; i++)
	{
		if (beams[i].type!=BEAM_VIRTUAL)
		{
			Real newlen=beams[i].L;
			if (!(beams[i].p1->iswheel)) len+=newlen/2.0;
			if (!(beams[i].p2->iswheel)) len+=newlen/2.0;
		};
	}
	if(!reCalc)
	{
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].type!=BEAM_VIRTUAL)
			{
				Real mass=beams[i].L*total/len;
				if (!(beams[i].p1->iswheel)) beams[i].p1->mass+=mass/2;
				if (!(beams[i].p2->iswheel)) beams[i].p2->mass+=mass/2;
			};
		}
	}
	//fix rope masses
	for(std::vector <rope_t>::iterator it = ropes.begin(); it!=ropes.end(); it++)
	{
		it->beam->p2->mass=100.0;
	}
	//fix camera mass
	for (i=0; i<freecinecamera; i++)
		nodes[cinecameranodepos[i]].mass=20;

	//hooks must be heavy
	for(std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
		if(!it->hookNode->overrideMass)
			it->hookNode->mass = 500.0f;

	//update gravimass
	for (i=0; i<free_node; i++)
	{
		//LogManager::getSingleton().logMessage("Nodemass "+StringConverter::toString(i)+"-"+StringConverter::toString(nodes[i].mass));
		//for stability
		if (!nodes[i].iswheel && nodes[i].mass<minimass)
		{
			if(debugMass)
				LogManager::getSingleton().logMessage("Node " + StringConverter::toString(i) +" mass ("+StringConverter::toString(nodes[i].mass)+"kg) too light. Resetting to minimass ("+ StringConverter::toString(minimass) +"kg).");
			nodes[i].mass=minimass;
		}
		nodes[i].gravimass=Vector3(0, ExampleFrameListener::getGravity() * nodes[i].mass, 0);
	}

    // update inverted mass cache
	for (i=0; i<free_node; i++)
	{
		nodes[i].inverted_mass=1.0f/nodes[i].mass;
    }

	//update minendmass
	for (i=0; i<free_beam; i++)
	{
		beams[i].minendmass=beams[i].p1->mass;
		if (beams[i].p2->mass < beams[i].minendmass)
			beams[i].minendmass=beams[i].p2->mass;
	}
	totalmass=0;
	for (i=0; i<free_node; i++)
	{
		if(debugMass)
		{
			String msg = "Node " + StringConverter::toString(i) +" : "+ StringConverter::toString((int)nodes[i].mass) +" kg";
			if (nodes[i].masstype==NODE_LOADED)
			{
				if (nodes[i].overrideMass)
					msg +=  " (overriden by node mass)";
				else
					msg +=  " (normal load node: "+StringConverter::toString(loadmass)+" kg / "+StringConverter::toString(masscount)+" nodes)";
			}
			LogManager::getSingleton().logMessage(msg);
		}
		totalmass+=nodes[i].mass;
	}
	LogManager::getSingleton().logMessage("TOTAL VEHICLE MASS: " + StringConverter::toString((int)totalmass) +" kg");
	//now a special stuff
	int unst=0;
	int st=0;
	int wunst=0;
	for (i=0; i<free_beam; i++)
	{
		float mass=beams[i].p1->mass;
		if (beams[i].p2->mass<mass) mass=beams[i].p2->mass;
	}
	LogManager::getSingleton().logMessage("Beams status: unstable:"+StringConverter::toString(unst)+" wheel:"+StringConverter::toString(wunst)+" normal:"+StringConverter::toString(free_beam-unst-wunst-st)+" superstable:"+StringConverter::toString(st));
}

// this recalcs the masses, useful when the gravity was changed...
void Beam::recalc_masses()
{
	this->calc_masses2(totalmass, true);
}

float Beam::getTotalMass(bool withLocked)
{
	float mass = totalmass; //already computed in calc_masses2
	if (withLocked)
	{
		for(std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
		{
			if(it->lockTruck && it->lockTruck->getTruckName() != getTruckName())
			{
				mass += it->lockTruck->getTotalMass();
			}
		}
	}
	return mass;
}

int Beam::getWheelNodeCount()
{
	return free_node-first_wheel_node;
}

//to load a truck file
int Beam::loadTruck(const char* fname, SceneManager *manager, SceneNode *parent, Real px, Real py, Real pz, Quaternion rot, collision_box_t *spawnbox)
{
	ScopeLog log("beam_"+String(fname));
	//FILE *fd;
	char line[1024];
	int linecounter = 0;
	int mode=0, savedmode=0;
	int hasfixes=0;
	int wingstart=-1;
	int leftlight=0;
	int rightlight=0;
	float wingarea=0.0;
	int currentScriptCommandNumber=-1;
	bool enable_truck_lod = (SETTINGS.getSetting("Truck LOD Enabled") == "Yes");

	//convert ry
	//ry=ry*3.14159/180.0;
	LogManager::getSingleton().logMessage("BEAM: Start of truck loading");
	String group = "";
	String filename = String(fname);

	try
	{
		if(!CACHE.checkResourceLoaded(filename, group))
		{
			LogManager::getSingleton().logMessage("Can't open truck file '"+filename+"'");
			return -1;
		}
	} catch(Ogre::Exception& e)
	{
		if(e.getNumber() == Ogre::Exception::ERR_ITEM_NOT_FOUND)
		{
			LogManager::getSingleton().logMessage("Can't open truck file '"+filename+"'");
			return -1;
		}
	}

	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(filename, group);
	linecounter = 0;

//		fd=fopen(fname, "r");
//		if (!fd) {
//			LogManager::getSingleton().logMessage("Can't open truck file '"+String(fname)+"'");
//			exit(1);
//		};
	LogManager::getSingleton().logMessage("Parsing '"+String(fname)+"'");
	//skip first line
//		fscanf(fd," %[^\n\r]",line);
	ds->readLine(line, 1023);
	// read in truckname for real
	strncpy(realtruckname, line, 255);

	// some temp vars used
	Real inertia_startDelay=-1, inertia_stopDelay=-1;
	char inertia_default_startFunction[50]="", inertia_default_stopFunction[50]="";

	while (!ds->eof())
	{
		size_t ll=ds->readLine(line, 1023);
		linecounter++;
//			fscanf(fd," %[^\n\r]",line);
		//LogManager::getSingleton().logMessage(line);
		//        printf("Mode %i Line:'%s'\n",mode, line);
		if (ll==0 || line[0]==';' || line[0]=='/')
		{
			//    printf("%s\n", line+1);
			continue;
		};

		if (!strcmp("end",line))
		{
			LogManager::getSingleton().logMessage("BEAM: End of truck loading");
			loading_finished=1;break;
		};

		if (!strcmp("patchEngineTorque",line)) {patchEngineTorque=true;continue;};
		if (!strcmp("end_commandlist",line) && mode == 35) {mode=0;continue;};
		if (!strcmp("end_description",line) && mode == 29) {mode=0;continue;};
		if (!strcmp("end_comment",line)  && mode == 30) {mode=savedmode;continue;};
		if (!strcmp("end_section",line)  && mode == 52) {mode=savedmode;continue;};
		if (mode==29)
		{
			// description
			//parse dscription, and ignore every possible keyword
			//char *tmp = strdup(line);
			description.push_back(std::string(line));
			continue;
		}

		if (mode==30)
		{
			// comment
			// ignore everything
			continue;
		}
		if (mode==52)
		{
			// ignored truck section
			continue;
		}
		if (!strcmp("nodes",line)) {mode=1;continue;};
		if (!strcmp("beams",line)) {mode=2;continue;};
		if (!strcmp("fixes",line)) {mode=3;continue;};
		if (!strcmp("shocks",line)) {mode=4;continue;};
		if (!strcmp("hydros",line)) {mode=5;continue;};
		if (!strcmp("wheels",line)) {mode=6;continue;};
		if (!strcmp("globals",line)) {mode=7;continue;};
		if (!strcmp("cameras",line)) {mode=8;continue;};
		if (!strcmp("engine",line)) {mode=9;continue;};
		if (!strcmp("texcoords",line)) {mode=10;continue;};
		if (!strcmp("cab",line)) {mode=11;continue;};
		if (!strcmp("commands",line)) {mode=12;continue;};
		if (!strcmp("commands2",line)) {mode=120;continue;};
		if (!strcmp("forwardcommands",line)) {forwardcommands=1;continue;};
		if (!strcmp("importcommands",line)) {importcommands=1;continue;};
		if (!strcmp("rollon",line)) {wheel_contact_requested=true;continue;};
		if (!strcmp("rescuer",line)) {rescuer=true;continue;};
		if (!strcmp("contacters",line)) {mode=13;continue;};
		if (!strcmp("ropes",line)) {mode=14;continue;};
		if (!strcmp("ropables",line)) {mode=15;continue;};
		if (!strcmp("ties",line)) {mode=16;continue;};
		if (!strcmp("help",line)) {mode=17;continue;};
		if (!strcmp("cinecam",line)) {mode=18;continue;};
		if (!strcmp("flares",line)) {mode=19;continue;};
		if (!strcmp("props",line)) {mode=20;continue;};
		if (!strcmp("globeams",line)) {mode=21;continue;};
		if (!strcmp("wings",line)) {mode=22;continue;};
		if (!strcmp("turboprops",line)) {mode=23;continue;};
		if (!strcmp("fusedrag",line)) {mode=24;continue;};
		if (!strcmp("engoption",line)) {mode=25;continue;};
		if (!strcmp("brakes",line)) {mode=26;continue;};
		if (!strcmp("rotators",line)) {mode=27;continue;};
		if (!strcmp("screwprops",line)) {mode=28;continue;};
		if (!strcmp("description",line)) {mode=29;continue;};
		if (!strcmp("comment",line)) {mode=30; savedmode=mode; continue;};
		if (!strcmp("wheels2",line)) {mode=31;continue;};
		if (!strcmp("guisettings",line)) {mode=32;continue;};
		if (!strcmp("minimass",line)) {mode=33;continue;};
		if (!strcmp("exhausts",line)) {mode=34;continue;};
		if (!strcmp("turboprops2",line)) {mode=35;continue;};
		if (!strcmp("pistonprops",line)) {mode=36;continue;};
		//apparently mode 37 is reserved for other use
		if (!strcmp("particles",line)) {mode=38;continue;};
		if (!strcmp("turbojets",line)) {mode=39;continue;};
		if (!strcmp("rigidifiers",line)) {mode=40;continue;};
		if (!strcmp("airbrakes",line)) {mode=41;continue;};
		if (!strcmp("meshwheels",line)) {mode=42;continue;};
		if (!strcmp("flexbodies",line)) {mode=43;continue;};
		if (!strcmp("hookgroup",line)) {mode=44; continue;};
		if (!strncmp("materialflarebindings",line, 21)) {mode=46; continue;};
		if (!strcmp("disabledefaultsounds",line)) {disable_default_sounds=true;continue;};
		if (!strcmp("soundsources",line)) {mode=47;continue;};
		if (!strcmp("envmap",line)) {mode=48;continue;};
		if (!strcmp("managedmaterials",line)) {mode=49;continue;};
		if (!strncmp("sectionconfig",line, 13)) {savedmode=mode;mode=50; /* NOT continue */};
		if (!strncmp("section",line, 7) && mode!=50) {mode=51; /* NOT continue */};
		/* 52 = reserved for ignored section */
		if (!strcmp("torquecurve",line)) {mode=53;continue;};
		if (!strcmp("advdrag",line)) {mode=54;continue;};
		if (!strcmp("axles",line)) {mode=55;continue;};
		if (!strcmp("shocks2",line)) {mode=56;continue;};
		if (!strcmp("railgroups",line)) {mode=63;continue;}
		if (!strcmp("slidenodes",line)) {mode=64;continue;}
		if (!strcmp("flares2",line)) {mode=65;continue;};
		if (!strcmp("animators",line)) {mode=66;continue;};
		if (!strncmp("enable_advanced_deformation", line, 27))
		{
			// parse the optional threshold value
			beam_creak = 0.0f;
			LogManager::getSingleton().logMessage("Advanced deformation beam physics enabled");
			continue;
		}
		if (!strcmp("commandlist",line))
		{
			int result = sscanf(line,"commandlist %d", &currentScriptCommandNumber);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (commandlist) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			mode=37;
			continue;
		};
		if (!strncmp("fileinfo", line, 8))
		{
			int result = sscanf(line,"fileinfo %s, %i, %i", uniquetruckid, &categoryid, &truckversion);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (fileinfo) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			mode=0;
			continue;
		}
		if (!strncmp("fileformatversion", line, 17))
		{
			int fileformatversion;
			int result = sscanf(line,"fileformatversion %i", &fileformatversion);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (fileformatversion) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (fileformatversion > TRUCKFILEFORMATVERSION) {
				LogManager::getSingleton().logMessage("The Truck File " + String(fname) +" is for a newer version or RoR! trying to continue ...");
				continue;
			}
			mode=0;
			continue;
		}
		if (!strncmp("author", line, 6))
		{
			int authorid;
			char authorname[255], authoremail[255], authortype[255];
			AuthorInfo author;
			author.id = -1;
			author.email = "unknown";
			author.name = "unknown";
			author.type = "unknown";

			int result = sscanf(line,"author %s %i %s %s", authortype, &authorid, authorname, authoremail);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (author) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//replace '_' with ' '
			char *tmp = authorname;
			while (*tmp!=0) {if (*tmp=='_') *tmp=' ';tmp++;};
			//fill the struct now
			author.id = authorid;
			if(strnlen(authortype, 250) > 0)
				author.type = String(authortype);
			if(strnlen(authorname, 250) > 0)
				author.name = String(authorname);
			if(strnlen(authoremail, 250) > 0)
				author.email = String(authoremail);
			authors.push_back(author);
			mode=0;
			continue;
		}

		if(!strcmp("slidenode_connect_instantly", line))
			slideNodesConnectInstantly=true;

		if (!strncmp("add_animation", line, 13))
		{
			/*
			 * this command has several layers for splitting up the line:
			 * 1. ',' the top level will be split up with a comma to separate the main options
			 * 2. ':' the second level will be split up with colon, it separates the entry name and its value
			 * 3. '|' the third level is used to specify multiple values for the entry value 
			 */
			int animnum = 0;
			float ratio = 0.0f, opt1 = -1.0f, opt2 = -1.0f;
			// parse the line
			if(String(line).size() < 14)
			{
				LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Not enough Options parsed, trying to continue ...");
				continue;
			}
			Ogre::vector<Ogre::String>::type options = Ogre::StringUtil::split(String(line).substr(14), ","); // "add_animation " = 14 characters
			// check for common errors
			if (options.size() < 4)
			{
				LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Not enough Options parsed, trying to continue ...");
				continue;
			}
			if (!free_prop)
			{
				LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". No prop to animate existing, trying to continue ...");
				continue;
			}

			// always use the last prop
			prop_t *prop = &props[free_prop-1];

			// look for a free anim slot, important: do not over the borders!
			while (prop->animFlags[animnum] && animnum < 10) 
				animnum++;

			// all slots used?
			if (animnum >= 10)
			{
				LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Cant animate a prop more then 10 times. Trying to continue...");
				continue;
			}

			// parse the arguments individually
			for(unsigned int i=0;i<options.size();i++)
			{
				if(i == 0)
				{
					ratio = StringConverter::parseReal(options[i]);
					//set ratio
					if (ratio)
						prop->animratio[animnum]=ratio;
					else
						LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Animation-Ratio = 0 ?");
				} else if(i == 1)
				{
					opt1 = StringConverter::parseReal(options[i]);
					prop->animOpt1[animnum] = opt1;
				} else if(i == 2)
				{
					opt2 = StringConverter::parseReal(options[i]);
					prop->animOpt2[animnum] = opt2;
					prop->animOpt3[animnum] = 0.0f;
					prop->animOpt4[animnum] = 0.0f;
					prop->animOpt5[animnum] = 0.0f;
					prop->animKeyState[animnum] = -1.0f;
				} else
				{
					// parse the rest
					Ogre::vector<Ogre::String>::type args2 = Ogre::StringUtil::split(options[i], ":");
					if(args2.size() == 0)
						continue;
					
					// trim spaces from the entry
					Ogre::StringUtil::trim(args2[0]);
					if(args2.size() >= 2) Ogre::StringUtil::trim(args2[1]);

					if(args2[0] == "source" && args2.size() == 2)
					{
						//set source identification flag
						Ogre::vector<Ogre::String>::type args3 = Ogre::StringUtil::split(args2[1], "|");
						for(unsigned int j=0;j<args3.size();j++)
						{
							String sourceStr = args3[j];
							Ogre::StringUtil::trim(sourceStr);
							if (sourceStr == "airspeed")             { prop->animFlags[animnum] |= (ANIM_FLAG_AIRSPEED); }
							else if (sourceStr == "vvi")             { prop->animFlags[animnum] |= (ANIM_FLAG_VVI); }
							else if (sourceStr == "altimeter100k")   { prop->animFlags[animnum] |= (ANIM_FLAG_ALTIMETER); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "altimeter10k")    { prop->animFlags[animnum] |= (ANIM_FLAG_ALTIMETER); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "altimeter1k")     { prop->animFlags[animnum] |= (ANIM_FLAG_ALTIMETER); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "aoa")             { prop->animFlags[animnum] |= (ANIM_FLAG_AOA); }
							else if (sourceStr == "flap")            { prop->animFlags[animnum] |= (ANIM_FLAG_FLAP); }
							else if (sourceStr == "airbrake")        { prop->animFlags[animnum] |= (ANIM_FLAG_AIRBRAKE); }
							else if (sourceStr == "roll")            { prop->animFlags[animnum] |= (ANIM_FLAG_ROLL); }
							else if (sourceStr == "pitch")           { prop->animFlags[animnum] |= (ANIM_FLAG_PITCH); }
							else if (sourceStr == "throttle1")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "throttle2")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "throttle3")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "throttle4")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "throttle5")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "throttle6")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "throttle7")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "throttle8")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "rpm1")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "rpm2")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "rpm3")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "rpm4")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "rpm5")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "rpm6")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "rpm7")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "rpm8")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "aerotorq1")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "aerotorq2")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "aerotorq3")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "aerotorq4")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "aerotorq5")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "aerotorq6")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "aerotorq7")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "aerotorq8")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "aeropit1")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "aeropit2")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "aeropit3")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "aeropit4")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "aeropit5")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "aeropit6")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "aeropit7")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "aeropit8")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "aerostatus1")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "aerostatus2")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "aerostatus3")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "aerostatus4")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "aerostatus5")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "aerostatus6")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "aerostatus7")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "aerostatus8")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "brakes")          { prop->animFlags[animnum] |= (ANIM_FLAG_BRAKE); }
							else if (sourceStr == "accel")           { prop->animFlags[animnum] |= (ANIM_FLAG_ACCEL); }
							else if (sourceStr == "clutch")          { prop->animFlags[animnum] |= (ANIM_FLAG_CLUTCH); }
							else if (sourceStr == "speedo")          { prop->animFlags[animnum] |= (ANIM_FLAG_SPEEDO); }
							else if (sourceStr == "tacho")           { prop->animFlags[animnum] |= (ANIM_FLAG_TACHO); }
							else if (sourceStr == "turbo")           { prop->animFlags[animnum] |= (ANIM_FLAG_TURBO); }
							else if (sourceStr == "parking")         { prop->animFlags[animnum] |= (ANIM_FLAG_PBRAKE); }
							else if (sourceStr == "shifterman1")     { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "shifterman2")     { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "sequential")      { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "shifterlin")      { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "torque")          { prop->animFlags[animnum] |= (ANIM_FLAG_TORQUE); }
							else if (sourceStr == "heading")         { prop->animFlags[animnum] |= (ANIM_FLAG_HEADING); }
							else if (sourceStr == "difflock")        { prop->animFlags[animnum] |= (ANIM_FLAG_DIFFLOCK); }
							else if (sourceStr == "steeringwheel")   { prop->animFlags[animnum] |= (ANIM_FLAG_STEERING); }
							else if (sourceStr == "aileron")         { prop->animFlags[animnum] |= (ANIM_FLAG_AILERONS); }
							else if (sourceStr == "elevator")        { prop->animFlags[animnum] |= (ANIM_FLAG_ELEVATORS); }
							else if (sourceStr == "rudderair")       { prop->animFlags[animnum] |= (ANIM_FLAG_ARUDDER); }
							else if (sourceStr == "rudderboat")      { prop->animFlags[animnum] |= (ANIM_FLAG_BRUDDER); }
							else if (sourceStr == "throttleboat")    { prop->animFlags[animnum] |= (ANIM_FLAG_BTHROTTLE); }
							else if (sourceStr == "permanent")       { prop->animFlags[animnum] |= (ANIM_FLAG_PERMANENT); }
							else if (sourceStr == "event")           { prop->animFlags[animnum] |= (ANIM_FLAG_EVENT); }
						}

						if (prop->animFlags[animnum] == 0)
							LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Failed to identify source.");
						else
							LogManager::getSingleton().logMessage("Animation source set to prop#: " + StringConverter::toString(free_prop-1) + ", flag " +StringConverter::toString(prop->animFlags[animnum]) + ", Animationnumber: " + StringConverter::toString(animnum));
					}
					else if(args2[0] == "mode" && args2.size() == 2)
					{
						//set mode identification flag
						Ogre::vector<Ogre::String>::type args3 = Ogre::StringUtil::split(args2[1], "|");
						for(unsigned int j=0;j<args3.size();j++)
						{
							String modeStr = args3[j];
							Ogre::StringUtil::trim(modeStr);
							if (modeStr == "x-rotation")      prop->animMode[animnum] |= (ANIM_MODE_ROTA_X);
							else if (modeStr == "y-rotation") prop->animMode[animnum] |= (ANIM_MODE_ROTA_Y);
							else if (modeStr == "z-rotation") prop->animMode[animnum] |= (ANIM_MODE_ROTA_Z);
							else if (modeStr == "x-offset")   prop->animMode[animnum] |= (ANIM_MODE_OFFSET_X);
							else if (modeStr == "y-offset")   prop->animMode[animnum] |= (ANIM_MODE_OFFSET_Y);
							else if (modeStr == "z-offset")   prop->animMode[animnum] |= (ANIM_MODE_OFFSET_Z);
						}

						if (prop->animMode[animnum] == 0)
							LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Failed to identify animation mode.");
						else
							LogManager::getSingleton().logMessage("Animation mode set to prop#: " + StringConverter::toString(free_prop-1)+ ", mode " +StringConverter::toString(prop->animMode[animnum]) + ", Animationnumber: " + StringConverter::toString(animnum));
					}
					else if (args2[0] == "autoanimate" && args2.size() == 1)
					{
						// TODO: re-add check for invalid cases
						prop->animMode[animnum] |= (ANIM_MODE_AUTOANIMATE);
						
						if     (prop->animMode[animnum] & ANIM_MODE_ROTA_X)   { prop->animOpt1[animnum] = opt1 + prop->rotaX; prop->animOpt2[animnum] = opt2 + prop->rotaX; prop->animOpt4[animnum] = prop->rotaX; }
						else if(prop->animMode[animnum] & ANIM_MODE_ROTA_Y)   { prop->animOpt1[animnum] = opt1 + prop->rotaY; prop->animOpt2[animnum] = opt2 + prop->rotaY; prop->animOpt4[animnum] = prop->rotaY; }
						else if(prop->animMode[animnum] & ANIM_MODE_ROTA_Z)   { prop->animOpt1[animnum] = opt1 + prop->rotaZ; prop->animOpt2[animnum] = opt2 + prop->rotaZ; prop->animOpt4[animnum] = prop->rotaZ; }
						else if(prop->animMode[animnum] & ANIM_MODE_OFFSET_X) { prop->animOpt1[animnum] = opt1 + prop->orgoffsetX; prop->animOpt2[animnum] = opt2 + prop->orgoffsetX; prop->animOpt4[animnum] = prop->orgoffsetX; }
						else if(prop->animMode[animnum] & ANIM_MODE_OFFSET_Y) { prop->animOpt1[animnum] = opt1 + prop->orgoffsetY; prop->animOpt2[animnum] = opt2 + prop->orgoffsetY; prop->animOpt4[animnum] = prop->orgoffsetY; }
						else if(prop->animMode[animnum] & ANIM_MODE_OFFSET_Z) { prop->animOpt1[animnum] = opt1 + prop->orgoffsetZ; prop->animOpt2[animnum] = opt2 + prop->orgoffsetZ; prop->animOpt4[animnum] = prop->orgoffsetZ; }
						LogManager::getSingleton().logMessage("Animation mode Autoanimation added to prop#: " + StringConverter::toString(free_prop-1) + " , Animationnumber: " + StringConverter::toString(animnum));
					}
					else if (args2[0] == "noflip" && args2.size() == 1)
					{
						prop->animMode[animnum] |= (ANIM_MODE_NOFLIP);
					}
					else if (args2[0] == "bounce" && args2.size() == 1)
					{
						prop->animMode[animnum] |= (ANIM_MODE_BOUNCE);
						prop->animOpt5[animnum] = 1.0f;
					}
					else if (args2[0] == "eventlock" && args2.size() == 1)
					{
						prop->animKeyState[animnum] = 0.0f;
						prop->lastanimKS[animnum] = 0.0f;
					}
					else if (args2[0] == "event" && args2.size() == 2)
					{
						// we are using keys as source
						prop->animFlags[animnum] |= ANIM_FLAG_EVENT;
						
						// now parse the keys
						prop->animFlags[animnum] |= ANIM_FLAG_EVENT;
						Ogre::vector<Ogre::String>::type args3 = Ogre::StringUtil::split(args2[1], "|");
						for(unsigned int j=0;j<args3.size();j++)
						{
							String eventStr = args3[j];
							Ogre::StringUtil::trim(eventStr);
							Ogre::StringUtil::toUpperCase(eventStr);
							int evtID = INPUTENGINE.resolveEventName(eventStr);
							if(evtID != -1)
								prop->animKey[animnum] = evtID;
							else
								LogManager::getSingleton().logMessage("Animation event unkown: " + eventStr);
						}
					}

				}
			}
			continue;
		}

		if (!strncmp("set_beam_defaults", line, 17))
		{
			char default_beam_material2[256]="";
			float tmpdefault_plastic_coef=-1.0f;
			int result = sscanf(line,"set_beam_defaults %f, %f, %f, %f, %f, %s, %f", &default_spring, &default_damp, &default_deform,&default_break,&default_beam_diameter, default_beam_material2, &tmpdefault_plastic_coef);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (set_beam_defaults) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(strnlen(default_beam_material2, 255))
			{
				MaterialPtr mat = MaterialManager::getSingleton().getByName(String(default_beam_material2));
				if(!mat.isNull())
					strncpy(default_beam_material, default_beam_material2, 256);
				else
					LogManager::getSingleton().logMessage("beam material '" + String(default_beam_material2) + "' not found!");
			}
			if (default_spring<0) default_spring=DEFAULT_SPRING;
			if (default_damp<0) default_damp=DEFAULT_DAMP;
			if (default_deform<0) default_deform=BEAM_DEFORM;
			if (default_break<0) default_break=BEAM_BREAK;
			if (default_beam_diameter<0) default_beam_diameter=DEFAULT_BEAM_DIAMETER;
			if (tmpdefault_plastic_coef>=0.0f)
			{
				beam_creak=0.0f;
				default_plastic_coef=tmpdefault_plastic_coef;
			}
			continue;
		}
		if (!strncmp("set_inertia_defaults", line, 20))
		{
			int result = sscanf(line,"set_inertia_defaults %f, %f, %s %s",&inertia_startDelay, &inertia_stopDelay, inertia_default_startFunction, inertia_default_stopFunction);
			if (result < 1 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_inertia_defaults) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (inertia_startDelay < 0 || inertia_stopDelay < 0)
			{
				// reset it
				inertia_startDelay = -1;
				inertia_stopDelay  = -1;
				memset(inertia_default_startFunction, 0, sizeof(inertia_default_startFunction));
				memset(inertia_default_stopFunction, 0, sizeof(inertia_default_stopFunction));
			}
			continue;
		}

		if (!strncmp("set_node_defaults", line, 17))
		{
			int result = sscanf(line,"set_node_defaults %f, %f, %f, %f, %s", &default_node_loadweight, &default_node_friction, &default_node_volume, &default_node_surface, default_node_options);
			if (result < 1 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_node_defaults) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (default_node_friction < 0)   default_node_friction=NODE_FRICTION_COEF_DEFAULT;
			if (default_node_volume < 0)     default_node_volume=NODE_VOLUME_COEF_DEFAULT;
			if (default_node_surface < 0)    default_node_surface=NODE_SURFACE_COEF_DEFAULT;
			if (default_node_loadweight < 0) default_node_loadweight=NODE_LOADWEIGHT_DEFAULT;
			if (result <= 4) memset(default_node_options, 0, sizeof default_node_options);
			continue;
		}

		if (!strncmp("set_skeleton_settings", line, 21))
		{
			int result = sscanf(line,"set_skeleton_settings %f, %f", &fadeDist, &skeleton_beam_diameter);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_skeleton_settings) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(fadeDist<0)
				fadeDist=150;
			if(skeleton_beam_diameter<0)
				skeleton_beam_diameter=BEAM_SKELETON_DIAMETER;
			continue;
		}

		if (!strcmp("backmesh",line))
		{
			//close the current mesh
			subtexcoords[free_sub]=free_texcoord;
			subcabs[free_sub]=free_cab;
			if(free_sub >= MAX_SUBMESHES)
			{
				LogManager::getSingleton().logMessage("submesh limit reached ("+StringConverter::toString(MAX_SUBMESHES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//make it normal
			subisback[free_sub]=0;
			free_sub++;

			//add an extra front mesh
			int i;
			int start;
			//texcoords
			if (free_sub==1) start=0; else start=subtexcoords[free_sub-2];
			for (i=start; i<subtexcoords[free_sub-1]; i++)
			{
				texcoords[free_texcoord]=texcoords[i];;
				free_texcoord++;
			}
			//cab
			if (free_sub==1) start=0; else start=subcabs[free_sub-2];
			for (i=start; i<subcabs[free_sub-1]; i++)
			{
				cabs[free_cab*3]=cabs[i*3];
				cabs[free_cab*3+1]=cabs[i*3+1];
				cabs[free_cab*3+2]=cabs[i*3+2];
				free_cab++;
			}
			//finish it, this is a window
			subisback[free_sub]=2;
			//close the current mesh
			subtexcoords[free_sub]=free_texcoord;
			subcabs[free_sub]=free_cab;
			//make is transparent
			free_sub++;


			//add an extra back mesh
			//texcoords
			if (free_sub==1) start=0; else start=subtexcoords[free_sub-2];
			for (i=start; i<subtexcoords[free_sub-1]; i++)
			{
				texcoords[free_texcoord]=texcoords[i];;
				free_texcoord++;
			}
			//cab
			if (free_sub==1) start=0; else start=subcabs[free_sub-2];
			for (i=start; i<subcabs[free_sub-1]; i++)
			{
				cabs[free_cab*3]=cabs[i*3+1];
				cabs[free_cab*3+1]=cabs[i*3];
				cabs[free_cab*3+2]=cabs[i*3+2];
				free_cab++;
			}
			//we don't finish, there will be a submesh statement later
			subisback[free_sub]=1;
			continue;
		};
		if (!strcmp("submesh",line))
		{
			subtexcoords[free_sub]=free_texcoord;
			subcabs[free_sub]=free_cab;
			if(free_sub >= MAX_SUBMESHES)
			{
				LogManager::getSingleton().logMessage("submesh limit reached ("+StringConverter::toString(MAX_SUBMESHES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			free_sub++;
			//initialize the next
			subisback[free_sub]=0;
			continue;
		};

		if (!strncmp("set_collision_range", line, 21))
		{
			int result = sscanf(line,"set_collision_range %f", &collrange);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_collision_range) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(collrange<0)
				collrange=DEFAULT_COLLISION_RANGE;
			continue;
		};

		if (mode==1)
		{
			//parse nodes
			int id=0;
			float x=0, y=0, z=0, mass=0;
			char options[255] = "n";
			int result = sscanf(line,"%i, %f, %f, %f, %s %f",&id,&x,&y,&z,options, &mass);
			// catch some errors
			if (result < 4 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				//LogManager::getSingleton().logMessage(strerror(errno));
				continue;
			}
			if (id != free_node)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Node) " + String(fname) +" line " + StringConverter::toString(linecounter) + ":");
				LogManager::getSingleton().logMessage("Error: lost sync in nodes numbers after node " + StringConverter::toString(free_node) + "(got " + StringConverter::toString(id) + " instead)");
				exit(2);
			};

			if(free_node >= MAX_NODES)
			{
				LogManager::getSingleton().logMessage("nodes limit reached ("+StringConverter::toString(MAX_NODES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			Vector3 npos = Vector3(px, py, pz) + rot * Vector3(x,y,z);
			init_node(id, npos.x, npos.y, npos.z, NODE_NORMAL, 10, 0, 0, free_node, -1, default_node_friction, default_node_volume, default_node_surface, default_node_loadweight);
			nodes[id].iIsSkin=true;

			if 	(default_node_loadweight >= 0.0f)
			{
				nodes[id].masstype=NODE_LOADED;
				nodes[id].overrideMass=true;
				nodes[id].mass=default_node_loadweight;
			}

			// merge options and default_node_options
			strncpy(options, ((String(default_node_options) + String(options)).c_str()), 250);

			// now 'parse' the options
			char *options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'l':	// load node
						if(mass != 0)
						{
							nodes[id].masstype=NODE_LOADED;
							nodes[id].overrideMass=true;
							nodes[id].mass=mass;
						}
						else
						{
							nodes[id].masstype=NODE_LOADED;
							masscount++;
						}
						break;
					case 'x':	//exhaust
						if (disable_smoke)
							break;
						if(smokeId == 0 && smokeRef != 0)
						{
							exhaust_t e;
							e.emitterNode = id;
							e.directionNode = smokeRef;
							e.isOldFormat = true;
							//smokeId=id;
							e.smokeNode = parent->createChildSceneNode();
							//ParticleSystemManager *pSysM=ParticleSystemManager::getSingletonPtr();
							char wname[256];
							sprintf(wname, "exhaust-%d-%s", exhausts.size(), truckname);
							//if (pSysM) smoker=pSysM->createSystem(wname, "tracks/Smoke");
							e.smoker=manager->createParticleSystem(wname, "tracks/Smoke");
							// ParticleSystem* pSys = ParticleSystemManager::getSingleton().createSystem("exhaust", "tracks/Smoke");
							if (!e.smoker)
								continue;
							e.smokeNode->attachObject(e.smoker);
							e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
							exhausts.push_back(e);

							nodes[smokeId].isHot=true;
							nodes[id].isHot=true;
						}
						smokeId = id;
						break;
					case 'y':	//exhaust reference
						if (disable_smoke)
							break;
						if(smokeId != 0 && smokeRef == 0)
						{
							exhaust_t e;
							e.emitterNode = smokeId;
							e.directionNode = id;
							e.isOldFormat = true;
							//smokeId=id;
							e.smokeNode = parent->createChildSceneNode();
							//ParticleSystemManager *pSysM=ParticleSystemManager::getSingletonPtr();
							char wname[256];
							sprintf(wname, "exhaust-%d-%s", exhausts.size(), truckname);
							//if (pSysM) smoker=pSysM->createSystem(wname, "tracks/Smoke");
							e.smoker=manager->createParticleSystem(wname, "tracks/Smoke");
							// ParticleSystem* pSys = ParticleSystemManager::getSingleton().createSystem("exhaust", "tracks/Smoke");
							if (!e.smoker)
								continue;
							e.smokeNode->attachObject(e.smoker);
							e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
							exhausts.push_back(e);

							nodes[smokeId].isHot=true;
							nodes[id].isHot=true;
						}
						smokeRef = id;
						break;
					case 'c':	//contactless
						nodes[id].contactless = 1;
						break;
					case 'h':	//hook
						// emulate the old behaviour using new fancy hookgroups
						hook_t h;
						h.hookNode=&nodes[id];
						h.locked=UNLOCKED;
						h.lockNode=0;
						h.lockTruck=0;
						h.lockNodes=true;
						h.group=-1;
						hooks.push_back(h);
						break;
					case 'e':	//editor
						if (!networking)
							editorId=id;
						break;
					case 'b':	//buoy
						nodes[id].buoyancy=10000.0f;
						break;
					case 'p':	//diasble particles
						nodes[id].disable_particles=true;
					break;
					case 'L':	//Log data:
						LogManager::getSingleton().logMessage("Node " + StringConverter::toString(id) + "  settings. Node load mass: " + StringConverter::toString(nodes[id].mass) + ", friction coefficient: " + StringConverter::toString(default_node_friction) + " and buoyancy volume coefficient: " + StringConverter::toString(default_node_volume) + " Fluid drag surface coefficient: " + StringConverter::toString(default_node_surface)+ " Particle mode: " + StringConverter::toString(nodes[id].disable_particles));
					break;
				}
				options_pointer++;
			}
			free_node++;
		}
		else if (mode==2)
		{
			//parse beams
			int id1, id2;
			char options[50] = "v";
			int type=BEAM_NORMAL;
			int result = sscanf(line,"%i, %i, %s",&id1,&id2,options);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Beam) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in beams section ("
					+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(3);
			};

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			//skip if a beam already exists
			int i;
			for (i=0; i<free_beam; i++)
			{
				if ((beams[i].p1==&nodes[id1] && beams[i].p2==&nodes[id2]) || (beams[i].p1==&nodes[id2] && beams[i].p2==&nodes[id1]))
				{
					LogManager::getSingleton().logMessage("Skipping duplicate beams: from node "+StringConverter::toString(id1)+" to node "+StringConverter::toString(id2));
					continue;
				}
			}

			// FIXME: separate init_beam and setup_beam to be able to set all parameters after creation
			// this is just ugly:
			char *options_pointer = options;
			while (*options_pointer != 0) {
				if(*options_pointer=='i') {
					type=BEAM_INVISIBLE;
					break;
				}
				options_pointer++;
			}

			float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
			/*
			if(beam_length < 0.01f)
			{
				LogManager::getSingleton().logMessage("Error: beam "+StringConverter::toString(free_beam)+" is too short ("+StringConverter::toString(beam_length)+"m)");
				LogManager::getSingleton().logMessage("Error: beam "+StringConverter::toString(free_beam)+" is between node "+StringConverter::toString(id1)+" and node "+StringConverter::toString(id2)+".");
				//LogManager::getSingleton().logMessage("will ignore this beam.");
				exit(8);
			}
			*/

			int pos=add_beam(&nodes[id1], &nodes[id2], manager, \
					  parent, type, default_break, default_spring, \
					  default_damp, -1, -1, -1, 1, \
					  default_beam_diameter);

			// now 'parse' the options
			options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'i':	// invisible
						beams[pos].type=BEAM_INVISIBLE;
						break;
					case 'v':	// visible
						beams[pos].type=BEAM_NORMAL;
						break;
					case 'r':
						beams[pos].bounded=ROPE;
						break;
					case 's':
						beams[pos].bounded=SUPPORTBEAM;
						break;
				}
				options_pointer++;
			}
		}
		else if (mode==4)
		{
			//parse shocks
			int id1, id2;
			float s, d, sbound,lbound,precomp;
			char options[50]="n";
			int result = sscanf(line,"%i, %i, %f, %f, %f, %f, %f, %s", &id1, &id2, &s, &d, &sbound, &lbound, &precomp, options);
			if (result < 7 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Shock) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			// checks ...
			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_shock >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("shock limit reached ("+StringConverter::toString(MAX_SHOCKS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in shocks section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(4);
			}


			// options
			int htype=BEAM_HYDRO;
			int shockflag = SHOCK_FLAG_NORMAL;

			// now 'parse' the options
			char *options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'i':	// invisible
						htype=BEAM_INVISIBLE_HYDRO;
						shockflag |= SHOCK_FLAG_INVISIBLE;
						break;
					case 'l':
					case 'L':
						shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
						shockflag |= SHOCK_FLAG_LACTIVE;
						free_active_shock++; // this has no array associated with it. its just to determine if there are active shocks!
						break;
					case 'r':
					case 'R':
						shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
						shockflag |= SHOCK_FLAG_RACTIVE;
						free_active_shock++; // this has no array associated with it. its just to determine if there are active shocks!
						break;
					case 'm':
						{
							// metric values: calculate sbound and lbound now
							float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
							sbound = sbound / beam_length;
							lbound = lbound / beam_length;
						}
						break;
				}
				options_pointer++;
			}
			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break*4.0, s, d, -1.0, sbound, lbound, precomp);
			beams[pos].shock = &shocks[free_shock];
			shocks[free_shock].beamid = pos;
			shocks[free_shock].flags = shockflag;
			free_shock++;
		}
		else if (mode==56)
		{
			//parse shocks2
			int id1, id2;
			float sin=-1.0f,din=-1.0f,psin=-1.0f,pdin=-1.0f,sout=-1.0f,dout=-1.0f,psout=-1.0f,pdout=-1.0f,sbound=-1.0f,lbound=-1.0f,precomp=-1.0f;
			char options[50]="n";
			int result = sscanf(line,"%i, %i, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %s", &id1, &id2, &sin, &din, &psin, &pdin, &sout, &dout, &psout, &pdout, &sbound, &lbound, &precomp, options);

			if (result < 13 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Shock) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			// checks ...
			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_shock >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("shock limit reached ("+StringConverter::toString(MAX_SHOCKS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in shocks2 section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(4);
			}

			if ( sin == -1.0f || din == -1.0f || psin == -1.0f || pdin == -1.0f || sout == -1.0f || dout == -1.0f || psout == -1.0f || pdout == -1.0f || sbound == -1.0f || lbound == -1.0f || precomp == -1.0f)
			{
				LogManager::getSingleton().logMessage("Error: Wrong values in shocks2 section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(123);
			}

			// options
			int htype=BEAM_HYDRO;
			int shockflag = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISSHOCK2;

			// now 'parse' the options
			char *options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'i':
						// invisible
						htype=BEAM_INVISIBLE_HYDRO;
						shockflag |= SHOCK_FLAG_INVISIBLE;
						break;
					case 's':
						// passive shock
						shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
						shockflag |= SHOCK_FLAG_SOFTBUMP;
						break;
					case 'm':
						{
							// metric values: calculate sbound and lbound now
							float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
							sbound = sbound / beam_length;
							lbound = lbound / beam_length;
						}
					case 'M':
						{
							// metric values: calculate sbound and lbound now
							float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
							sbound = (beam_length - sbound) / beam_length;
							lbound = (lbound - beam_length) / beam_length;

							if (lbound < 0)
							{
								LogManager::getSingleton().logMessage("Error parsing File (Shocks) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Metric shock length calculation failed, longbound less then beams spawn length, reset to beams spawn length (longbound=0).");
								lbound = 0.0f;
							}

							if (sbound > 1)
							{
								LogManager::getSingleton().logMessage("Error parsing File (Shocks) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Metric shock length calculation failed, shortbound less then 0 meters, reset to 0 meters (shortbound=1).");
								sbound = 1.0f;
							}
						}
						break;
				}
				options_pointer++;
			}
			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break*4.0, sin, din, -1.0, sbound, lbound, precomp);
			beams[pos].bounded=SHOCK2;
			beams[pos].shock = &shocks[free_shock];
			shocks[free_shock].springin = sin;
			shocks[free_shock].dampin = din;
			shocks[free_shock].sprogin = psin;
			shocks[free_shock].dprogin = pdin;
			shocks[free_shock].springout = sout;
			shocks[free_shock].dampout = dout;
			shocks[free_shock].sprogout = psout;
			shocks[free_shock].dprogout = pdout;
			shocks[free_shock].beamid = pos;
			shocks[free_shock].flags = shockflag;
			free_shock++;
		}

		else if (mode==3)
		{
			//parse fixes
			int id;
			int result = sscanf(line,"%i",&id);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Fixes) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (id>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in fixes section ("
					+StringConverter::toString(id)+")");
				exit(5);
			};
			nodes[id].locked=1;
			hasfixes=1;
		}
		else if (mode==5)
		{
			//parse hydros
			int id1, id2;
			float ratio;
			char options[50] = "n";
			Real startDelay=0;
			Real stopDelay=0;
			char startFunction[50]="";
			char stopFunction[50]="";

			int result = sscanf(line,"%i, %i, %f, %s %f, %f, %s %s",&id1,&id2,&ratio,options,&startDelay,&stopDelay,startFunction,stopFunction);
			if (result < 3 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Hydro) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			int htype=BEAM_HYDRO;

			// FIXME: separate init_beam and setup_beam to be able to set all parameters after creation
			// this is just ugly:
			char *options_pointer = options;
			while (*options_pointer != 0)
			{
				if(*options_pointer=='i')
				{
					htype=BEAM_INVISIBLE_HYDRO;
					break;
				}
				options_pointer++;
			}

			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in hydros section ("
					+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(6);
			};

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_hydro >= MAX_HYDROS)
			{
				LogManager::getSingleton().logMessage("hydros limit reached ("+StringConverter::toString(MAX_HYDROS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//            printf("beam : %i %i\n", id1, id2);

			if(hydroInertia && startDelay != 0 && stopDelay != 0)
				hydroInertia->setCmdKeyDelay(free_hydro,startDelay,stopDelay,String (startFunction), String (stopFunction));
			else if(hydroInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
				hydroInertia->setCmdKeyDelay(free_hydro,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));


			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break, default_spring, default_damp);
			hydro[free_hydro]=pos;free_hydro++;
			beams[pos].Lhydro=beams[pos].L;
			beams[pos].hydroRatio=ratio;
			beams[pos].animOption = 0.0f;


			// now 'parse' the options
			options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'i':	// invisible
						beams[pos].type = BEAM_INVISIBLE_HYDRO;
						break;
					case 'n':	// normal
						beams[pos].type = BEAM_HYDRO;
						beams[pos].hydroFlags |= HYDRO_FLAG_DIR;
						break;
					case 's': // speed changing hydro
						beams[pos].hydroFlags |= HYDRO_FLAG_SPEED;
						break;
					case 'a':
						beams[pos].hydroFlags |= HYDRO_FLAG_AILERON;
						break;
					case 'r':
						beams[pos].hydroFlags |= HYDRO_FLAG_RUDDER;
						break;
					case 'e':
						beams[pos].hydroFlags |= HYDRO_FLAG_ELEVATOR;
						break;
					case 'u':
						beams[pos].hydroFlags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_ELEVATOR);
						break;
					case 'v':
						beams[pos].hydroFlags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_ELEVATOR);
						break;
					case 'x':
						beams[pos].hydroFlags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_RUDDER);
						break;
					case 'y':
						beams[pos].hydroFlags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_RUDDER);
						break;
					case 'g':
						beams[pos].hydroFlags |= (HYDRO_FLAG_ELEVATOR | HYDRO_FLAG_RUDDER);
						break;
					case 'h':
						beams[pos].hydroFlags |= (HYDRO_FLAG_REV_ELEVATOR | HYDRO_FLAG_RUDDER);
						break;
				}
				options_pointer++;
				// if you use the i flag on its own, add the direction to it
				if(beams[pos].type == BEAM_INVISIBLE_HYDRO && !beams[pos].hydroFlags)
					beams[pos].hydroFlags |= HYDRO_FLAG_DIR;
			}
		}
		else if (mode==66)
		{
			//parse animators
			int id1, id2;
			float ratio;
			Ogre::vector<Ogre::String>::type options = Ogre::StringUtil::split(String(line), ",");
			if (options.size() < 4)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Animator) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			// the required values
			id1 = StringConverter::parseInt(options[0]);
			id2 = StringConverter::parseInt(options[1]);
			ratio = StringConverter::parseReal(options[2]);
			String optionStr = options[3];
			Ogre::vector<Ogre::String>::type optionArgs = Ogre::StringUtil::split(optionStr, "|");

			int htype=BEAM_HYDRO;

			// detect invisible beams
			for(unsigned int i=0;i<optionArgs.size();i++)
			{
				String arg = optionArgs[i];
				StringUtil::trim(arg);
				if(arg == "inv")
					htype=BEAM_INVISIBLE_HYDRO;
			}

			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in animators section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(6);
			}

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_hydro >= MAX_HYDROS)
			{
				LogManager::getSingleton().logMessage("hydros limit reached (via animators) ("+StringConverter::toString(MAX_HYDROS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(hydroInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
				hydroInertia->setCmdKeyDelay(free_hydro,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));

			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break, default_spring, default_damp);
			hydro[free_hydro]=pos;
			free_hydro++;
			beams[pos].Lhydro=beams[pos].L;
			beams[pos].hydroRatio=ratio;

			// parse the rest
			for(unsigned int i=0;i<optionArgs.size();i++)
			{
				String arg = optionArgs[i];
				StringUtil::trim(arg);
				beam_t *beam = &beams[pos];
				if      (arg == "vis")            { beam->type = BEAM_HYDRO; }
				else if (arg == "inv")            { htype=BEAM_INVISIBLE_HYDRO; }
				else if (arg == "airspeed")       { beam->animFlags |= (ANIM_FLAG_AIRSPEED); }
				else if (arg == "vvi")            { beam->animFlags |= (ANIM_FLAG_VVI); }
				else if (arg == "altimeter100k")  { beam->animFlags |= (ANIM_FLAG_ALTIMETER); beam->animOption = 1.0f; }
				else if (arg == "altimeter10k")   { beam->animFlags |= (ANIM_FLAG_ALTIMETER); beam->animOption = 2.0f; }
				else if (arg == "altimeter1k")    { beam->animFlags |= (ANIM_FLAG_ALTIMETER); beam->animOption = 3.0f; }
				else if (arg == "aoa")            { beam->animFlags |= (ANIM_FLAG_AOA); }
				else if (arg == "flap")           { beam->animFlags |= (ANIM_FLAG_FLAP); }
				else if (arg == "airbrake")       { beam->animFlags |= (ANIM_FLAG_AIRBRAKE); }
				else if (arg == "roll")           { beam->animFlags |= (ANIM_FLAG_ROLL); }
				else if (arg == "pitch")          { beam->animFlags |= (ANIM_FLAG_PITCH); }
				else if (arg == "throttle1")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 1.0f; }
				else if (arg == "throttle2")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 2.0f; }
				else if (arg == "throttle3")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 3.0f; }
				else if (arg == "throttle4")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 4.0f; }
				else if (arg == "throttle5")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 5.0f; }
				else if (arg == "throttle6")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 6.0f; }
				else if (arg == "throttle7")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 7.0f; }
				else if (arg == "throttle8")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 8.0f; }
				else if (arg == "rpm1")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 1.0f; }
				else if (arg == "rpm2")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 2.0f; }
				else if (arg == "rpm3")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 3.0f; }
				else if (arg == "rpm4")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 4.0f; }
				else if (arg == "rpm5")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 5.0f; }
				else if (arg == "rpm6")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 6.0f; }
				else if (arg == "rpm7")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 7.0f; }
				else if (arg == "rpm8")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 8.0f; }
				else if (arg == "aerotorq1")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 1.0f; }
				else if (arg == "aerotorq2")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 2.0f; }
				else if (arg == "aerotorq3")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 3.0f; }
				else if (arg == "aerotorq4")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 4.0f; }
				else if (arg == "aerotorq5")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 5.0f; }
				else if (arg == "aerotorq6")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 6.0f; }
				else if (arg == "aerotorq7")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 7.0f; }
				else if (arg == "aerotorq8")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 8.0f; }
				else if (arg == "aeropit1")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 1.0f; }
				else if (arg == "aeropit2")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 2.0f; }
				else if (arg == "aeropit3")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 3.0f; }
				else if (arg == "aeropit4")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 4.0f; }
				else if (arg == "aeropit5")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 5.0f; }
				else if (arg == "aeropit6")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 6.0f; }
				else if (arg == "aeropit7")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 7.0f; }
				else if (arg == "aeropit8")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 8.0f; }
				else if (arg == "aerostatus1")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 1.0f; }
				else if (arg == "aerostatus2")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 2.0f; }
				else if (arg == "aerostatus3")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 3.0f; }
				else if (arg == "aerostatus4")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 4.0f; }
				else if (arg == "aerostatus5")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 5.0f; }
				else if (arg == "aerostatus6")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 6.0f; }
				else if (arg == "aerostatus7")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 7.0f; }
				else if (arg == "aerostatus8")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 8.0f; }
				else if (arg == "brakes")         { beam->animFlags |= (ANIM_FLAG_BRAKE); }
				else if (arg == "accel")          { beam->animFlags |= (ANIM_FLAG_ACCEL); }
				else if (arg == "clutch")         { beam->animFlags |= (ANIM_FLAG_CLUTCH); }
				else if (arg == "speedo")         { beam->animFlags |= (ANIM_FLAG_SPEEDO); }
				else if (arg == "tacho")          { beam->animFlags |= (ANIM_FLAG_TACHO); }
				else if (arg == "turbo")          { beam->animFlags |= (ANIM_FLAG_TURBO); }
				else if (arg == "parking")        { beam->animFlags |= (ANIM_FLAG_PBRAKE); }
				else if (arg == "shifterman1")    { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 1.0f; }
				else if (arg == "shifterman2")    { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 2.0f; }
				else if (arg == "seqential")      { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 3.0f; }
				else if (arg == "shifterlin")     { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 4.0f; }
				else if (arg == "torque")         { beam->animFlags |= (ANIM_FLAG_TORQUE); }
				else if (arg == "throttleboat")   { beam->animFlags |= (ANIM_FLAG_BTHROTTLE); }
				else if (arg == "rudderboat")     { beam->animFlags |= (ANIM_FLAG_BRUDDER); }

				if (beam->animFlags == 0)
					LogManager::getSingleton().logMessage("Error parsing File (animators) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Failed to identify source.");
				else
					LogManager::getSingleton().logMessage("Animator source set: with flag " +StringConverter::toString(beams[pos].animFlags));
			}

		}

		else if (mode==6)
		{
			//parse wheels
			float radius, width, mass, spring, damp;
			char texf[256];
			char texb[256];
			int rays, node1, node2, snode, braked, propulsed, torquenode;
			int result = sscanf(line,"%f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %s %s",&radius,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp, texf, texb);
			if (result < 14 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Wheel) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			addWheel(manager, parent, radius,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, texf, texb);
		}
		else if (mode==31)
		{
			//parse wheels2
			char texf[256];
			char texb[256];
			float radius, radius2, width, mass, spring, damp, spring2, damp2;
			int rays, node1, node2, snode, braked, propulsed, torquenode;
			int result = sscanf(line,"%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %f, %f, %s %s",&radius,&radius2,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp,&spring2,&damp2, texf, texb);
			if (result < 17 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Wheel2) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (enable_wheel2)
				addWheel2(manager, parent, radius,radius2,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, spring2, damp2, texf, texb);
			else
				addWheel(manager, parent, radius2,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring2, damp2, texf, texb);
		}
		else if (mode==42)
		{
			//parse meshwheels
			char meshw[256];
			char texb[256];
			float radius, rimradius, width, mass, spring, damp;
			char side;
			int rays, node1, node2, snode, braked, propulsed, torquenode;
			int result = sscanf(line,"%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %c, %s %s",&radius,&rimradius,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp, &side, meshw, texb);
			if (result < 16 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (MeshWheel) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			addWheel(manager, parent, radius,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, meshw, texb, true, rimradius, side!='r');
		}
		else if (mode==7)
		{
			//parse globals
			int result = sscanf(line,"%f, %f, %s",&truckmass, &loadmass, texname);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Globals) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//
			LogManager::getSingleton().logMessage("BEAM: line: '"+String(line)+"'");
			LogManager::getSingleton().logMessage("BEAM: texname: '"+String(texname)+"'");


			// check for skins
			if(!usedSkin.isNull() && usedSkin->hasReplacementForMaterial(texname))
			{
				// yay, we use a skin :D
				strncpy(texname, usedSkin->getReplacementForMaterial(texname).c_str(), 1024);
			}

			//we clone the material
			char clonetex[256];
			sprintf(clonetex, "%s-%s",texname,truckname);
			MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().getByName(texname));
			if(mat.getPointer() == 0)
			{
				LogManager::getSingleton().logMessage("Material '" + String(texname) + "' used in Section 'globals' not found! We will try to use the material 'tracks/black' instead." + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				mat=(MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/black"));
				if(mat.getPointer() == 0)
				{
					LogManager::getSingleton().logMessage("Material not found! Try to ensure that tracks/black exists and retry.");
					exit(124);
				}
			}
			mat->clone(clonetex);
			strcpy(texname, clonetex);
		}
		else if (mode==8)
		{
			//parse cameras
			int nodepos, nodedir, dir;
			int result = sscanf(line,"%i, %i, %i",&nodepos,&nodedir,&dir);
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Camera) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			addCamera(nodepos, nodedir, dir);
		}
		else if (mode==9)
		{
			//parse engine
			float minrpm, maxrpm, torque, dratio, rear;
			float gears[16];
			int numgears;
			if(driveable == MACHINE)
				continue;

			driveable=TRUCK;
			int result = sscanf(line,"%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", &minrpm, &maxrpm, &torque, &dratio, &rear, &gears[0],&gears[1],&gears[2],&gears[3],&gears[4],&gears[5],&gears[6],&gears[7],&gears[8],&gears[9],&gears[10],&gears[11],&gears[12],&gears[13],&gears[14],&gears[15]);
			if (result < 7 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Engine) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			for (numgears=0; numgears<16; numgears++)
				if (gears[numgears]<=0)
					break;
			if (numgears < 3)
			{
				LogManager::getSingleton().logMessage("Trucks with less than 3 gears are not supported! " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//if (audio) audio->setupEngine();

			engine = new BeamEngine(minrpm, maxrpm, torque, rear, numgears - 1, gears, dratio, trucknum);
			//engine->start();
		}

		else if (mode==10)
		{
			//parse texcoords
			int id;
			float x, y;
			int result = sscanf(line,"%i, %f, %f", &id, &x, &y);
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (TexCoords) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_texcoord >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("texcoords limit reached ("+StringConverter::toString(MAX_TEXCOORDS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			texcoords[free_texcoord]=Vector3(id, x, y);
			free_texcoord++;
		}

		else if (mode==11)
		{
			//parse cab
			char type='n';
			int id1, id2, id3;
			int result = sscanf(line,"%i, %i, %i, %c", &id1, &id2, &id3,&type);
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Cab) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_cab >= MAX_CABS)
			{
				LogManager::getSingleton().logMessage("cabs limit reached ("+StringConverter::toString(MAX_CABS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			cabs[free_cab*3]=id1;
			cabs[free_cab*3+1]=id2;
			cabs[free_cab*3+2]=id3;
			if(free_collcab >= MAX_CABS)
			{
				LogManager::getSingleton().logMessage("unable to create cabs: cabs limit reached ("+StringConverter::toString(MAX_CABS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (type=='c') {collcabs[free_collcab]=free_cab; collcabstype[free_collcab]=0; free_collcab++;};
			if (type=='p') {collcabs[free_collcab]=free_cab; collcabstype[free_collcab]=1; free_collcab++;};
			if (type=='u') {collcabs[free_collcab]=free_cab; collcabstype[free_collcab]=2; free_collcab++;};
			if (type=='b') {buoycabs[free_buoycab]=free_cab; collcabstype[free_collcab]=0; buoycabtypes[free_buoycab]=BUOY_NORMAL; free_buoycab++;   if (!buoyance) buoyance=new Buoyance(water);};
			if (type=='r') {buoycabs[free_buoycab]=free_cab; collcabstype[free_collcab]=0; buoycabtypes[free_buoycab]=BUOY_DRAGONLY; free_buoycab++; if (!buoyance) buoyance=new Buoyance(water);};
			if (type=='s') {buoycabs[free_buoycab]=free_cab; collcabstype[free_collcab]=0; buoycabtypes[free_buoycab]=BUOY_DRAGLESS; free_buoycab++; if (!buoyance) buoyance=new Buoyance(water);};
			if (type=='D' || type == 'F' || type == 'S')
			{

				if(free_collcab >= MAX_CABS || free_buoycab >= MAX_CABS)
				{
					LogManager::getSingleton().logMessage("unable to create buoycabs: cabs limit reached ("+StringConverter::toString(MAX_CABS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				collcabs[free_collcab]=free_cab;
				collcabstype[free_collcab]=0;
				if(type == 'F') collcabstype[free_collcab]=1;
				if(type == 'S') collcabstype[free_collcab]=2;
				free_collcab++;
				buoycabs[free_buoycab]=free_cab; buoycabtypes[free_buoycab]=BUOY_NORMAL; free_buoycab++; if (!buoyance) buoyance=new Buoyance(water);
			}
			free_cab++;
		}

		else if (mode==12 || mode==120)
		{
			//parse commands
			int id1, id2,keys,keyl;
			float rateShort, rateLong, shortl, longl;
			char options[250]="";
			char descr[200] = "";
			hascommands=1;
			Real startDelay=0;
			Real stopDelay=0;
			char startFunction[50]="";
			char stopFunction[50]="";
			int result = 0;
			if(mode == 12)
			{
				char opt='n';
				result = sscanf(line,"%i, %i, %f, %f, %f, %i, %i, %c, %s %f, %f, %s %s", &id1, &id2, &rateShort, &shortl, &longl, &keys, &keyl, &opt, descr, &startDelay, &stopDelay, startFunction, stopFunction);
				if (result < 7 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Command) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				options[0] = opt;
				options[1] = 0;
				rateLong = rateShort;
			}
			else if(mode == 120)
			{
				result = sscanf(line,"%i, %i, %f, %f, %f, %f, %i, %i, %s %s %f,%f,%s %s", &id1, &id2, &rateShort, &rateLong, &shortl, &longl, &keys, &keyl, options, descr, &startDelay, &stopDelay, startFunction, stopFunction);
				if (result < 8 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Command) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//LogManager::getSingleton().logMessage("added command2: " + String(options)+ ", " + String(descr));
				//LogManager::getSingleton().logMessage(String(line));
			}

			int htype=BEAM_HYDRO;

			char *options_pointer = options;
			while (*options_pointer != 0) {
				if(*options_pointer=='i') {
					htype=BEAM_INVISIBLE_HYDRO;
					break;
				}
				options_pointer++;
			}

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("cannot create command: beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break, default_spring, default_damp, -1, -1, -1, 1, default_beam_diameter);
			// now 'parse' the options
			options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'r':
						beams[pos].bounded=ROPE;
						break;
					case 'c':
						if(beams[pos].isOnePressMode>0)
						{
							LogManager::getSingleton().logMessage("Command cannot be one-pressed and self centering at the same time!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						beams[pos].iscentering=true;
						break;
					case 'p':
						if(beams[pos].iscentering)
						{
							LogManager::getSingleton().logMessage("Command cannot be one-pressed and self centering at the same time!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						if(beams[pos].isOnePressMode>0)
						{
							LogManager::getSingleton().logMessage("Command already has a one-pressed mode! All after the first are ignored!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						beams[pos].isOnePressMode=1;
						break;
					case 'o':
						if(beams[pos].iscentering)
						{
							LogManager::getSingleton().logMessage("Command cannot be one-pressed and self centering at the same time!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						if(beams[pos].isOnePressMode>0)
						{
							LogManager::getSingleton().logMessage("Command already has a one-pressed mode! All after the first are ignored!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						beams[pos].isOnePressMode=2;
						break;
					case 'f':
						beams[pos].isforcerestricted=true;
						break;
				}
				options_pointer++;
			}

			beams[pos].Lhydro=beams[pos].L;

			//add short key
			commandkey[keys].beams.push_back(-pos);
			char *descr_pointer = descr;
			//replace '_' with ' '
			while (*descr_pointer!=0) {if (*descr_pointer=='_') *descr_pointer=' ';descr_pointer++;};

			if(strlen(descr) != 0)
				commandkey[keys].description = String(descr);
			else if (strlen(descr) == 0 && commandkey[keys].description.size() == 0)
				commandkey[keys].description = "";

			//add long key
			commandkey[keyl].beams.push_back(pos);
			if(strlen(descr) != 0)
				commandkey[keyl].description = String(descr);
			else if (strlen(descr) == 0 && commandkey[keyl].description.size() == 0)
				commandkey[keyl].description = "";

			LogManager::getSingleton().logMessage("added command: short=" + StringConverter::toString(keys)+ ", long=" + StringConverter::toString(keyl) + ", descr=" + (descr));
			beams[pos].commandRatioShort=rateShort;
			beams[pos].commandRatioLong=rateLong;
			beams[pos].commandShort=shortl;
			beams[pos].commandLong=longl;

			// set the middle of the command, so its not required to recalculate this everytime ...
			if(beams[pos].commandLong > beams[pos].commandShort)
				beams[pos].centerLength = (beams[pos].commandLong-beams[pos].commandShort)/2 + beams[pos].commandShort;
			else
				beams[pos].centerLength = (beams[pos].commandShort-beams[pos].commandLong)/2 + beams[pos].commandLong;

			if(cmdInertia && startDelay > 0 && stopDelay > 0)
			{
				cmdInertia->setCmdKeyDelay(keys,startDelay,stopDelay,String (startFunction),String (stopFunction));
				cmdInertia->setCmdKeyDelay(keyl,startDelay,stopDelay,String (startFunction),String (stopFunction));
			}
			else if(cmdInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
			{
				cmdInertia->setCmdKeyDelay(keys,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
				cmdInertia->setCmdKeyDelay(keyl,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
			}

		}
		else if (mode==13)
		{
			//parse contacters
			int id1;
			int result = sscanf(line,"%i", &id1);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Contacters) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_contacter >= MAX_CONTACTERS)
			{
				LogManager::getSingleton().logMessage("contacters limit reached ("+StringConverter::toString(MAX_CONTACTERS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			contacters[free_contacter].nodeid=id1;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			nodes[id1].iIsSkin=true;
			free_contacter++;;
		}
		else if (mode==14)
		{
			//parse ropes
			int id1=0, id2=0, key=0, group=0;
			int result = sscanf(line,"%i, %i, %i, %i", &id1, &id2, &key, &group);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Ropes) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//add beam
			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in ropes section ("
					+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(7);
			};

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("cannot create rope: beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, BEAM_NORMAL, default_break, default_spring, default_damp);
			beams[pos].bounded=ROPE;

			rope_t r;
			//register rope
			r.beam=&beams[pos];
			r.lockedto=0;
			r.group=group;
			ropes.push_back(r);

			nodes[id1].iIsSkin=true;
			nodes[id2].iIsSkin=true;
		}
		else if (mode==15)
		{
			//parse ropables
			int id1=0, group=-1, multilock=0;
			int result = sscanf(line,"%i, %i, %i", &id1, &group, &multilock);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Ropeable) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (id1>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in ropables section ("+StringConverter::toString(id1)+")");
				exit(7);
			}

			ropable_t r;
			r.node = &nodes[id1];
			r.group = group;
			r.used = 0;
			r.multilock = (bool)(multilock == 1);
			ropables.push_back(r);

			nodes[id1].iIsSkin=true;
		}
		else if (mode==16)
		{
			//parse ties
			int id1=0, group=-1;
			float maxl, rate, shortl, longl, maxstress;
			char option='n';
			maxstress=100000.0f;
			hascommands=1;
			int result = sscanf(line,"%i, %f, %f, %f, %f, %c, %f, %i", &id1, &maxl, &rate, &shortl, &longl, &option, &maxstress, &group);
			if (result < 5 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Ties) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("cannot create tie: beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			int htype=BEAM_HYDRO;
			if (option=='i') htype=BEAM_INVISIBLE_HYDRO;
			int pos=add_beam(&nodes[id1], &nodes[0], manager, parent, htype, default_break, default_spring, default_damp);
			beams[pos].L=maxl;
			beams[pos].refL=maxl;
			beams[pos].Lhydro=maxl;
			beams[pos].bounded=ROPE;
			beams[pos].disabled=true;
			beams[pos].mSceneNode->detachAllObjects();
			beams[pos].commandRatioShort=rate;
			beams[pos].commandRatioLong=rate;
			beams[pos].commandShort=shortl;
			beams[pos].commandLong=longl;
			beams[pos].maxtiestress=maxstress;

			//register tie
			tie_t t;
			t.group = group;
			t.tying=false;
			t.tied=false;
			t.beam=&beams[pos];
			ties.push_back(t);
		}

		else if (mode==17)
		{
			//help material
			strcpy(helpmat,line);
			hashelp=1;
		}
		else if (mode==18)
		{
			//cinecam
			float x,y,z;
			int n1, n2, n3, n4, n5, n6, n7, n8;
			float spring=8000.0;
			float damp=800.0;
			int result = sscanf(line,"%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %i, %f, %f", &x,&y,&z,&n1,&n2,&n3,&n4,&n5,&n6,&n7,&n8, &spring, &damp);
			if (result < 11 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Cinecam) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("cannot create cinecam: beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_node >= MAX_NODES)
			{
				LogManager::getSingleton().logMessage("cannot create cinecam: nodes limit reached ("+StringConverter::toString(MAX_NODES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			//add node
			cinecameranodepos[freecinecamera]=free_node;
			Vector3 npos=Vector3(px, py, pz)+rot*Vector3(x,y,z);
			init_node(cinecameranodepos[freecinecamera], npos.x, npos.y, npos.z);
//				init_node(cinecameranodepos[freecinecamera], px+x*cos(ry)+z*sin(ry), py+y , pz+x*cos(ry+3.14159/2.0)+z*sin(ry+3.14159/2.0));
			nodes[free_node].contactless = 1;
			free_node++;

			//add beams
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n1], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n2], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n3], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n4], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n5], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n6], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n7], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n8], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);

			if(flaresMode>=2 && !cablight)
			{
				// create cabin light :)
				char flarename[256];
				sprintf(flarename, "cabinglight-%s", truckname);
				cablight=manager->createLight(flarename);
				cablight->setType(Light::LT_POINT);
				cablight->setDiffuseColour( ColourValue(0.4, 0.4, 0.3));
				cablight->setSpecularColour( ColourValue(0.4, 0.4, 0.3));
				cablight->setAttenuation(20, 1, 0, 0);
				cablight->setCastShadows(false);
				cablight->setVisible(true);
				cablightNode = manager->getRootSceneNode()->createChildSceneNode();
				deletion_sceneNodes.push_back(cablightNode);
				if(cablight)
					cablightNode->attachObject(cablight);
				cablightNode->setVisible(false);
			}

			freecinecamera++;
		}

		else if (mode==19 || mode==65)
		{
			if(flaresMode==0)
				continue;
			//parse flares
			int ref=-1, nx=0, ny=0, controlnumber=-1, blinkdelay=-2;
			float ox=0, oy=0, oz=1, size=-2;
			char type='f';
			char matname[255]="";
			int result=-1;
			if(mode == 19)
			{
				// original flares
				result = sscanf(line,"%i, %i, %i, %f, %f, %c, %i, %i, %f %s", &ref, &nx, &ny, &ox, &oy, &type, &controlnumber, &blinkdelay, &size, matname);
				if (result < 5 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (Flares) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
			} else if(mode == 65)
			{
				// flares 2
				result = sscanf(line,"%i, %i, %i, %f, %f, %f, %c, %i, %i, %f %s", &ref, &nx, &ny, &ox, &oy, &oz, &type, &controlnumber, &blinkdelay, &size, matname);
				if (result < 5 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (Flares) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}

			}

			// check validity
			// b = brake light
			// f = front light
			// l = left blink light
			// r = right blink light
			// R = reverse light
			// u = user controlled light (i.e. fog light) (use controlnumber later)
			if (type != 'f' && type != 'b' && type != 'l' && type != 'r' && type != 'R' && type != 'u')
				type = 'f';

			// backwards compatibility
			if(blinkdelay == -2 && (type == 'l' || type == 'r'))
				// default blink
				blinkdelay = -1;
			else if(blinkdelay == -2 && !(type == 'l' || type == 'r'))
				//default no blink
				blinkdelay = 0;
			if(size == -2 && type == 'f')
				size = 1;
			else if ((size == -2 && type != 'f') || size == -1)
				size = 0.5;

			//LogManager::getSingleton().logMessage(StringConverter::toString(controlnumber));
			if(controlnumber < -1 || controlnumber > 500)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Flares) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Controlnumber must be between -1 and 500! trying to continue ...");
				continue;
			}
			//LogManager::getSingleton().logMessage(StringConverter::toString(blinkdelay));
			if(blinkdelay < -1 || blinkdelay > 60000)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Flares) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Blinkdelay must be between 0 and 60000! trying to continue ...");
				continue;
			}
			flare_t f;
			f.type = type;
			f.controlnumber = controlnumber;
			f.controltoggle_status = false;
			if(blinkdelay == -1)
				f.blinkdelay = 0.5f;
			else
				f.blinkdelay = (float)blinkdelay / 1000.0f;
			f.blinkdelay_curr=0.0f;
			f.blinkdelay_state=false;
			f.noderef=ref;
			f.nodex=nx;
			f.nodey=ny;
			f.offsetx=ox;
			f.offsety=oy;
			f.offsetz=oz;
			f.size=size;
			f.snode = manager->getRootSceneNode()->createChildSceneNode();
			char flarename[256];
			sprintf(flarename, "flare-%s-%i", truckname, free_flare);
			f.bbs=manager->createBillboardSet(flarename,1);
			f.bbs->createBillboard(0,0,0);
			bool usingDefaultMaterial=true;
			if (f.bbs && (!strncmp(matname, "default", 250) || strnlen(matname, 250) == 0))
			{
				if (type == 'b')
					f.bbs->setMaterialName("tracks/brakeflare");
				else if (type == 'l' || type == 'r')
					f.bbs->setMaterialName("tracks/blinkflare");
				else // (type == 'f' || type == 'R')
					f.bbs->setMaterialName("tracks/flare");
			} else
			{
				usingDefaultMaterial=false;
				if(f.bbs)
					f.bbs->setMaterialName(String(matname));
			}
			if(f.bbs)
				f.snode->attachObject(f.bbs);
			f.isVisible=true;
			f.light=NULL;
			//LogManager::getSingleton().logMessage("Blinkdelay2: " + StringConverter::toString(blinkdelay));
			if (type == 'f' && usingDefaultMaterial && flaresMode >=2 && size > 0.001)
			{
				// front light
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1, 1, 1));
				f.light->setSpecularColour( ColourValue(1, 1, 1));
				f.light->setAttenuation(400, 0.9, 0, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if (type == 'f' && !usingDefaultMaterial && flaresMode >=4 && size > 0.001)
			{
				// this is a quick fix for the red backlight when frontlight is switched on
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1.0, 0, 0));
				f.light->setSpecularColour( ColourValue(1.0, 0, 0));
				f.light->setAttenuation(10.0, 1.0, 0, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if (type == 'R' && flaresMode >= 4 && size > 0.001)
			{
				// brake light
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour(ColourValue(1, 1, 1));
				f.light->setSpecularColour(ColourValue(1, 1, 1));
				f.light->setAttenuation(20.0, 1, 0, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if (type == 'b' && flaresMode >= 4 && size > 0.001)
			{
				// brake light
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1.0, 0, 0));
				f.light->setSpecularColour( ColourValue(1.0, 0, 0));
				f.light->setAttenuation(10.0, 1.0, 0, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if ((type == 'l' || type == 'r') && flaresMode >= 4 && size > 0.001)
			{
				// blink light
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1, 1, 0));
				f.light->setSpecularColour( ColourValue(1, 1, 0));
				f.light->setAttenuation(10.0, 1, 1, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if ((type == 'u') && flaresMode >= 4 && size > 0.001)
			{
				// user light always white (TODO: improve this)
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1, 1, 1));
				f.light->setSpecularColour( ColourValue(1, 1, 1));
				f.light->setAttenuation(50.0, 1.0, 1, 0.2);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}

			// update custom light array
			if(type == 'u' && netCustomLightArray_counter < 4)
			{
				netCustomLightArray[netCustomLightArray_counter] = free_flare;
				netCustomLightArray_counter++;
			}
			flares.push_back(f);
			free_flare++;
		}
		else if (mode==20)
		{
			//parse props
			int ref, nx, ny;
			float ox, oy, oz;
			float rx, ry, rz;
			char meshname[256];
			int result = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname);
			if (result < 10 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Prop) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_prop >= MAX_PROPS)
			{
				LogManager::getSingleton().logMessage("props limit reached ("+StringConverter::toString(MAX_PROPS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			/* Initialize prop memory to avoid invalid pointers. */
			memset(&props[free_prop], 0, sizeof props[free_prop]);
			props[free_prop].noderef=ref;
			props[free_prop].nodex=nx;
			props[free_prop].nodey=ny;
			props[free_prop].offsetx=ox;
			props[free_prop].offsety=oy;
			props[free_prop].offsetz=oz;
			props[free_prop].orgoffsetX=ox;
			props[free_prop].orgoffsetY=oy;
			props[free_prop].orgoffsetZ=oz;
			props[free_prop].rotaX=rx;
			props[free_prop].rotaY=ry;
			props[free_prop].rotaZ=rz;
			props[free_prop].rot=Quaternion(Degree(rz), Vector3::UNIT_Z);
			props[free_prop].rot=props[free_prop].rot*Quaternion(Degree(ry), Vector3::UNIT_Y);
			props[free_prop].rot=props[free_prop].rot*Quaternion(Degree(rx), Vector3::UNIT_X);
			props[free_prop].wheel=0;
			props[free_prop].mirror=0;
			props[free_prop].pale=0;
			props[free_prop].spinner=0;
			props[free_prop].wheelrotdegree=160.0;
			//set no animation by default
			props[free_prop].animFlags[0]=0;
			props[free_prop].animMode[0]=0;
			props[free_prop].animKey[0]=-1;
			props[free_prop].animKeyState[0]=-1.0f;
			String meshnameString = String(meshname);
			std::string::size_type loc = meshnameString.find("leftmirror", 0);
			if( loc != std::string::npos ) props[free_prop].mirror=1;

			loc = meshnameString.find("rightmirror", 0);
			if( loc != std::string::npos ) props[free_prop].mirror=-1;

			loc = meshnameString.find("dashboard", 0);
			if( loc != std::string::npos )
			{
				char dirwheelmeshname[256];
				float dwx=0, dwy=0, dwz=0;
				Real rotdegrees=160;
				Vector3 stdpos = Vector3(-0.67, -0.61,0.24);
				if (!strncmp("dashboard-rh", meshname, 12))
					stdpos=Vector3(0.67, -0.61,0.24);
				String diwmeshname = "dirwheel.mesh";
				int result2 = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s %s %f, %f, %f, %f", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname, dirwheelmeshname, &dwx, &dwy, &dwz, &rotdegrees);
				if(result2 != result && result2 >= 14)
				{
					stdpos = Vector3(dwx, dwy, dwz);
					diwmeshname = String(dirwheelmeshname);
				}
				if(result2 != result && result2 >= 15) props[free_prop].wheelrotdegree=rotdegrees;
				//create a wheel
				char propname[256];
				sprintf(propname, "prop-%s-%i-wheel", truckname, free_prop);
				Entity *te=0;
				try
				{
					te = manager->createEntity(propname, diwmeshname);
				}catch(...)
				{
					LogManager::getSingleton().logMessage("error loading mesh: "+diwmeshname);
					continue;
				}
				MaterialFunctionMapper::replaceSimpleMeshMaterials(te, ColourValue(0, 0.5, 0.5));

				if(materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(te);
				if(!usedSkin.isNull()) usedSkin->replaceMeshMaterials(te);
				props[free_prop].wheel=manager->getRootSceneNode()->createChildSceneNode();
				if(te)
					props[free_prop].wheel->attachObject(te);
				props[free_prop].wheelpos=stdpos;
			}
			char propname[256];
			sprintf(propname, "prop-%s-%i", truckname, free_prop);
			Entity *te=0;
			try
			{
				// LODs for props
#if 0
				// DISABLED FOR OGRE 1.7 
				// XXX: TODO: FIX!
				if(enable_truck_lod)
				{
					String meshGroup = ResourceGroupManager::getSingleton().findGroupContainingResource(meshname);
					MeshPtr mesh = MeshManager::getSingleton().load(meshname, meshGroup);

					// find vertex count
					unsigned long vertex_count = 0;
					if(mesh->getNumLodLevels() < 2)
					{
						if (mesh->sharedVertexData)
							vertex_count += mesh->sharedVertexData->vertexCount;
						for (int i=0; i<mesh->getNumSubMeshes(); i++)
							if (!mesh->getSubMesh(i)->useSharedVertices)
								vertex_count += mesh->getSubMesh(i)->vertexData->vertexCount;
					}

					// check against some border
					if(mesh->getNumLodLevels() < 2 && vertex_count > 10000)
					{
						LogManager::getSingleton().logMessage("prop uses > 10k (" + StringConverter::toString(vertex_count) + ") vertices but does not provide its own LODs, will generate some now. This can take a while. Please add LODs when exporting the mesh, this prevents the automatic generation (and the waiting time).");

						Ogre::Mesh::LodDistanceList default_dists;
						//default_dists.push_back(50);
						//default_dists.push_back(100);
						default_dists.push_back(300);
						mesh->generateLodLevels(default_dists, ProgressiveMesh::VRQ_PROPORTIONAL, Ogre::Real(0.8));

						// custom entities for custom LODs
						te = manager->createEntity(String(propname)+"LOD", mesh->getName());
					} else
					{
						// no custom LOD's here
						te = manager->createEntity(propname, meshname);
					}
				} else
				{
						// no LOD's
						te = manager->createEntity(propname, meshname);
				}
				LogManager::getSingleton().logMessage("prop is using LODs");
#endif //0
				// no LOD's
				te = manager->createEntity(propname, meshname);
			}catch(...)
			{
				LogManager::getSingleton().logMessage("error loading mesh: "+String(meshname));
				continue;
			}
			MaterialFunctionMapper::replaceSimpleMeshMaterials(te, ColourValue(1, 1, 0));
			if(materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(te);
			if(!usedSkin.isNull()) usedSkin->replaceMeshMaterials(te);
			props[free_prop].snode=manager->getRootSceneNode()->createChildSceneNode();
			if(te)
				props[free_prop].snode->attachObject(te);
			//hack for the spinprops
			if (!strncmp("spinprop", meshname, 8))
			{
				props[free_prop].spinner=1;
				if(props[free_prop].snode->numAttachedObjects())
					props[free_prop].snode->getAttachedObject(0)->setCastShadows(false);
				props[free_prop].snode->setVisible(false);
			}
			if (!strncmp("pale", meshname, 4))
			{
				props[free_prop].pale=1;
			}
			//detect driver seat, used to position the driver and make the seat translucent at times
			if (!strncmp("seat", meshname, 4) && !driversseatfound)
			{
				driversseatfound=true;
				if(te)
					te->setMaterialName("driversseat");
				driverSeat = &props[free_prop];
			}
			else if (!strncmp("seat2", meshname, 5) && !driversseatfound)
			{
				driversseatfound=true;
				driverSeat = &props[free_prop];
			}
			props[free_prop].beacontype='n';
			if (!strncmp("beacon", meshname, 6) && flaresMode>0)
			{
				ColourValue color = ColourValue(1.0, 0.5, 0.0);
				String matname = "tracks/beaconflare";
				char beaconmaterial[256];
				float br=0, bg=0, bb=0;
				int result2 = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s %s %f, %f, %f", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname, beaconmaterial, &br, &bg, &bb);
				if(result2 != result && result2 >= 14)
				{
					color = ColourValue(br, bg, bb);
					matname = String(beaconmaterial);
				}

				props[free_prop].bpos[0]=2.0*3.14*((Real)rand()/(Real)RAND_MAX);
				props[free_prop].brate[0]=4.0*3.14+((Real)rand()/(Real)RAND_MAX)-0.5;
				props[free_prop].beacontype='b';
				props[free_prop].bbs[0]=0;
				//the light
				props[free_prop].light[0]=manager->createLight(propname);
				props[free_prop].light[0]->setType(Light::LT_SPOTLIGHT);
				props[free_prop].light[0]->setDiffuseColour(color);
				props[free_prop].light[0]->setSpecularColour(color);
				props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
				props[free_prop].light[0]->setSpotlightRange( Degree(35), Degree(45) );
				props[free_prop].light[0]->setCastShadows(false);
				props[free_prop].light[0]->setVisible(false);
				//the flare billboard
				props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
				props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
				props[free_prop].bbs[0]->createBillboard(0,0,0);
				if(props[free_prop].bbs[0])
					props[free_prop].bbs[0]->setMaterialName(matname);
				if(props[free_prop].bbs[0])
					props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
				props[free_prop].bbsnode[0]->setVisible(false);
			}
			if (!strncmp("redbeacon", meshname, 9) && flaresMode>0)
			{
				props[free_prop].bpos[0]=0.0;
				props[free_prop].brate[0]=1.0;
				props[free_prop].beacontype='r';
				props[free_prop].bbs[0]=0;
				//the light
				props[free_prop].light[0]=manager->createLight(propname);
				props[free_prop].light[0]->setType(Light::LT_POINT);
				props[free_prop].light[0]->setDiffuseColour( ColourValue(1.0, 0.0, 0.0));
				props[free_prop].light[0]->setSpecularColour( ColourValue(1.0, 0.0, 0.0));
				props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
				props[free_prop].light[0]->setCastShadows(false);
				props[free_prop].light[0]->setVisible(false);
				//the flare billboard
				props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
				props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
				props[free_prop].bbs[0]->createBillboard(0,0,0);
				if(props[free_prop].bbs[0])
					props[free_prop].bbs[0]->setMaterialName("tracks/redbeaconflare");
				if(props[free_prop].bbs[0])
					props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
				props[free_prop].bbsnode[0]->setVisible(false);
				props[free_prop].bbs[0]->setDefaultDimensions(1.0, 1.0);
			}
			if (!strncmp("lightbar", meshname, 6) && flaresMode>0)
			{
				int k;
				ispolice=true;
				props[free_prop].beacontype='p';
				for (k=0; k<4; k++)
				{
					props[free_prop].bpos[k]=2.0*3.14*((Real)rand()/(Real)RAND_MAX);
					props[free_prop].brate[k]=4.0*3.14+((Real)rand()/(Real)RAND_MAX)-0.5;
					props[free_prop].bbs[k]=0;
					//the light
					char rpname[256];
					sprintf(rpname,"%s-%i", propname, k);
					props[free_prop].light[k]=manager->createLight(rpname);
					props[free_prop].light[k]->setType(Light::LT_SPOTLIGHT);
					if (k>1)
					{
						props[free_prop].light[k]->setDiffuseColour( ColourValue(1.0, 0.0, 0.0));
						props[free_prop].light[k]->setSpecularColour( ColourValue(1.0, 0.0, 0.0));
					}
					else
					{
						props[free_prop].light[k]->setDiffuseColour( ColourValue(0.0, 0.5, 1.0));
						props[free_prop].light[k]->setSpecularColour( ColourValue(0.0, 0.5, 1.0));
					}
					props[free_prop].light[k]->setAttenuation(50.0, 1.0, 0.3, 0.0);
					props[free_prop].light[k]->setSpotlightRange( Degree(35), Degree(45) );
					props[free_prop].light[k]->setCastShadows(false);
					props[free_prop].light[k]->setVisible(false);
					//the flare billboard
					props[free_prop].bbsnode[k] = manager->getRootSceneNode()->createChildSceneNode();
					props[free_prop].bbs[k]=manager->createBillboardSet(rpname,1);
					props[free_prop].bbs[k]->createBillboard(0,0,0);
					if(props[free_prop].bbs[k])
					{
						if (k>1)
							props[free_prop].bbs[k]->setMaterialName("tracks/brightredflare");
						else
							props[free_prop].bbs[k]->setMaterialName("tracks/brightblueflare");
						if(props[free_prop].bbs[k])
							props[free_prop].bbsnode[k]->attachObject(props[free_prop].bbs[k]);
					}
					props[free_prop].bbsnode[k]->setVisible(false);
				}
			}

			//set no animation by default
			props[free_prop].animFlags[0]=0;
			props[free_prop].animMode[0]=0;

			free_prop++;
		}
		else if (mode==21)
		{
			//parse globeams
			int result = sscanf(line,"%f, %f, %f, %s", &default_deform,&default_break,&default_beam_diameter, default_beam_material);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (globeams) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			fadeDist=1000.0;
		}
		else if (mode==22)
		{
			//parse wings
			int nds[8];
			float txes[8];
			char type;
			float cratio, mind, maxd, liftcoef = 1.0f;
			char afname[256];
			int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %f, %f, %f, %f, %f, %c, %f, %f, %f, %s %f",
				&nds[0],
				&nds[1],
				&nds[2],
				&nds[3],
				&nds[4],
				&nds[5],
				&nds[6],
				&nds[7],
				&txes[0],
				&txes[1],
				&txes[2],
				&txes[3],
				&txes[4],
				&txes[5],
				&txes[6],
				&txes[7],
				&type,
				&cratio,
				&mind,
				&maxd,
				afname,
				&liftcoef
				);

			//visuals
			if (result < 13 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Wing) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_wing >= MAX_WINGS)
			{
				LogManager::getSingleton().logMessage("wings limit reached ("+StringConverter::toString(MAX_WINGS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			char wname[256];
			sprintf(wname, "wing-%s-%i",truckname, free_wing);
			char wnamei[256];
			sprintf(wnamei, "wingobj-%s-%i",truckname, free_wing);
			if (liftcoef != 1.0f) LogManager::getSingleton().logMessage("Wing liftforce coefficent: " + StringConverter::toString(liftcoef));
			wings[free_wing].fa=new FlexAirfoil(manager, wname, nodes, nds[0], nds[1], nds[2], nds[3], nds[4], nds[5], nds[6], nds[7], texname, Vector2(txes[0], txes[1]), Vector2(txes[2], txes[3]), Vector2(txes[4], txes[5]), Vector2(txes[6], txes[7]), type, cratio, mind, maxd, afname, liftcoef, aeroengines, state!=NETWORKED);
			Entity *ec=0;
			try
			{
				ec = manager->createEntity(wnamei, wname);
			}catch(...)
			{
				LogManager::getSingleton().logMessage("error loading mesh: "+String(wname));
				continue;
			}
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0.5, 1, 0));
			if(materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
			if(!usedSkin.isNull()) usedSkin->replaceMeshMaterials(ec);
			wings[free_wing].cnode = manager->getRootSceneNode()->createChildSceneNode();
			if(ec)
				wings[free_wing].cnode->attachObject(ec);
			//induced drag
			if (wingstart==-1) {wingstart=free_wing;wingarea=warea(nodes[wings[free_wing].fa->nfld].AbsPosition, nodes[wings[free_wing].fa->nfrd].AbsPosition, nodes[wings[free_wing].fa->nbld].AbsPosition, nodes[wings[free_wing].fa->nbrd].AbsPosition);}
			else
			{
				if (nds[1]!=wings[free_wing-1].fa->nfld)
				{
					//discontinuity
					//inform wing segments
					float span=(nodes[wings[wingstart].fa->nfrd].RelPosition-nodes[wings[free_wing-1].fa->nfld].RelPosition).length();
					//					float chord=(nodes[wings[wingstart].fa->nfrd].Position-nodes[wings[wingstart].fa->nbrd].Position).length();
					LogManager::getSingleton().logMessage("BEAM: Full Wing "+StringConverter::toString(wingstart)+"-"+StringConverter::toString(free_wing-1)+" SPAN="+StringConverter::toString(span)+" AREA="+StringConverter::toString(wingarea));
					wings[wingstart].fa->enableInducedDrag(span,wingarea, false);
					wings[free_wing-1].fa->enableInducedDrag(span,wingarea, true);
					//we want also to add positional lights for first wing
					if (!hasposlights && flaresMode>0)
					{

						if(free_prop+4 >= MAX_PROPS)
						{
							LogManager::getSingleton().logMessage("cannot create wing props: props limit reached ("+StringConverter::toString(MAX_PROPS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							continue;
						}
						//Left green
						leftlight=wings[free_wing-1].fa->nfld;
						props[free_prop].noderef=wings[free_wing-1].fa->nfld;
						props[free_prop].nodex=wings[free_wing-1].fa->nflu;
						props[free_prop].nodey=wings[free_wing-1].fa->nfld; //ignored
						props[free_prop].offsetx=0.5;
						props[free_prop].offsety=0.0;
						props[free_prop].offsetz=0.0;
						props[free_prop].rot=Quaternion::IDENTITY;
						props[free_prop].wheel=0;
						props[free_prop].wheelrotdegree=0.0;
						props[free_prop].mirror=0;
						props[free_prop].pale=0;
						props[free_prop].spinner=0;
						props[free_prop].snode=NULL; //no visible prop
						props[free_prop].bpos[0]=0.0;
						props[free_prop].brate[0]=1.0;
						props[free_prop].beacontype='L';
						//no light
						props[free_prop].light[0]=0;
						//the flare billboard
						char propname[256];
						sprintf(propname, "prop-%s-%i", truckname, free_prop);
						props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if(props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName("tracks/greenflare");
							props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						}
						props[free_prop].bbsnode[0]->setVisible(false);
						props[free_prop].bbs[0]->setDefaultDimensions(0.5, 0.5);
						props[free_prop].animFlags[0]=0;
						props[free_prop].animMode[0]=0;
						free_prop++;
						//Left flash
						props[free_prop].noderef=wings[free_wing-1].fa->nbld;
						props[free_prop].nodex=wings[free_wing-1].fa->nblu;
						props[free_prop].nodey=wings[free_wing-1].fa->nbld; //ignored
						props[free_prop].offsetx=0.5;
						props[free_prop].offsety=0.0;
						props[free_prop].offsetz=0.0;
						props[free_prop].rot=Quaternion::IDENTITY;
						props[free_prop].wheel=0;
						props[free_prop].wheelrotdegree=0.0;
						props[free_prop].mirror=0;
						props[free_prop].pale=0;
						props[free_prop].spinner=0;
						props[free_prop].snode=NULL; //no visible prop
						props[free_prop].bpos[0]=0.5; //alt
						props[free_prop].brate[0]=1.0;
						props[free_prop].beacontype='w';
						//light
						sprintf(propname, "prop-%s-%i", truckname, free_prop);
						props[free_prop].light[0]=manager->createLight(propname);
						props[free_prop].light[0]->setType(Light::LT_POINT);
						props[free_prop].light[0]->setDiffuseColour( ColourValue(1.0, 1.0, 1.0));
						props[free_prop].light[0]->setSpecularColour( ColourValue(1.0, 1.0, 1.0));
						props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
						props[free_prop].light[0]->setCastShadows(false);
						props[free_prop].light[0]->setVisible(false);
						//the flare billboard
						props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if(props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName("tracks/flare");
							props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						}
						props[free_prop].bbsnode[0]->setVisible(false);
						props[free_prop].bbs[0]->setDefaultDimensions(1.0, 1.0);
						free_prop++;
						//Right red
						rightlight=wings[wingstart].fa->nfrd;
						props[free_prop].noderef=wings[wingstart].fa->nfrd;
						props[free_prop].nodex=wings[wingstart].fa->nfru;
						props[free_prop].nodey=wings[wingstart].fa->nfrd; //ignored
						props[free_prop].offsetx=0.5;
						props[free_prop].offsety=0.0;
						props[free_prop].offsetz=0.0;
						props[free_prop].rot=Quaternion::IDENTITY;
						props[free_prop].wheel=0;
						props[free_prop].wheelrotdegree=0.0;
						props[free_prop].mirror=0;
						props[free_prop].pale=0;
						props[free_prop].spinner=0;
						props[free_prop].snode=NULL; //no visible prop
						props[free_prop].bpos[0]=0.0;
						props[free_prop].brate[0]=1.0;
						props[free_prop].beacontype='R';
						//no light
						props[free_prop].light[0]=0;
						//the flare billboard
						sprintf(propname, "prop-%s-%i", truckname, free_prop);
						props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if(props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName("tracks/redflare");
							props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						}
						props[free_prop].bbsnode[0]->setVisible(false);
						props[free_prop].bbs[0]->setDefaultDimensions(0.5, 0.5);
						props[free_prop].animFlags[0]=0;
						props[free_prop].animMode[0]=0;
						free_prop++;
						//Right flash
						props[free_prop].noderef=wings[wingstart].fa->nbrd;
						props[free_prop].nodex=wings[wingstart].fa->nbru;
						props[free_prop].nodey=wings[wingstart].fa->nbrd; //ignored
						props[free_prop].offsetx=0.5;
						props[free_prop].offsety=0.0;
						props[free_prop].offsetz=0.0;
						props[free_prop].rot=Quaternion::IDENTITY;
						props[free_prop].wheel=0;
						props[free_prop].wheelrotdegree=0.0;
						props[free_prop].mirror=0;
						props[free_prop].pale=0;
						props[free_prop].spinner=0;
						props[free_prop].snode=NULL; //no visible prop
						props[free_prop].bpos[0]=0.5; //alt
						props[free_prop].brate[0]=1.0;
						props[free_prop].beacontype='w';
						//light
						sprintf(propname, "prop-%s-%i", truckname, free_prop);
						props[free_prop].light[0]=manager->createLight(propname);
						props[free_prop].light[0]->setType(Light::LT_POINT);
						props[free_prop].light[0]->setDiffuseColour( ColourValue(1.0, 1.0, 1.0));
						props[free_prop].light[0]->setSpecularColour( ColourValue(1.0, 1.0, 1.0));
						props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
						props[free_prop].light[0]->setCastShadows(false);
						props[free_prop].light[0]->setVisible(false);
						//the flare billboard
						props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if(props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName("tracks/flare");
							props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						}
						props[free_prop].bbsnode[0]->setVisible(false);
						props[free_prop].bbs[0]->setDefaultDimensions(1.0, 1.0);
						props[free_prop].animFlags[0]=0;
						props[free_prop].animMode[0]=0;
						free_prop++;
						hasposlights=true;
					}
					wingstart=free_wing;
					wingarea=warea(nodes[wings[free_wing].fa->nfld].AbsPosition, nodes[wings[free_wing].fa->nfrd].AbsPosition, nodes[wings[free_wing].fa->nbld].AbsPosition, nodes[wings[free_wing].fa->nbrd].AbsPosition);
				}
				else wingarea+=warea(nodes[wings[free_wing].fa->nfld].AbsPosition, nodes[wings[free_wing].fa->nfrd].AbsPosition, nodes[wings[free_wing].fa->nbld].AbsPosition, nodes[wings[free_wing].fa->nbrd].AbsPosition);
			}

			free_wing++;
		}
		else if (mode==23 || mode==35 || mode==36) //turboprops, turboprops2, pistonprops
		{
			//parse turboprops
			int ref,back,p1,p2,p3,p4;
			int couplenode=-1;
			float pitch=-10;
			bool isturboprops=true;
			float power;
			char propfoil[256];
			if (mode==23)
			{
				int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %f, %s", &ref, &back, &p1, &p2, &p3, &p4, &power, propfoil);
				if (result < 8 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Turboprop) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
			}
			if (mode==35)
			{
				int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %f, %s", &ref, &back, &p1, &p2, &p3, &p4, &couplenode, &power, propfoil);
				if (result < 9 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Turboprop2) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
			}
			if (mode==36)
			{
				int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %f, %f, %s", &ref, &back, &p1, &p2, &p3, &p4, &couplenode, &power, &pitch, propfoil);
				if (result < 10 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Pistonprop) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				isturboprops=false;
			}

			if(free_aeroengine >= MAX_AEROENGINES)
			{
				LogManager::getSingleton().logMessage("airoengine limit reached ("+StringConverter::toString(MAX_AEROENGINES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			char propname[256];
			sprintf(propname, "turboprop-%s-%i", truckname, free_aeroengine);
			Turboprop *tp=new Turboprop(manager, propname, nodes, ref, back,p1,p2,p3,p4, couplenode, power, propfoil, free_aeroengine, trucknum, disable_smoke, !isturboprops, pitch, heathaze);
			aeroengines[free_aeroengine]=tp;
			driveable=AIRPLANE;
			if (!autopilot && state != NETWORKED)
				autopilot=new Autopilot(hfinder, water, trucknum);
			//if (audio) audio->setupAeroengines(audiotype);
			//setup visual
			int i;
			float pscale=(nodes[ref].RelPosition-nodes[p1].RelPosition).length()/2.25;
			for (i=0; i<free_prop; i++)
			{
				if (props[i].pale && props[i].noderef==ref)
				{
					//setup size
					props[i].snode->scale(pscale,pscale,pscale);
					tp->addPale(props[i].snode);
				}
				if (props[i].spinner && props[i].noderef==ref)
				{
					props[i].snode->scale(pscale,pscale,pscale);
					tp->addSpinner(props[i].snode);
				}
			}
			free_aeroengine++;
		}
		else if (mode==24)
		{
			//parse fusedrag
			int front,back;
			float width;
			char fusefoil[256];
			int result = sscanf(line,"%i, %i, %f, %s", &front,&back,&width, fusefoil);
			if (result < 4 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Fusedrag) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			fuseAirfoil=new Airfoil(fusefoil);
			fuseFront=&nodes[front];
			fuseBack=&nodes[front];
			fuseWidth=width;
		}
		else if (mode==25)
		{
			//parse engoption
			float inertia;
			char type;
			float clutch = -1.0f, shifttime = -1.0f, clutchtime = -1.0f, postshifttime = -1.0f;
			int result = sscanf(line,"%f, %c, %f, %f, %f, %f", &inertia, &type, &clutch, &shifttime, &clutchtime, &postshifttime);
			if (result < 1 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Engoption) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (engine) engine->setOptions(inertia, type, clutch, shifttime, clutchtime, postshifttime);
		}
		else if (mode==26)
		{
			// parse brakes
			int result = sscanf(line,"%f, %f", &brakeforce, &hbrakeforce);
			// Read in footbrake force and handbrake force. If handbrakeforce is not present, set it to the default value 2*footbrake force to preserve older functionality
			if (result == 1)
				hbrakeforce = 2.0f * brakeforce;
		}
		else if (mode==27)
		{
			//parse rotators
			int axis1, axis2,keys,keyl;
			int p1[4], p2[4];
			float rate;
			hascommands=1;
			Real startDelay=0;
			Real stopDelay=0;
			char startFunction[50]="";
			char stopFunction[50]="";
			int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %f, %i, %i, %f, %f, %s %s", &axis1, &axis2, &p1[0], &p1[1], &p1[2], &p1[3], &p2[0], &p2[1], &p2[2], &p2[3], &rate, &keys, &keyl, &startDelay, &stopDelay, startFunction, stopFunction);
			if (result < 13 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Rotators) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_rotator >= MAX_ROTATORS)
			{
				LogManager::getSingleton().logMessage("rotators limit reached ("+StringConverter::toString(MAX_ROTATORS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			rotators[free_rotator].angle=0;
			rotators[free_rotator].rate=rate;
			rotators[free_rotator].axis1=axis1;
			rotators[free_rotator].axis2=axis2;
			int i;
			for (i=0; i<4; i++)
			{
				rotators[free_rotator].nodes1[i]=p1[i];
				rotators[free_rotator].nodes2[i]=p2[i];
			}
			//add short key
			commandkey[keys].rotators.push_back(-(free_rotator+1));
			commandkey[keys].description = "Rotate Left";
			//add long key
			commandkey[keyl].rotators.push_back(free_rotator+1);
			commandkey[keyl].description = "Rotate Right";

			if(rotaInertia && startDelay > 0 && stopDelay > 0)
			{
				rotaInertia->setCmdKeyDelay(keys,startDelay,stopDelay,String (startFunction),String (stopFunction));
				rotaInertia->setCmdKeyDelay(keyl,startDelay,stopDelay,String (startFunction),String (stopFunction));
			}
			else if(rotaInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
			{
				rotaInertia->setCmdKeyDelay(keys,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
				rotaInertia->setCmdKeyDelay(keyl,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
			}

			free_rotator++;
		}
		else if (mode==28)
		{
			//parse screwprops
			int ref,back,up;
			float power;
			int result = sscanf(line,"%i, %i, %i, %f", &ref,&back,&up, &power);
			if (result < 4 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Screwprops) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//if (audio) audio->setupBoat(truckmass);

			if(free_screwprop >= MAX_SCREWPROPS)
			{
				LogManager::getSingleton().logMessage("screwprops limit reached ("+StringConverter::toString(MAX_SCREWPROPS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			screwprops[free_screwprop]=new Screwprop(nodes, ref, back, up, power, water, trucknum);
			driveable=BOAT;
			free_screwprop++;
		}
		else if (mode==32)
		{
			// guisettings
			char keyword[255];
			char value[255];
			int result = sscanf(line,"%s %s", keyword, value);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (guisettings) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(!strncmp(keyword, "interactiveOverviewMap", 255) && strnlen(value, 255) > 0)
			{
				dynamicMapMode = 0;
				if(!strncmp(value, "off", 255))
					dynamicMapMode = 0;
				else if(!strncmp(value, "simple", 255))
					dynamicMapMode = 1;
				else if(!strncmp(value, "zoom", 255))
					dynamicMapMode = 2;
			}
			if(!strncmp(keyword, "debugBeams", 255) && strnlen(value, 255) > 0)
			{
				debugVisuals = 0;
				if(!strncmp(value, "off", 255))
					debugVisuals = 0;
				else if(!strncmp(value, "node-numbers", 255))
					debugVisuals = 1;
				else if(!strncmp(value, "beam-numbers", 255))
					debugVisuals = 2;
				else if(!strncmp(value, "node-and-beam-numbers", 255))
					debugVisuals = 3;
				else if(!strncmp(value, "node-mass", 255))
					debugVisuals = 4;
				else if(!strncmp(value, "node-locked", 255))
					debugVisuals = 5;
				else if(!strncmp(value, "beam-compression", 255))
					debugVisuals = 6;
				else if(!strncmp(value, "beam-broken", 255))
					debugVisuals = 7;
				else if(!strncmp(value, "beam-stress", 255))
					debugVisuals = 8;
				else if(!strncmp(value, "beam-strength", 255))
					debugVisuals = 9;
				else if(!strncmp(value, "beam-hydros", 255))
					debugVisuals = 10;
				else if(!strncmp(value, "beam-commands", 255))
					debugVisuals = 11;
			}
			if(!strncmp(keyword, "tachoMaterial", 255) && strnlen(value, 255) > 0)
			{
				tachomat = String(value);
			}
			else if(!strncmp(keyword, "speedoMaterial", 255) && strnlen(value, 255) > 0)
			{
				speedomat = String(value);
			}
			else if(!strncmp(keyword, "helpMaterial", 255) && strnlen(value, 255) > 0)
			{
				strncpy(helpmat, value, 254);
			}
			else if(!strncmp(keyword, "speedoMax", 255) && strnlen(value, 255) > 0)
			{
				float tmp = StringConverter::parseReal(String(value));
				if(tmp > 10 && tmp < 32000)
					speedoMax = tmp;
			}
			else if(!strncmp(keyword, "useMaxRPM", 255) && strnlen(value, 255) > 0)
			{
				int use = StringConverter::parseInt(String(value));
				useMaxRPMforGUI = (use == 1);
			}

		}
		else if (mode==33)
		{
			//parse minimass
			//sets the minimum node mass
			//usefull for very light vehicles with lots of nodes (e.g. small airplanes)
			sscanf(line,"%f", &minimass);
		}
		else if (mode==34)
		{
			// parse exhausts
			if (disable_smoke)
				continue;
			int id1, id2;
			float factor;
			char material[50] = "";
			int result = sscanf(line,"%i, %i, %f %s", &id1, &id2, &factor, material);
			// catch some errors
			if (result < 4 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			exhaust_t e;
			e.emitterNode = id1;
			e.directionNode = id2;
			e.isOldFormat = false;
			e.smokeNode = parent->createChildSceneNode();
			char wname[256];
			sprintf(wname, "exhaust-%d-%s", exhausts.size(), truckname);
			if(strnlen(material,50) == 0 || String(material) == "default")
				strcpy(material, "tracks/Smoke");

			if(!usedSkin.isNull()) strncpy(material, usedSkin->getReplacementForMaterial(material).c_str(), 50);

			e.smoker = manager->createParticleSystem(wname, material);
			if (!e.smoker)
				continue;
			e.smokeNode->attachObject(e.smoker);
			e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
			nodes[id1].isHot=true;
			nodes[id2].isHot=true;
			nodes[id1].iIsSkin=true;
			nodes[id2].iIsSkin=true;
			exhausts.push_back(e);
		}
		else if (mode==37)
		{
			// command lists
			//truckScript->addCommand(line, currentScriptCommandNumber);
		}
		else if (mode==38)
		{
			//particles
			if(!cparticle_enabled)
				continue;
			// parse particle
			int id1, id2;
			char psystem[250] = "";
			int result = sscanf(line,"%i, %i, %s", &id1, &id2, psystem);
			// catch some errors
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_cparticle >= MAX_CPARTICLES)
			{
				LogManager::getSingleton().logMessage("custom particles limit reached ("+StringConverter::toString(MAX_CPARTICLES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			cparticles[free_cparticle].emitterNode = id1;
			cparticles[free_cparticle].directionNode = id2;
			cparticles[free_cparticle].snode = parent->createChildSceneNode();
			char wname[256];
			sprintf(wname, "cparticle-%i-%s", free_cparticle, truckname);
			cparticles[free_cparticle].psys = manager->createParticleSystem(wname, psystem);
			if (!cparticles[free_cparticle].psys)
				continue;
			cparticles[free_cparticle].snode->attachObject(cparticles[free_cparticle].psys);
			cparticles[free_cparticle].snode->setPosition(nodes[cparticles[free_cparticle].emitterNode].AbsPosition);
			//shut down the emitters
			cparticles[free_cparticle].active=false;
			for (int i=0; i<cparticles[free_cparticle].psys->getNumEmitters(); i++) cparticles[free_cparticle].psys->getEmitter(i)->setEnabled(false);
			free_cparticle++;
		}
		else if (mode==39) //turbojets
		{
			//parse turbojets
			int front,back,ref, rev;
			float len, fdiam, bdiam, drthrust, abthrust;
			int result = sscanf(line,"%i, %i, %i, %i, %f, %f, %f, %f, %f", &front, &back, &ref, &rev, &drthrust, &abthrust, &fdiam, &bdiam, &len);
			if (result < 9 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Turbojet) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_aeroengine >= MAX_AEROENGINES)
			{
				LogManager::getSingleton().logMessage("airoengine limit reached ("+StringConverter::toString(MAX_AEROENGINES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			char propname[256];
			sprintf(propname, "turbojet-%s-%i", truckname, free_aeroengine);
			Turbojet *tj=new Turbojet(manager, propname, free_aeroengine, trucknum, nodes, front, back, ref, drthrust, rev!=0, abthrust>0, abthrust, fdiam, bdiam, len, disable_smoke, heathaze, materialFunctionMapper, usedSkin);
			aeroengines[free_aeroengine]=tj;
			driveable=AIRPLANE;
			if (!autopilot && state != NETWORKED)
				autopilot=new Autopilot(hfinder, water, trucknum);
			//if (audio) audio->setupAeroengines(TURBOJETS);
			free_aeroengine++;
		}
		else if (mode==40)
		{
			//parse rigidifiers
			int na,nb,nc;
			float k=DEFAULT_RIGIDIFIER_SPRING;
			float d=DEFAULT_RIGIDIFIER_DAMP;
			int result = sscanf(line,"%i, %i, %i, %f, %f", &na, &nb, &nc, &k, &d);
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (rigidifier) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_rigidifier >= MAX_RIGIDIFIERS)
			{
				LogManager::getSingleton().logMessage("rigidifiers limit reached ("+StringConverter::toString(MAX_RIGIDIFIERS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			rigidifiers[free_rigidifier].a=&nodes[na];
			rigidifiers[free_rigidifier].b=&nodes[nb];
			rigidifiers[free_rigidifier].c=&nodes[nc];
			rigidifiers[free_rigidifier].k=k;
			rigidifiers[free_rigidifier].d=d;
			rigidifiers[free_rigidifier].alpha=2.0*acos((nodes[na].RelPosition-nodes[nb].RelPosition).getRotationTo(nodes[nc].RelPosition-nodes[nb].RelPosition).w);
			rigidifiers[free_rigidifier].lastalpha=rigidifiers[free_rigidifier].alpha;
			rigidifiers[free_rigidifier].beama=0;
			rigidifiers[free_rigidifier].beamc=0;
			//searching for associated beams
			for (int i=0; i<free_beam; i++)
			{
				if ((beams[i].p1==&nodes[na] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[na] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beama=&beams[i];
				if ((beams[i].p1==&nodes[nc] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[nc] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beamc=&beams[i];
			}
			free_rigidifier++;
		}
		else if (mode==41)
		{
			//parse airbrakes
			int ref, nx, ny, na;
			float ox, oy, oz;
			float tx1, tx2, tx3, tx4;
			float wd, len, liftcoef = 1.0f;
			float maxang;
			int result = sscanf(line,"%i, %i, %i, %i, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", &ref, &nx, &ny, &na, &ox, &oy, &oz, &wd, &len, &maxang, &tx1, &tx2, &tx3, &tx4, &liftcoef);
			if (result < 14 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Airbrakes) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_airbrake >= MAX_AIRBRAKES)
			{
				LogManager::getSingleton().logMessage("airbrakes limit reached ("+StringConverter::toString(MAX_AIRBRAKES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (liftcoef != 1.0f) 
				LogManager::getSingleton().logMessage("Airbrakes force coefficent: " + StringConverter::toString(liftcoef));

			airbrakes[free_airbrake]=new Airbrake(manager, truckname, free_airbrake, &nodes[ref], &nodes[nx], &nodes[ny], &nodes[na], Vector3(ox,oy,oz), wd, len, maxang, texname, tx1,tx2,tx3,tx4,liftcoef);
			free_airbrake++;
		}
		else if (mode==43)
		{
			//parse flexbodies
			int ref, nx, ny;
			float ox, oy, oz;
			float rx, ry, rz;
			char meshname[256];
			int result = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname);
			if (result < 10 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Flexbodies) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			Vector3 offset=Vector3(ox, oy, oz);
			Quaternion rot=Quaternion(Degree(rz), Vector3::UNIT_Z);
			rot=rot*Quaternion(Degree(ry), Vector3::UNIT_Y);
			rot=rot*Quaternion(Degree(rx), Vector3::UNIT_X);
			char uname[256];
			sprintf(uname, "flexbody-%s-%i", truckname, free_flexbody);
			//read an extra line!
			ds->readLine(line, 1023);
			linecounter++;
			if (strncmp(line, "forset", 6))
			{
				LogManager::getSingleton().logMessage("Error parsing File (Flexbodies/forset) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". No forset statement after a flexbody. trying to continue ...");
				continue;
			}

			if(free_flexbody >= MAX_FLEXBODIES)
			{
				LogManager::getSingleton().logMessage("flexbodies limit reached ("+StringConverter::toString(MAX_FLEXBODIES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			flexbodies[free_flexbody]=new FlexBody(manager, nodes, free_node, meshname, uname, ref, nx, ny, offset, rot, line+6, materialFunctionMapper, usedSkin);
			free_flexbody++;
		}
		else if (mode==44)
		{
			//parse hookgroups
			int id1=0, group=-1, lockNodes=1;
			int result = sscanf(line,"%i, %i, %i", &id1, &group, &lockNodes);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (hookgroup) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Too less arguments. trying to continue ...");
				continue;
			}
			if (id1>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in hookgroup section ("+StringConverter::toString(id1)+")");
				exit(7);
			};
			hook_t h;
			h.hookNode=&nodes[id1];
			h.group=group;
			h.locked=UNLOCKED;
			h.lockNode=0;
			h.lockTruck=0;
			h.lockNodes=(bool)(lockNodes != 0);
			hooks.push_back(h);

		}
		else if (mode==46)
		{
			// parse materialflarebindings
			int flareid;
			char material[255]="";
			memset(material, 0, 255);
			int result = sscanf(line,"%d, %s", &flareid, material);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (materialbindings) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			String materialName = String(material);
			String newMaterialName = materialName + "_mfb_" + String(truckname);
			MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
			if(mat.isNull())
			{
				LogManager::getSingleton().logMessage("Error in materialbindings: material " + materialName + " was not found! " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//clone the material
			MaterialPtr newmat = mat->clone(newMaterialName);
			//create structes and add
			materialmapping_t t;
			t.originalmaterial = materialName;
			t.material = newMaterialName;
			t.type=0;
			if(materialFunctionMapper)
				materialFunctionMapper->addMaterial(flareid, t);
		}
		else if (mode==47)
		{
			//parse soundsources
			int ref;
			char script[256];
			int result = sscanf(line,"%i, %s", &ref, script);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (soundsource) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
#ifdef USE_OPENAL
			addSoundSource(ssm->createInstance(script, trucknum, NULL), ref);
#endif //OPENAL
		}
		else if (mode==48)
		{
			// parse envmap
			// we do nothing of this for the moment
		}
		else if (mode==49)
		{
			// parse managedmaterials
			char material[255];
			material[0]=0;
			char type[255];
			type[0]=0;
			int result = sscanf(line,"%s %s", material, type);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//first, check if work has already been done
			MaterialPtr tstmat=(MaterialPtr)(MaterialManager::getSingleton().getByName(material));
			if (!tstmat.isNull())
			{
				//material already exists, probably because the vehicle was already spawned previously
				LogManager::getSingleton().logMessage("Warning: managed material '" + String(material) +"' already exists");
				continue;
			}

			if (!strcmp(type, "flexmesh_standard"))
			{
				char maintex[255];
				maintex[0]=0;
				char dmgtex[255];
				dmgtex[0]=0;
				char spectex[255];
				spectex[0]=0;
				result = sscanf(line,"%s %s %s %s %s", material, type, maintex, dmgtex, spectex);
				if (result < 3 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//different cases
				//caution, this is hardwired against the managed.material file
				if (strlen(dmgtex)==0 || dmgtex[0]=='-')
				{
					//no damage texture
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/simple"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_standard/simple' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular, but no damage
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/specularonly"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_standard/specularonly' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
				}
				else
				{
					//damage texture
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/damageonly"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_standard/damageonly' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(dmgtex);
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular, and damage
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/speculardamage"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_standard/speculardamage' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName(dmgtex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
				}
			}
			else if (!strcmp(type, "flexmesh_transparent"))
			{
				char maintex[255];
				maintex[0]=0;
				char dmgtex[255];
				dmgtex[0]=0;
				char spectex[255];
				spectex[0]=0;
				result = sscanf(line,"%s %s %s %s %s", material, type, maintex, dmgtex, spectex);
				if (result < 3 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//different cases
				//caution, this is hardwired against the managed.material file
				if (strlen(dmgtex)==0 || dmgtex[0]=='-')
				{
					//no damage texture
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/simple"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_transparent/simple' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular, but no damage
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/specularonly"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_transparent/specularonly' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
				}
				else
				{
					//damage texture
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/damageonly"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_transparent/damageonly' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(dmgtex);
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular, and damage
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/speculardamage"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_transparent/speculardamage' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName(dmgtex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
				}
			}
			else if (!strcmp(type, "mesh_standard"))
			{
				char maintex[255];
				maintex[0]=0;
				char spectex[255];
				spectex[0]=0;
				result = sscanf(line,"%s %s %s %s", material, type, maintex, spectex);
				if (result < 3 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//different cases
				//caution, this is hardwired against the managed.material file
				if (strlen(spectex)==0 || spectex[0]=='-')
				{
					//no specular texture
					MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_standard/simple"));
					if(srcmat.isNull())
					{
						LogManager::getSingleton().logMessage("Material 'managed/mesh_standard/simple' missing!");
						continue;
					}

					MaterialPtr dstmat=srcmat->clone(material);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
					dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
					dstmat->compile();
				}
				else
				{
					//specular
					MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_standard/specular"));
					if(srcmat.isNull())
					{
						LogManager::getSingleton().logMessage("Material 'managed/mesh_standard/specular' missing!");
						continue;
					}

					MaterialPtr dstmat=srcmat->clone(material);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
					dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
					dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
					dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
					dstmat->compile();
				}
			}
			else if (!strcmp(type, "mesh_transparent"))
			{
				char maintex[255];
				maintex[0]=0;
				char spectex[255];
				spectex[0]=0;
				result = sscanf(line,"%s %s %s %s", material, type, maintex, spectex);
				if (result < 3 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//different cases
				//caution, this is hardwired against the managed.material file
				if (strlen(spectex)==0 || spectex[0]=='-')
				{
					//no specular texture
					MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_transparent/simple"));
					if(srcmat.isNull())
					{
						LogManager::getSingleton().logMessage("Material 'managed/mesh_transparent/simple' missing!");
						continue;
					}

					MaterialPtr dstmat=srcmat->clone(material);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
					dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
					dstmat->compile();
				}
				else
				{
					//specular
					MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_transparent/specular"));
					if(srcmat.isNull())
					{
						LogManager::getSingleton().logMessage("Material 'managed/mesh_transparent/specular' missing!");
						continue;
					}

					MaterialPtr dstmat=srcmat->clone(material);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
					dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
					dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
					dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
					dstmat->compile();
				}
			}
			else
			{
				LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Unknown effect. trying to continue ...");
				continue;
			}

		}
		else if (mode==50)
		{
			// parse sectionconfig
			// not used here ...
			mode=savedmode;
		}
		else if (mode==51)
		{
			// parse section
			int version=0;
			char sectionName[10][256];
			for(int i=0;i<10;i++) memset(sectionName, 0, 255); // clear
			if(strnlen(line,9)<8) continue;
			int result = sscanf(line+7,"%d %s %s %s %s %s %s %s %s %s %s", &version, sectionName[0], sectionName[1], sectionName[2], sectionName[3], sectionName[4], sectionName[5], sectionName[6], sectionName[7], sectionName[8], sectionName[9]);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (section) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			bool found = false;
			for(int i=0;i<10;i++)
			{
				for(std::vector<String>::iterator it=truckconfig.begin(); it!=truckconfig.end(); it++)
				{
					if(sectionName[i] == *it)
					{
						found = true;
						break;
					}
				}
			}
			if(found)
				continue;
			else
				// wait for end_section otherwise
				mode=52;
		}
		/* mode 52 is reserved */
		else if (mode==53)
		{
			// parse torquecurve
			if (engine && engine->getTorqueCurve())
				engine->getTorqueCurve()->processLine(String(line));
		} else if (mode==54)
		{
			//parse advanced drag
			float drag;
			int result = sscanf(line,"%f", &drag);
			if (result < 4 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (advdrag) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			advanced_total_drag=drag;
			advanced_drag=true;
		}
		else if (mode==55)
		{
			// parse axle section
			// search for wheel

			if(!free_wheel)
			{
				LogManager::getSingleton().logMessage("AXLE ERROR: the axle section must come AFTER some wheels");
				continue;
			}

			int wheel_node[2][2] = {0};
			Ogre::vector<Ogre::String>::type options = Ogre::StringUtil::split(line, ",");
			Ogre::vector<Ogre::String>::type::iterator cur = options.begin();

			for(; cur != options.end(); ++cur)
			{
				Ogre::StringUtil::trim(*cur);

				LogManager::getSingleton().logMessage("AXLE: Parsing property: [" + *cur + "]" );

				switch(cur->at(0))
				{
				// wheels
				case 'w':
					// dirty repetitive method, could stand to be cleaned up
					if( cur->at(1) == '1')
					{
						int result = sscanf(cur->c_str(), "w1(%d %d)", &wheel_node[0][0], &wheel_node[0][1]);

						if (result < 2 )
						{
							LogManager::getSingleton().logMessage("AXLE: line did not contain enough points: " + *cur);
							continue;
						}
					}
					else if( cur->at(1) == '2')
					{
						int result = sscanf(cur->c_str(), "w2(%d %d)", &wheel_node[1][0], &wheel_node[1][1]);

						if (result < 2 )
						{
							LogManager::getSingleton().logMessage("AXLE: line did not contain enough points: " + *cur);
							continue;
						}
					}
					break;
				case 'd':
				{
						char diffs[10] = {0};
						int results = sscanf(cur->c_str(), "d(%9s)", diffs);
						if(results == 0 ) break;
						for(int i = 0; i < 10; ++i)
						{
							switch(diffs[i])
							{
							case 'l': axles[free_axle].addDiffType(LOCKED_DIFF); break;
							case 'o': axles[free_axle].addDiffType(OPEN_DIFF); break;
							//case 'v': axles[free_axle].addDiffType(VISCOUS_DIFF); break;
							case 's': axles[free_axle].addDiffType(SPLIT_DIFF); break;
							//case 'm': axles[free_axle].addDiffType(LIMITED_SLIP_DIFF); break;
							}
						}
					break;
				}
				case 's':
					LogManager::getSingleton().logMessage("AXLE: selection property not yet available");
					break;
				case 'r':
					LogManager::getSingleton().logMessage("AXLE: Gear ratio property not yet available");
					break;
				default:
					LogManager::getSingleton().logMessage("AXLE: malformed property: " + *cur);
					break;
				}

			}

			//
			for( int i = 0; i < free_wheel &&  (axles[free_axle].wheel_1 < 0 || axles[free_axle].wheel_2 < 0); ++i)
			{
				if( ( wheels[i].refnode0->id == wheel_node[0][0] || wheels[i].refnode0->id == wheel_node[0][1]) &&
					( wheels[i].refnode1->id == wheel_node[0][0] || wheels[i].refnode1->id == wheel_node[0][1]))
				{
					axles[free_axle].wheel_1 = i;
				}
				if( ( wheels[i].refnode0->id == wheel_node[1][0] || wheels[i].refnode0->id == wheel_node[1][1]) &&
					( wheels[i].refnode1->id == wheel_node[1][0] || wheels[i].refnode1->id == wheel_node[1][1]))
				{
				axles[free_axle].wheel_2 = i;
				}
			}


			if( axles[free_axle].wheel_1 < 0 || axles[free_axle].wheel_2 < 0 )
			{
				// if one or the other is null
				if( axles[free_axle].wheel_1 < 0)
				{
					LogManager::getSingleton().logMessage("AXLE: could not find wheel 1 nodes: " +
						StringConverter::toString(wheel_node[0][0]) + " " +
						StringConverter::toString(wheel_node[0][1]) );
				}
				if( axles[free_axle].wheel_2 < 0)
				{
					LogManager::getSingleton().logMessage("AXLE: could not find wheel 2 nodes: " +
					StringConverter::toString(wheel_node[1][0]) + " " +
					StringConverter::toString(wheel_node[1][1]) );
				}
				continue;
			}

			// test if any differentials have been defined
			if( axles[free_axle].availableDiffs().empty() )
			{
				LogManager::getSingleton().logMessage("AXLE: nodiffs defined, defaulting to Open and Locked");
				axles[free_axle].addDiffType(OPEN_DIFF);
				axles[free_axle].addDiffType(LOCKED_DIFF);
			}

			LogManager::getSingleton().logMessage("AXLE: Created: w1(" + StringConverter::toString(wheel_node[0][0]) + ") " +
				StringConverter::toString(wheel_node[0][1]) + ", w2(" + StringConverter::toString(wheel_node[1][0]) + " " +
				StringConverter::toString(wheel_node[1][1]) + ")");
			++free_axle;
		}

		else if (mode==63) parseRailGroupLine(line);
		else if (mode==64) parseSlideNodeLine(line);

	};
	if(!loading_finished) {
		LogManager::getSingleton().logMessage("Reached end of file "+ String(fname)+ ". No 'end' was found! Did you forgot it? Trying to continue...");
	}

	// ds closes automatically, so do not close it explicitly here:
	//ds->close();
//cameras workaround
	{
		for (int i=0; i<freecamera; i++)
		{
//LogManager::getSingleton().logMessage("Camera dir="+StringConverter::toString(nodes[cameranodedir[i]].RelPosition-nodes[cameranodepos[i]].RelPosition)+" roll="+StringConverter::toString(nodes[cameranoderoll[i]].RelPosition-nodes[cameranodepos[i]].RelPosition));
			revroll[i]=(nodes[cameranodedir[i]].RelPosition-nodes[cameranodepos[i]].RelPosition).crossProduct(nodes[cameranoderoll[i]].RelPosition-nodes[cameranodepos[i]].RelPosition).y>0;
			if (revroll[i]) LogManager::getSingleton().logMessage("Warning: camera definition is probably invalid and has been corrected. It should be center, back, left");
		}
	}
//		fclose(fd);
	//wing closure
	if (wingstart!=-1)
	{
		if (autopilot) autopilot->setInertialReferences(&nodes[leftlight], &nodes[rightlight], fuseBack);
		//inform wing segments
		float span=(nodes[wings[wingstart].fa->nfrd].RelPosition-nodes[wings[free_wing-1].fa->nfld].RelPosition).length();
		//		float chord=(nodes[wings[wingstart].fa->nfrd].Position-nodes[wings[wingstart].fa->nbrd].Position).length();
		LogManager::getSingleton().logMessage("BEAM: Full Wing "+StringConverter::toString(wingstart)+"-"+StringConverter::toString(free_wing-1)+" SPAN="+StringConverter::toString(span)+" AREA="+StringConverter::toString(wingarea));
		wings[wingstart].fa->enableInducedDrag(span,wingarea, false);
		wings[free_wing-1].fa->enableInducedDrag(span,wingarea, true);
		//wash calculator
		wash_calculator(rot);
	}
	//add the cab visual
	LogManager::getSingleton().logMessage("BEAM: creating cab");
	if (free_texcoord>0 && free_cab>0)
	{
		//closure
		subtexcoords[free_sub]=free_texcoord;
		subcabs[free_sub]=free_cab;
		char wname[256];
		sprintf(wname, "cab-%s",truckname);
		char wnamei[256];
		sprintf(wnamei, "cabobj-%s",truckname);
		//the cab materials are as follow:
		//texname: base texture with emissive(2 pass) or without emissive if none available(1 pass), alpha cutting
		//texname-trans: transparency texture (1 pass)
		//texname-back: backface texture: black+alpha cutting (1 pass)
		//texname-noem: base texture without emissive (1 pass), alpha cutting

		//material passes must be:
		//0: normal texture
		//1: transparent (windows)
		//2: emissive
		/*strcpy(texname, "testtex");
		char transmatname[256];
		sprintf(transmatname, "%s-trans", texname);
		char backmatname[256];
		sprintf(backmatname, "%s-back", texname);
		hasEmissivePass=1;*/

		MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().getByName(texname));
		if(mat.isNull())
		{
			LogManager::getSingleton().logMessage("Material '"+String(texname)+"' missing!");
			exit(123);
		}

		//-trans
		char transmatname[256];
		sprintf(transmatname, "%s-trans", texname);
		MaterialPtr transmat=mat->clone(transmatname);
		if (mat->getTechnique(0)->getNumPasses()>1) transmat->getTechnique(0)->removePass(1);
		transmat->getTechnique(0)->getPass(0)->setAlphaRejectSettings(CMPF_LESS_EQUAL, 128);
		transmat->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
		if(transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
			transmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(TFO_NONE);
		transmat->compile();

		//-back
		char backmatname[256];
		sprintf(backmatname, "%s-back", texname);
		MaterialPtr backmat=mat->clone(backmatname);
		if (mat->getTechnique(0)->getNumPasses()>1) backmat->getTechnique(0)->removePass(1);
		if(transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
			backmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setColourOperationEx(LBX_SOURCE1, LBS_MANUAL, LBS_MANUAL, ColourValue(0,0,0),ColourValue(0,0,0));
		backmat->setReceiveShadows(false);
		//just in case
		//backmat->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
		//backmat->getTechnique(0)->getPass(0)->setAlphaRejectSettings(CMPF_GREATER, 128);
		backmat->compile();

		//-noem and -noem-trans
		if (mat->getTechnique(0)->getNumPasses()>1)
		{
			hasEmissivePass=1;
			char clomatname[256];
			sprintf(clomatname, "%s-noem", texname);
			MaterialPtr clomat=mat->clone(clomatname);
			clomat->getTechnique(0)->removePass(1);
			clomat->compile();
		}

		//base texture is not modified
		//	mat->compile();


		LogManager::getSingleton().logMessage("BEAM: creating mesh");
		cabMesh=new FlexObj(manager, nodes, free_texcoord, texcoords, free_cab, cabs, free_sub, subtexcoords, subcabs, texname, wname, subisback, backmatname, transmatname);
		LogManager::getSingleton().logMessage("BEAM: creating entity");

		LogManager::getSingleton().logMessage("BEAM: creating cabnode");
		cabNode = manager->getRootSceneNode()->createChildSceneNode();
		Entity *ec = 0;
		try
		{
			LogManager::getSingleton().logMessage("BEAM: loading cab");
			ec = manager->createEntity(wnamei, wname);
			//		ec->setRenderQueueGroup(RENDER_QUEUE_6);
			LogManager::getSingleton().logMessage("BEAM: attaching cab");
			if(ec)
				cabNode->attachObject(ec);
		}catch(...)
		{
			LogManager::getSingleton().logMessage("error loading mesh: "+String(wname));
		}
		MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0.5, 1, 0.5));
		if(materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
		if(!usedSkin.isNull()) usedSkin->replaceMeshMaterials(ec);
	};
	LogManager::getSingleton().logMessage("BEAM: cab ok");
	//	mWindow->setDebugText("Beam number:"+ StringConverter::toString(free_beam));

	//place correctly
	if (!hasfixes)
	{
		//check if oversized
		calcBox();
		//px=px-(maxx-minx)/2.0;
		px-=(maxx+minx)/2.0-px;
		//pz=pz-(maxz-minz)/2.0;
		pz-=(maxz+minz)/2.0-pz;
		float miny=-9999.0;
		if (spawnbox) miny=spawnbox->relo_y+spawnbox->center.y+0.01;
		if(freePositioned)
			resetPosition(Vector3(px, py, pz), true);
		else
			resetPosition(px, pz, true, miny);

		if (spawnbox)
		{
			bool inside=true;
			for (int i=0; i<free_node; i++) inside=inside && collisions->isInside(nodes[i].AbsPosition, spawnbox, 0.2f);
			if (!inside)
			{
				Vector3 gpos=Vector3(px, 0, pz);
				gpos-=rot*Vector3((spawnbox->hi_x-spawnbox->lo_x+maxx-minx)*0.6, 0, 0);
				resetPosition(gpos.x, gpos.z, true, miny);
			}
		}
	}
	//compute final mass
	calc_masses2(truckmass);
	//setup default sounds
	if (!disable_default_sounds) setupDefaultSoundSources();
	//compute collision box
	calcBox();

	//compute node connectivity graph
	calcNodeConnectivityGraph();

	//update contacter nodes
	updateContacterNodes();

	// print some truck memory stats
	int mem = 0, memr = 0, tmpmem = 0;
	LogManager::getSingleton().logMessage("BEAM: memory stats following");

	tmpmem = free_beam * sizeof(beam_t); mem += tmpmem;
	memr += MAX_BEAMS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: beam memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_beam) + " x " + StringConverter::toString(sizeof(beam_t)) + " B) / " + StringConverter::toString(MAX_BEAMS * sizeof(beam_t)));

	tmpmem = free_node * sizeof(node_t); mem += tmpmem;
	memr += MAX_NODES * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: node memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_node) + " x " + StringConverter::toString(sizeof(node_t)) + " B) / " + StringConverter::toString(MAX_NODES * sizeof(node_t)));

	tmpmem = free_shock * sizeof(shock_t); mem += tmpmem;
	memr += MAX_SHOCKS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: shock memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_shock) + " x " + StringConverter::toString(sizeof(shock_t)) + " B) / " + StringConverter::toString(MAX_SHOCKS * sizeof(shock_t)));

	tmpmem = free_prop * sizeof(prop_t); mem += tmpmem;
	memr += MAX_PROPS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: prop memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_prop) + " x " + StringConverter::toString(sizeof(prop_t)) + " B) / " + StringConverter::toString(MAX_PROPS * sizeof(prop_t)));

	tmpmem = free_wheel * sizeof(wheel_t); mem += tmpmem;
	memr += MAX_WHEELS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: wheel memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_wheel) + " x " + StringConverter::toString(sizeof(wheel_t)) + " B) / " + StringConverter::toString(MAX_WHEELS * sizeof(wheel_t)));

	tmpmem = free_rigidifier * sizeof(rigidifier_t); mem += tmpmem;
	memr += MAX_RIGIDIFIERS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: rigidifier memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_rigidifier) + " x " + StringConverter::toString(sizeof(rigidifier_t)) + " B) / " + StringConverter::toString(MAX_RIGIDIFIERS * sizeof(rigidifier_t)));

	tmpmem = free_flare * sizeof(flare_t); mem += tmpmem;
	memr += free_flare * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: flare memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_flare) + " x " + StringConverter::toString(sizeof(flare_t)) + " B)");

	LogManager::getSingleton().logMessage("BEAM: truck memory used: " + StringConverter::toString(mem)  + " B (" + StringConverter::toString(mem/1024)  + " kB)");
	LogManager::getSingleton().logMessage("BEAM: truck memory allocated: " + StringConverter::toString(memr)  + " B (" + StringConverter::toString(memr/1024)  + " kB)");
	return 0;
}

void Beam::addSoundSource(SoundScriptInstance *ssi, int nodenum)
{
	if (!ssi) return; //fizzle
	if (free_soundsource==MAX_SOUNDSCRIPTS_PER_TRUCK)
	{
		LogManager::getSingleton().logMessage("BEAM: Error, too many sound sources per vehicle!");
		return;
	}
	soundsources[free_soundsource].ssi=ssi;
	soundsources[free_soundsource].nodenum=nodenum;
	free_soundsource++;
}

void Beam::setupDefaultSoundSources()
{
#ifdef USE_OPENAL
	//engine
	if (engine)
	{
		if (engine->type=='t')
		{
			addSoundSource(ssm->createInstance("tracks/default_diesel", trucknum, NULL), smokeId);
			addSoundSource(ssm->createInstance("tracks/default_force", trucknum, NULL), smokeId);
			addSoundSource(ssm->createInstance("tracks/default_brakes", trucknum, NULL), 0);
			addSoundSource(ssm->createInstance("tracks/default_parkbrakes", trucknum, NULL), 0);
			addSoundSource(ssm->createInstance("tracks/default_reverse_beep", trucknum, NULL), 0);
		}
		if (engine->type=='c')
			addSoundSource(ssm->createInstance("tracks/default_car", trucknum, NULL), smokeId);
		if (engine->hasturbo)
			addSoundSource(ssm->createInstance("tracks/default_turbo", trucknum, NULL), smokeId);
		if (engine->hasair)
			addSoundSource(ssm->createInstance("tracks/default_air_purge", trucknum, NULL), 0);
		//starter
		addSoundSource(ssm->createInstance("tracks/default_starter", trucknum, NULL), 0);
		// turn signals
		addSoundSource(ssm->createInstance("tracks/default_turn_signal", trucknum, NULL), 0);
	}
	if (driveable==TRUCK)
	{
		//horn
		if (ispolice)
			addSoundSource(ssm->createInstance("tracks/default_police", trucknum, NULL), 0);
		else
			addSoundSource(ssm->createInstance("tracks/default_horn", trucknum, NULL), 0);
		//shift
		addSoundSource(ssm->createInstance("tracks/default_shift", trucknum, NULL), 0);
	}
	//pump
	if (hascommands)
		addSoundSource(ssm->createInstance("tracks/default_pump", trucknum, NULL), 0);
	//screetch
	if ((driveable==TRUCK || driveable==AIRPLANE) && free_wheel)
		addSoundSource(ssm->createInstance("tracks/default_screetch", trucknum, NULL), 0);
	//break & creak
	addSoundSource(ssm->createInstance("tracks/default_break", trucknum, NULL), 0);
	addSoundSource(ssm->createInstance("tracks/default_creak", trucknum, NULL), 0);
	//boat engine
	if (driveable==BOAT)
	{
		if (totalmass>50000.0)
			addSoundSource(ssm->createInstance("tracks/default_marine_large", trucknum, NULL), smokeId);
		else
			addSoundSource(ssm->createInstance("tracks/default_marine_small", trucknum, NULL), smokeId);
		//no start/stop engine for boats, so set sound always on!
		ssm->trigStart(trucknum, SS_TRIG_ENGINE);
		ssm->modulate(trucknum, SS_MOD_ENGINE, 0.5);
	}
	//airplane engines
	for (int i=0; i<free_aeroengine && i<8; i++)
	{
		if (aeroengines[i]->getType()==AEROENGINE_TYPE_TURBOJET)
		{
			addSoundSource(ssm->createInstance(String("tracks/default_turbojet_start")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			addSoundSource(ssm->createInstance(String("tracks/default_turbojet_lopower")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			addSoundSource(ssm->createInstance(String("tracks/default_turbojet_hipower")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			if (((Turbojet*)(aeroengines[i]))->afterburnable)
				addSoundSource(ssm->createInstance(String("tracks/default_turbojet_afterburner")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
		}
		else if (aeroengines[i]->getType()==AEROENGINE_TYPE_TURBOPROP)
		{
			if (((Turboprop*)aeroengines[i])->is_piston)
			{
				addSoundSource(ssm->createInstance(String("tracks/default_pistonprop_start")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
				addSoundSource(ssm->createInstance(String("tracks/default_pistonprop_lopower")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
				addSoundSource(ssm->createInstance(String("tracks/default_pistonprop_hipower")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			}
			else
			{
				addSoundSource(ssm->createInstance(String("tracks/default_turboprop_start")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
				addSoundSource(ssm->createInstance(String("tracks/default_turboprop_lopower")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
				addSoundSource(ssm->createInstance(String("tracks/default_turboprop_hipower")+StringConverter::toString(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			}
		}
	}

#endif //OPENAL
}

void Beam::calcBox()
{
	minx=nodes[0].AbsPosition.x;
	maxx=nodes[0].AbsPosition.x;
	miny=nodes[0].AbsPosition.y;
	maxy=nodes[0].AbsPosition.y;
	minz=nodes[0].AbsPosition.z;
	maxz=nodes[0].AbsPosition.z;
	lowestnode=-1;
	int i;
	for (i=1; i<free_node; i++)
	{
		if (nodes[i].AbsPosition.x>maxx) maxx=nodes[i].AbsPosition.x;
		if (nodes[i].AbsPosition.x<minx) minx=nodes[i].AbsPosition.x;
		if (nodes[i].AbsPosition.y>maxy) maxy=nodes[i].AbsPosition.y;
		if (nodes[i].AbsPosition.y<miny)
		{
			miny=nodes[i].AbsPosition.y;
			lowestnode=i;
		}
		if (nodes[i].AbsPosition.z>maxz) maxz=nodes[i].AbsPosition.z;
		if (nodes[i].AbsPosition.z<minz) minz=nodes[i].AbsPosition.z;
	}
	minx-=0.3;
	maxx+=0.3;
	miny-=0.3;
	maxy+=0.3;
	minz-=0.3;
	maxz+=0.3;
}

void Beam::calcNodeConnectivityGraph()
{
	int i;

	nodetonodeconnections.resize(free_node, std::vector< int >());
	nodebeamconnections.resize(free_node, std::vector< int >());

	for (i=0; i<free_beam; i++)
	{
		if (beams[i].p1!=NULL && beams[i].p2!=NULL && beams[i].p1->pos>=0 && beams[i].p2->pos>=0)
		{
			nodetonodeconnections[beams[i].p1->pos].push_back(beams[i].p2->pos);
			nodebeamconnections[beams[i].p1->pos].push_back(i);
			nodetonodeconnections[beams[i].p2->pos].push_back(beams[i].p1->pos);
			nodebeamconnections[beams[i].p2->pos].push_back(i);
		}
	}
}

void Beam::updateContacterNodes()
{
	for (int i=0; i<free_collcab; i++)
	{
		int tmpv=collcabs[i]*3;
		nodes[cabs[tmpv]].contacter=true;
		nodes[cabs[tmpv+1]].contacter=true;
		nodes[cabs[tmpv+2]].contacter=true;
	}
}

float Beam::warea(Vector3 ref, Vector3 x, Vector3 y, Vector3 aref)
{
	return (((x-ref).crossProduct(y-ref)).length()+((x-aref).crossProduct(y-aref)).length())*0.5f;
}

void Beam::wash_calculator(Quaternion rot)
{
	Quaternion invrot=rot.Inverse();
	//we will compute wash
	int w,p;
	for (p=0; p<free_aeroengine; p++)
	{
		Vector3 prop=invrot*nodes[aeroengines[p]->getNoderef()].RelPosition;
		float radius=aeroengines[p]->getRadius();
		for (w=0; w<free_wing; w++)
		{
			//left wash
			Vector3 wcent=invrot*((nodes[wings[w].fa->nfld].RelPosition+nodes[wings[w].fa->nfrd].RelPosition)/2.0);
			//check if wing is near enough along X (less than 15m back)
			if (wcent.x>prop.x && wcent.x<prop.x+15.0)
			{
				//check if it's okay vertically
				if (wcent.y>prop.y-radius && wcent.y<prop.y+radius)
				{
					//okay, compute wash coverage ratio along Z
					float wleft=(invrot*nodes[wings[w].fa->nfld].RelPosition).z;
					float wright=(invrot*nodes[wings[w].fa->nfrd].RelPosition).z;
					float pleft=prop.z+radius;
					float pright=prop.z-radius;
					float aleft=wleft;
					if (pleft<aleft) aleft=pleft;
					float aright=wright;
					if (pright>aright) aright=pright;
					if (aright<aleft)
					{
						//we have a wash
						float wratio=(aleft-aright)/(wleft-wright);
						wings[w].fa->addwash(p, wratio);
						LogManager::getSingleton().logMessage("BEAM: Wing "+StringConverter::toString(w)+" is washed by prop "+StringConverter::toString(p)+" at "+StringConverter::toString((float)(wratio*100.0))+"%");
					}
				}
			}
		}
	}
}

int Beam::savePosition(int indexPosition)
{
	if(!posStorage) return -1;
	Vector3* nbuff = posStorage->getStorage(indexPosition);
	if(!nbuff) return -3;
	for (int i=0; i<free_node; i++)
		nbuff[i] = nodes[i].AbsPosition;
	posStorage->setUsage(indexPosition, true);
	return 0;
}

int Beam::loadPosition(int indexPosition)
{
	if(!posStorage) return -1;
	if(!posStorage->getUsage(indexPosition)) return -2;

	Vector3* nbuff = posStorage->getStorage(indexPosition);
	if(!nbuff) return -3;
	Vector3 pos = Vector3(0,0,0);
	for (int i=0; i<free_node; i++)
	{
		nodes[i].AbsPosition   = nbuff[i];
		nodes[i].RelPosition   = nbuff[i] - origin;
		nodes[i].smoothpos     = nbuff[i];

		// reset forces
		nodes[i].Velocity      = Vector3::ZERO;
		nodes[i].Forces        = Vector3::ZERO;
		nodes[i].lastdrag      = Vector3::ZERO;
		nodes[i].buoyanceForce = Vector3::ZERO;
		nodes[i].lastdrag      = Vector3::ZERO;
		nodes[i].lockednode    = 0;
		nodes[i].isSkin        = nodes[i].iIsSkin;

		pos = pos + nbuff[i];
	}
	position = pos / (float)(free_node);

	resetSlideNodes();

	return 0;
}

void Beam::resetAngle(float rot)
{
	// Set origin of rotation to camera node
	Vector3 origin = nodes[cameranodepos[0]].AbsPosition;

	// Set up matrix for yaw rotation
	Matrix3 matrix;
	matrix.FromEulerAnglesXYZ(Radian(0), Radian(-rot - PI/2), Radian(0));

	for (int i = 0; i < free_node; i++)
	{
		// Move node back to origin, apply rotation matrix, and move node back
		nodes[i].AbsPosition -= origin;
		nodes[i].AbsPosition  = matrix * nodes[i].AbsPosition;
		nodes[i].AbsPosition += origin;
		// Update related values
		nodes[i].RelPosition  = nodes[i].AbsPosition;
		nodes[i].smoothpos    = nodes[i].AbsPosition;
	}

	resetSlideNodePositions();
}

void Beam::resetPosition(float px, float pz, bool setI, float miny)
{
	if(!hfinder)
		return;
	int i;
	//horizontal displacement
	Vector3 offset=Vector3(px,0,pz)-nodes[0].AbsPosition;
	offset.y=-ipy;
	for (i=0; i<free_node; i++)
	{
		nodes[i].AbsPosition+=offset;
	}
	//vertical
	float minoffset=0.0;
	if (miny<-9000)
	{
		minoffset=nodes[0].AbsPosition.y-hfinder->getHeightAt(nodes[0].AbsPosition.x, nodes[0].AbsPosition.z);
		for (i=1; i<free_node; i++)
		{
			Vector3 pos=Vector3(nodes[i].AbsPosition.x,hfinder->getHeightAt(nodes[i].AbsPosition.x, nodes[i].AbsPosition.z),nodes[i].AbsPosition.z);
			//if (water && pos.y<water->getHeight()) pos.y=water->getHeight();
			collisions->collisionCorrect(&pos);
			float offset=nodes[i].AbsPosition.y-pos.y;
			if (offset<minoffset) minoffset=offset;
		}
	}
	else
	{
		minoffset=nodes[0].AbsPosition.y-miny;
		for (i=1; i<free_node; i++)
		{
			float offset=nodes[i].AbsPosition.y-miny;
			if (offset<minoffset) minoffset=offset;
		}
	}
	if (water && -minoffset<water->getHeight()) minoffset=-water->getHeight();

	// calculate average position
	Vector3 apos=Vector3::ZERO;
	for (i=0; i<free_node; i++)
	{
		nodes[i].AbsPosition.y-=minoffset;
		if (setI) nodes[i].iPosition=nodes[i].AbsPosition;
		//		if (setI) nodes[i].iPosition.y-=minoffset;
		nodes[i].smoothpos=nodes[i].AbsPosition;
		nodes[i].RelPosition=nodes[i].AbsPosition-origin;
		apos+=nodes[i].AbsPosition;
	}

	position=apos/free_node;

	// calculate min camera radius for truck
	if(minCameraRadius<0.01)
	{
		// recalc
		for (i=0; i<free_node; i++)
		{
			Real dist = nodes[i].AbsPosition.distance(position);
			if(dist > minCameraRadius)
			{
				minCameraRadius = dist;
			}
		}
		minCameraRadius *= 1.2f; // ten percent buffer
	}

	//if (netLabelNode) netLabelNode->setPosition(nodes[0].Position);

	resetSlideNodePositions();
}

void Beam::resetPosition(Ogre::Vector3 translation, bool setInitPosition)
{
	int i;
	// total displacement
	if(translation != Vector3::ZERO)
	{
		Vector3 offset = translation - nodes[0].AbsPosition;
		for (i=0; i<free_node; i++)
			nodes[i].AbsPosition += offset;
	}

	// calculate average position
	Vector3 apos=Vector3::ZERO;
	for (i=0; i<free_node; i++)
	{
		if (setInitPosition)
			nodes[i].iPosition=nodes[i].AbsPosition;
		nodes[i].smoothpos = nodes[i].AbsPosition;
		nodes[i].RelPosition = nodes[i].AbsPosition-origin;
		apos += nodes[i].AbsPosition;
	}
	position = apos / free_node;

	// calculate min camera radius for truck
	if(minCameraRadius < 0.01f)
	{
		// recalc
		for (i=0; i<free_node; i++)
		{
			Real dist = nodes[i].AbsPosition.distance(position);
			if(dist > minCameraRadius)
			{
				minCameraRadius = dist;
			}
		}
		minCameraRadius *= 1.2f; // ten percent buffer
	}

	//if (netLabelNode) netLabelNode->setPosition(nodes[0].Position);

	resetSlideNodePositions();

}

void Beam::mouseMove(int node, Vector3 pos, float force)
{
	mousenode = node;
	mousemoveforce = force;
	mousepos = pos;
}

void Beam::addCamera(int nodepos, int nodedir, int noderoll)
{
	cameranodepos[freecamera]=nodepos;
	cameranodedir[freecamera]=nodedir;
	cameranoderoll[freecamera]=noderoll;
	freecamera++;
}

int Beam::calculateDriverPos(Vector3 &pos, Quaternion &rot)
{
	if(!this || !driverSeat) return 1;
	Vector3 normal=(nodes[driverSeat->nodey].smoothpos-nodes[driverSeat->noderef].smoothpos).crossProduct(nodes[driverSeat->nodex].smoothpos-nodes[driverSeat->noderef].smoothpos);
	normal.normalise();
	//position
	Vector3 mposition=nodes[driverSeat->noderef].smoothpos+driverSeat->offsetx*(nodes[driverSeat->nodex].smoothpos-nodes[driverSeat->noderef].smoothpos)+driverSeat->offsety*(nodes[driverSeat->nodey].smoothpos-nodes[driverSeat->noderef].smoothpos);
	pos = mposition+normal*driverSeat->offsetz;

	//orientation
	Vector3 refx=nodes[driverSeat->nodex].smoothpos-nodes[driverSeat->noderef].smoothpos;
	refx.normalise();
	Vector3 refy=refx.crossProduct(normal);
	rot = Quaternion(refx, normal, refy) * driverSeat->rot * Quaternion(Degree(180), Vector3::UNIT_Y); // rotate towards the driving direction
	return 0;
}

void Beam::addWheel(SceneManager *manager, SceneNode *parent, Real radius, Real width, int rays, int node1, int node2, int snode, int braked, int propulsed, int torquenode, float mass, float wspring, float wdamp, char* texf, char* texb, bool meshwheel, float rimradius, bool rimreverse)
{
	int i;
	int nodebase=free_node;
	int node3;
	int contacter_wheel=1;
	//ignore the width parameter
	width=(nodes[node1].RelPosition-nodes[node2].RelPosition).length();
	//enforce the "second node must have a larger Z coordinate than the first" constraint
	if (nodes[node1].RelPosition.z>nodes[node2].RelPosition.z)
	{
		//swap
		node3=node1;
		node1=node2;
		node2=node3;
	}
	//ignore the sign of snode, just do the thing automatically
	//if (snode<0) node3=-snode; else node3=snode;
	if (snode<0) snode=-snode;
	bool closest1=false;
	if (snode!=9999) closest1=(nodes[snode].RelPosition-nodes[node1].RelPosition).length()<(nodes[snode].RelPosition-nodes[node2].RelPosition).length();

	//unused:
	//Real px=nodes[node1].Position.x;
	//Real py=nodes[node1].Position.y;
	//Real pz=nodes[node1].Position.z;

	Vector3 axis=nodes[node2].RelPosition-nodes[node1].RelPosition;
	axis.normalise();
	Vector3 rayvec = axis.perpendicular() * radius;
	// old rayvec:
	//Vector3 rayvec=Vector3(0, radius, 0);
	Quaternion rayrot=Quaternion(Degree(-360.0/(Real)(rays*2)), axis);
	for (i=0; i<rays; i++)
	{
		//with propnodes and variable friction
//			init_node(nodebase+i*2, px+radius*sin((Real)i*6.283185307179/(Real)rays), py+radius*cos((Real)i*6.283185307179/(Real)rays), pz, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width);
		Vector3 raypoint;
		raypoint=nodes[node1].RelPosition+rayvec;
		rayvec=rayrot*rayvec;
		init_node(nodebase+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);

		// outer ring has wheelid%2 != 0
		nodes[nodebase+i*2].iswheel = free_wheel*2+1;

		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+i*2;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
//			init_node(nodebase+i*2+1, px+radius*sin((Real)i*6.283185307179/(Real)rays), py+radius*cos((Real)i*6.283185307179/(Real)rays), pz+width, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width);
		raypoint=nodes[node2].RelPosition+rayvec;

		rayvec=rayrot*rayvec;
		init_node(nodebase+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);

		// inner ring has wheelid%2 == 0
		nodes[nodebase+i*2+1].iswheel = free_wheel*2+2;
		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+i*2+1;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
		//wheel object
		wheels[free_wheel].nodes[i*2]=&nodes[nodebase+i*2];
		wheels[free_wheel].nodes[i*2+1]=&nodes[nodebase+i*2+1];
	}
	free_node+=2*rays;
	for (i=0; i<rays; i++)
	{
		//bounded
		add_beam(&nodes[node1], &nodes[nodebase+i*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp, -1.0, 0.66, 0.0);
		//bounded
		add_beam(&nodes[node2], &nodes[nodebase+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp, -1.0, 0.66, 0.0);
		add_beam(&nodes[node2], &nodes[nodebase+i*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[node1], &nodes[nodebase+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		//reinforcement
		add_beam(&nodes[node1], &nodes[nodebase+i*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[nodebase+i*2], &nodes[nodebase+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		//reinforcement
		//init_beam(free_beam , &nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);free_beam++;

		if (snode!=9999)
		{
			//back beams //BEAM_VIRTUAL

			if (closest1) {add_beam(&nodes[snode], &nodes[nodebase+i*2], manager, parent, BEAM_VIRTUAL, default_break, wspring, wdamp);}
			else         {add_beam(&nodes[snode], &nodes[nodebase+i*2+1], manager, parent, BEAM_VIRTUAL, default_break, wspring, wdamp);};
			/* THIS ALMOST WORKS BUT IT IS INSTABLE AT SPEED !!!!
			//rigidifier version
			if(free_rigidifier >= MAX_RIGIDIFIERS)
			{
				LogManager::getSingleton().logMessage("rigidifiers limit reached ...");
			}

			int na=(closest1)?node2:node1;
			int nb=(closest1)?node1:node2;
			int nc=snode;
			rigidifiers[free_rigidifier].a=&nodes[na];
			rigidifiers[free_rigidifier].b=&nodes[nb];
			rigidifiers[free_rigidifier].c=&nodes[nc];
			rigidifiers[free_rigidifier].k=wspring;
			rigidifiers[free_rigidifier].d=wdamp;
			rigidifiers[free_rigidifier].alpha=2.0*acos((nodes[na].RelPosition-nodes[nb].RelPosition).getRotationTo(nodes[nc].RelPosition-nodes[nb].RelPosition).w);
			rigidifiers[free_rigidifier].lastalpha=rigidifiers[free_rigidifier].alpha;
			rigidifiers[free_rigidifier].beama=0;
			rigidifiers[free_rigidifier].beamc=0;
			//searching for associated beams
			for (int i=0; i<free_beam; i++)
			{
				if ((beams[i].p1==&nodes[na] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[na] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beama=&beams[i];
				if ((beams[i].p1==&nodes[nc] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[nc] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beamc=&beams[i];
			}
			free_rigidifier++;
			*/
		}
	}
	//wheel object
	wheels[free_wheel].braked=braked;
	wheels[free_wheel].propulsed=propulsed;
	wheels[free_wheel].nbnodes=2*rays;
	wheels[free_wheel].refnode0=&nodes[node1];
	wheels[free_wheel].refnode1=&nodes[node2];
	wheels[free_wheel].radius=radius;
	wheels[free_wheel].speed=0.0;
	wheels[free_wheel].rp=0;
	wheels[free_wheel].rp1=0;
	wheels[free_wheel].rp2=0;
	wheels[free_wheel].rp3=0;
	wheels[free_wheel].width=width;
	wheels[free_wheel].arm=&nodes[torquenode];
	wheels[free_wheel].lastContactInner=Vector3::ZERO;
	wheels[free_wheel].lastContactOuter=Vector3::ZERO;
	if (propulsed>0)
	{
		//for inter-differential locking
		proppairs[proped_wheels]=free_wheel;
		proped_wheels++;
	}
	if (braked) braked_wheels++;
	//find near attach
	Real l1=(nodes[node1].RelPosition-nodes[torquenode].RelPosition).length();
	Real l2=(nodes[node2].RelPosition-nodes[torquenode].RelPosition).length();
	if (l1<l2) wheels[free_wheel].near_attach=&nodes[node1]; else wheels[free_wheel].near_attach=&nodes[node2];
	//visuals
	char wname[256];
	sprintf(wname, "wheel-%s-%i",truckname, free_wheel);
	char wnamei[256];
	sprintf(wnamei, "wheelobj-%s-%i",truckname, free_wheel);
	//	strcpy(texf, "tracks/wheelface,");
	vwheels[free_wheel].meshwheel = meshwheel;
	if (meshwheel)
	{
		vwheels[free_wheel].fm=new FlexMeshWheel(manager, wname, nodes, node1, node2, nodebase, rays, texf, texb, rimradius, rimreverse, materialFunctionMapper, usedSkin);
		try
		{
			Entity *ec = manager->createEntity(wnamei, wname);
			vwheels[free_wheel].cnode = manager->getRootSceneNode()->createChildSceneNode();
			if(ec)
				vwheels[free_wheel].cnode->attachObject(ec);
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0, 0.5, 0.5));
			if(materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
			if(!usedSkin.isNull()) usedSkin->replaceMeshMaterials(ec);
		}catch(...)
		{
			LogManager::getSingleton().logMessage("error loading mesh: "+String(wname));
		}
	}
	else
	{
		vwheels[free_wheel].fm=new FlexMesh(manager, wname, nodes, node1, node2, nodebase, rays, texf, texb);
		try
		{
			Entity *ec = manager->createEntity(wnamei, wname);
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0, 0.5, 0.5));
			if(materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
			if(!usedSkin.isNull()) usedSkin->replaceMeshMaterials(ec);
			vwheels[free_wheel].cnode = manager->getRootSceneNode()->createChildSceneNode();
			if(ec)
				vwheels[free_wheel].cnode->attachObject(ec);
		} catch(...)
		{
			LogManager::getSingleton().logMessage("error loading mesh: "+String(wname));
		}
	}
	free_wheel++;
}

void Beam::addWheel2(SceneManager *manager, SceneNode *parent, Real radius, Real radius2, Real width, int rays, int node1, int node2, int snode, int braked, int propulsed, int torquenode, float mass, float wspring, float wdamp, float wspring2, float wdamp2, char* texf, char* texb)
{
	int i;
	int nodebase=free_node;
	int node3;
	int contacter_wheel=1;
	//ignore the width parameter
	width=(nodes[node1].RelPosition-nodes[node2].RelPosition).length();
	//enforce the "second node must have a larger Z coordinate than the first" constraint
	if (nodes[node1].RelPosition.z>nodes[node2].RelPosition.z)
	{
		//swap
		node3=node1;
		node1=node2;
		node2=node3;
	}
	//ignore the sign of snode, just do the thing automatically
	//if (snode<0) node3=-snode; else node3=snode;
	if (snode<0) snode=-snode;
	bool closest1=false;
	if (snode!=9999) closest1=(nodes[snode].RelPosition-nodes[node1].RelPosition).length()<(nodes[snode].RelPosition-nodes[node2].RelPosition).length();

	//unused:
	//Real px=nodes[node1].Position.x;
	//Real py=nodes[node1].Position.y;
	//Real pz=nodes[node1].Position.z;

	Vector3 axis=nodes[node2].RelPosition-nodes[node1].RelPosition;
	axis.normalise();
	Vector3 rayvec=Vector3(0, radius, 0);
	Quaternion rayrot=Quaternion(Degree(-360.0/(Real)rays), axis);
	Quaternion rayrot2=Quaternion(Degree(-180.0/(Real)rays), axis);
	Vector3 rayvec2=Vector3(0, radius2, 0);
	rayvec2=rayrot2*rayvec2;
	//rim nodes
	for (i=0; i<rays; i++)
	{
		//with propnodes
		Vector3 raypoint=nodes[node1].RelPosition+rayvec;
		init_node(nodebase+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, -1, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);
		// outer ring has wheelid%2 != 0
		nodes[nodebase+i*2].iswheel = free_wheel*2+1;

		raypoint=nodes[node2].RelPosition+rayvec;
		init_node(nodebase+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, -1, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);

		// inner ring has wheelid%2 == 0
		nodes[nodebase+i*2+1].iswheel = free_wheel*2+2;
		//wheel object
		wheels[free_wheel].nodes[i*2]=&nodes[nodebase+i*2];
		wheels[free_wheel].nodes[i*2+1]=&nodes[nodebase+i*2+1];
		rayvec=rayrot*rayvec;
	}
	//tire nodes
	for (i=0; i<rays; i++)
	{
		//with propnodes and variable friction
		Vector3 raypoint=nodes[node1].RelPosition+rayvec2;
		init_node(nodebase+2*rays+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, 0.67*mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface);
		// outer ring has wheelid%2 != 0
		nodes[nodebase+2*rays+i*2].iswheel = free_wheel*2+1;
		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+2*rays+i*2;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
		raypoint=nodes[node2].RelPosition+rayvec2;
		init_node(nodebase+2*rays+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, 0.33*mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1,  free_wheel, default_node_friction, default_node_volume, default_node_surface);

		// inner ring has wheelid%2 == 0
		nodes[nodebase+2*rays+i*2+1].iswheel = free_wheel*2+2;
		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+2*rays+i*2+1;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
		//wheel object
//			wheels[free_wheel].nodes[i*2]=&nodes[nodebase+i*2];
//			wheels[free_wheel].nodes[i*2+1]=&nodes[nodebase+i*2+1];
		rayvec2=rayrot*rayvec2; //this is not a bug
	}
	free_node+=4*rays;
	for (i=0; i<rays; i++)
	{
		//rim
		//bounded
		add_beam(&nodes[node1], &nodes[nodebase+i*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp, -1.0, 0.66, 0.0);
		add_beam(&nodes[node2], &nodes[nodebase+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp, -1.0, 0.66, 0.0);
		add_beam(&nodes[node2], &nodes[nodebase+i*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[node1], &nodes[nodebase+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		//reinforcement
		add_beam(&nodes[node1], &nodes[nodebase+i*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[nodebase+i*2], &nodes[nodebase+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(&nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		//reinforcement
		add_beam(&nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);
		if (snode!=9999)
		{
			//back beams
			if (closest1) {add_beam(&nodes[snode], &nodes[nodebase+i*2], manager, parent, BEAM_VIRTUAL, default_break, wspring, wdamp);}
			else         {add_beam(&nodes[snode], &nodes[nodebase+i*2+1], manager, parent, BEAM_VIRTUAL, default_break, wspring, wdamp);};
		}
		//tire
		//band
		//init_beam(free_beam , &nodes[nodebase+2*rays+i*2], &nodes[nodebase+2*rays+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		//pressure_beams[free_pressure_beam]=free_beam-1; free_pressure_beam++;
		int pos;
		pos=add_beam(&nodes[nodebase+2*rays+i*2], &nodes[nodebase+2*rays+((i+1)%rays)*2], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[nodebase+2*rays+i*2], &nodes[nodebase+2*rays+((i+1)%rays)*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+2*rays+((i+1)%rays)*2], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+2*rays+((i+1)%rays)*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		//walls
		pos=add_beam(&nodes[nodebase+2*rays+i*2], &nodes[nodebase+i*2], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[nodebase+2*rays+i*2], &nodes[nodebase+((i+1)%rays)*2], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+((i+1)%rays)*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		//reinforcement
		pos=add_beam(&nodes[nodebase+2*rays+i*2], &nodes[nodebase+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[nodebase+2*rays+i*2], &nodes[nodebase+((i+1)%rays)*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+i*2], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+((i+1)%rays)*2], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		//backpressure, bounded
		pos=add_beam(&nodes[node1], &nodes[nodebase+2*rays+i*2], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2, -1.0, radius/radius2, 0.0);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(&nodes[node2], &nodes[nodebase+2*rays+i*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring2, wdamp2, -1.0, radius/radius2, 0.0);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
	}
	//wheel object
	wheels[free_wheel].braked=braked;
	wheels[free_wheel].propulsed=propulsed;
	wheels[free_wheel].nbnodes=2*rays;
	wheels[free_wheel].refnode0=&nodes[node1];
	wheels[free_wheel].refnode1=&nodes[node2];
	wheels[free_wheel].radius=radius;
	wheels[free_wheel].speed=0.0;
	wheels[free_wheel].width=width;
	wheels[free_wheel].rp=0;
	wheels[free_wheel].rp1=0;
	wheels[free_wheel].rp2=0;
	wheels[free_wheel].rp3=0;
	wheels[free_wheel].arm=&nodes[torquenode];
	if (propulsed)
	{
		//for inter-differential locking
		proppairs[proped_wheels]=free_wheel;
		proped_wheels++;
	}
	if (braked) braked_wheels++;
	//find near attach
	Real l1=(nodes[node1].RelPosition-nodes[torquenode].RelPosition).length();
	Real l2=(nodes[node2].RelPosition-nodes[torquenode].RelPosition).length();
	if (l1<l2) wheels[free_wheel].near_attach=&nodes[node1]; else wheels[free_wheel].near_attach=&nodes[node2];
	//visuals
	char wname[256];
	sprintf(wname, "wheel-%s-%i",truckname, free_wheel);
	char wnamei[256];
	sprintf(wnamei, "wheelobj-%s-%i",truckname, free_wheel);
	//	strcpy(texf, "tracks/wheelface,");
	vwheels[free_wheel].fm=new FlexMesh(manager, wname, nodes, node1, node2, nodebase, rays, texf, texb, true, radius/radius2);
	try
	{
		Entity *ec = manager->createEntity(wnamei, wname);
		MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0, 0.5, 0.5));
		if(materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
		if(!usedSkin.isNull()) usedSkin->replaceMeshMaterials(ec);
		//	ec->setMaterialName("tracks/wheel");
		//ec->setMaterialName("Test/ColourTest");
		vwheels[free_wheel].cnode = manager->getRootSceneNode()->createChildSceneNode();
		if(ec)
			vwheels[free_wheel].cnode->attachObject(ec);
		//	cnode->setPosition(1000,2,940);
		free_wheel++;
	}catch(...)
	{
		LogManager::getSingleton().logMessage("error loading mesh: "+String(wname));
	}
}

void Beam::init_node(int pos, Real x, Real y, Real z, int type, Real m, int iswheel, Real friction, int id, int wheelid, Real nfriction, Real nvolume, Real nsurface, Real nloadweight)
{
	nodes[pos].AbsPosition=Vector3(x,y,z);
	nodes[pos].RelPosition=Vector3(x,y,z)-origin;
	nodes[pos].smoothpos=nodes[pos].AbsPosition;
	nodes[pos].iPosition=Vector3(x,y,z);
	if(pos != 0)
		nodes[pos].iDistance=(nodes[0].AbsPosition - Vector3(x,y,z)).squaredLength();
	else
		nodes[pos].iDistance=0;
	nodes[pos].Velocity=Vector3::ZERO;
	nodes[pos].Forces=Vector3::ZERO;
	nodes[pos].locked=m<0.0;
	nodes[pos].mass=m;
	nodes[pos].iswheel=iswheel;
	nodes[pos].wheelid=wheelid;
	nodes[pos].friction_coef=nfriction;
	nodes[pos].volume_coef=nvolume;
	nodes[pos].surface_coef=nsurface;
	if (nloadweight >=0.0f)
	{
		nodes[pos].masstype=NODE_LOADED;
		nodes[pos].overrideMass=true;
		nodes[pos].mass=nloadweight;
	}
	nodes[pos].disable_particles=false;
	nodes[pos].masstype=type;
	nodes[pos].contactless=0;
	nodes[pos].contacted=0;
	nodes[pos].lockednode=0;
	nodes[pos].buoyanceForce=Vector3::ZERO;
	nodes[pos].buoyancy=truckmass/15.0;//DEFAULT_BUOYANCY;
	nodes[pos].lastdrag=Vector3(0,0,0);
	nodes[pos].gravimass=Vector3(0,ExampleFrameListener::getGravity()*m,0);
	nodes[pos].wetstate=DRY;
	nodes[pos].isHot=false;
	nodes[pos].overrideMass=false;
	nodes[pos].id = id;
	nodes[pos].colltesttimer=0;
	nodes[pos].iIsSkin=false;
	nodes[pos].isSkin=nodes[pos].iIsSkin;
	nodes[pos].pos=pos;
//		nodes[pos].tsmooth=Vector3::ZERO;
	if (type==NODE_LOADED) masscount++;
}

int Beam::add_beam(node_t *p1, node_t *p2, SceneManager *manager, SceneNode *parent, int type, Real strength, Real spring, Real damp, Real length, float shortbound, float longbound, float precomp,float diameter)
{
	int pos=free_beam;

	beams[pos].p1=p1;
	beams[pos].p2=p2;
	beams[pos].p2truck=0;
	beams[pos].type=type;
	if (length<0.0)
	{
		//calculate the length
		Vector3 t;
		t=p1->RelPosition;
		t=t-p2->RelPosition;
		beams[pos].L=precomp*t.length();
	} else beams[pos].L=length;
	beams[pos].k=spring;
	beams[pos].d=damp;
	beams[pos].broken=0;
	beams[pos].Lhydro=beams[pos].L;
	beams[pos].refL=beams[pos].L;
	beams[pos].hydroRatio=0.0;
	beams[pos].hydroFlags=0;
	beams[pos].animFlags=0;
	beams[pos].stress=0.0;
	beams[pos].lastforce=Vector3(0,0,0);
	beams[pos].iscentering=false;
	beams[pos].isOnePressMode=0;
	beams[pos].isforcerestricted=false;
	beams[pos].autoMovingMode=0;
	beams[pos].autoMoveLock=false;
	beams[pos].pressedCenterMode=false;
	beams[pos].disabled=false;
	beams[pos].shock=0;
	if (default_deform<beam_creak) default_deform=beam_creak;
	beams[pos].default_deform=default_deform;
	beams[pos].minmaxposnegstress=default_deform;
	beams[pos].maxposstress=default_deform;
	beams[pos].maxnegstress=-default_deform;
	beams[pos].plastic_coef=default_plastic_coef;
	beams[pos].default_plastic_coef=default_plastic_coef;
	beams[pos].strength=strength;
	beams[pos].iStrength=strength;
	beams[pos].diameter=default_beam_diameter;
	beams[pos].minendmass=1.0;
	beams[pos].diameter = diameter;
	beams[pos].scale=0.0;
	if (shortbound!=-1.0)
	{
		beams[pos].bounded=1;
		beams[pos].shortbound=shortbound;
		beams[pos].longbound=longbound;

	} else beams[pos].bounded=0;

	/*
	if (beams[pos].L<0.01)
	{
		LogManager::getSingleton().logMessage("Error: beam "+StringConverter::toString(pos)+" is too short ("+StringConverter::toString(beams[pos].L)+"m)");
		LogManager::getSingleton().logMessage("Error: beam "+StringConverter::toString(pos)+" is between node "+StringConverter::toString(beams[pos].p1->id)+" and node "+StringConverter::toString(beams[pos].p2->id)+".");
		// this causes crash to desktop in MP!
		//exit(8);
	};
	*/

	//        if (type!=BEAM_VIRTUAL && type!=BEAM_INVISIBLE)
	if (type!=BEAM_VIRTUAL)
	{
		//setup visuals
		//the cube is 100x100x100
		char bname[255];
		sprintf(bname, "beam-%s-%i", truckname, pos);
		try
		{
			beams[pos].mEntity = manager->createEntity(bname, "beam.mesh");
		}catch(...)
		{
			LogManager::getSingleton().logMessage("error loading mesh: beam.mesh");
		}

		// no materialmapping for beams!
		//		ec->setCastShadows(false);
		
		if (beams[pos].mEntity && (type==BEAM_HYDRO || type==BEAM_MARKED))
			beams[pos].mEntity->setMaterialName("tracks/Chrome");
		else if(beams[pos].mEntity)
			beams[pos].mEntity->setMaterialName(default_beam_material);
		beams[pos].mSceneNode = beamsRoot->createChildSceneNode();
		//            beams[pos].mSceneNode->attachObject(ec);
		//            beams[pos].mSceneNode->setScale(default_beam_diameter/100.0,length/100.0,default_beam_diameter/100.0);
		beams[pos].mSceneNode->setScale(beams[pos].diameter, length, beams[pos].diameter);

		// colourize beams in simple mode
		ColourValue c = ColourValue::Blue;
		if(type == BEAM_HYDRO)
			c = ColourValue::Red;
		else if(type == BEAM_HYDRO)
			c = ColourValue::Red;
		MaterialFunctionMapper::replaceSimpleMeshMaterials(beams[pos].mEntity, c);

	}
	else {beams[pos].mSceneNode=0;beams[pos].mEntity=0;};
	if (beams[pos].mSceneNode && beams[pos].mEntity && !(type==BEAM_VIRTUAL || type==BEAM_INVISIBLE || type==BEAM_INVISIBLE_HYDRO)) beams[pos].mSceneNode->attachObject(beams[pos].mEntity);//beams[pos].mSceneNode->setVisible(0);

	free_beam++;
	return pos;
}

void Beam::resetAutopilot()
{
	autopilot->disconnect();
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_gpws_but")->setMaterialName("tracks/gpws-on");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_val")->setCaption("000");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_val")->setCaption("1000");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_val")->setCaption("000");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_val")->setCaption("150");
}
void Beam::disconnectAutopilot()
{
	autopilot->disconnect();
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-off");
}


void Beam::toggleAxleLock()
{
	for(int i = 0; i < free_axle; ++i)
	{
		axles[i].toggleDiff();
	}
}

int Beam::getAxleLockCount()
{
	return free_axle;
}

Ogre::String Beam::getAxleLockName()
{
	return axles[0].getDiffTypeName();
}

void Beam::reset(bool keepPosition)
{
	reset_requested = keepPosition ? 2 : 1;
}

void Beam::SyncReset()
{
	int i;
	hydrodirstate=0.0;
	hydroaileronstate=0.0;
	hydrorudderstate=0.0;
	hydroelevatorstate=0.0;
	hydrodirwheeldisplay=0.0;
	if(hydroInertia) hydroInertia->resetCmdKeyDelay(MAX_HYDROS);
	parkingbrake=0;
	fusedrag=Vector3::ZERO;
	origin=Vector3::ZERO; //to fix
	pointCD->reset();

	Vector3 cur_position = nodes[0].AbsPosition;
	Vector3 cur_dir = nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition;
	float cur_rot = atan2(cur_dir.dotProduct(Vector3::UNIT_X), cur_dir.dotProduct(-Vector3::UNIT_Z));

	if (engine) engine->start();
	for (i=0; i<free_node; i++)
	{
		nodes[i].AbsPosition=nodes[i].iPosition;
		nodes[i].RelPosition=nodes[i].iPosition-origin;
		nodes[i].smoothpos=nodes[i].iPosition;
		nodes[i].Velocity=Vector3::ZERO;
		nodes[i].Forces=Vector3::ZERO;
		nodes[i].lastdrag=Vector3::ZERO;
		nodes[i].buoyanceForce=Vector3::ZERO;
		nodes[i].lastdrag=Vector3::ZERO;
		//this is problematic, we should also find what is locked to this, and unlock it
		nodes[i].lockednode=0;
		nodes[i].isSkin=nodes[i].iIsSkin;
	}

	for (i=0; i<free_beam; i++)
	{
		beams[i].broken=0;
		beams[i].maxposstress=beams[i].default_deform;
		beams[i].maxnegstress=-beams[i].default_deform;
		beams[i].minmaxposnegstress=beams[i].default_deform;
		beams[i].strength=beams[i].iStrength;
		beams[i].plastic_coef=beams[i].default_plastic_coef;
		beams[i].L=beams[i].refL;
		beams[i].lastforce=Vector3::ZERO;
		beams[i].stress=0.0;
		beams[i].disabled=false;
		if (beams[i].mSceneNode && beams[i].type!=BEAM_VIRTUAL && beams[i].type!=BEAM_INVISIBLE && beams[i].type!=BEAM_INVISIBLE_HYDRO)
		{
			//reattach possibly detached nodes
			//beams[i].mSceneNode->setVisible(true);
			if (beams[i].mSceneNode->numAttachedObjects()==0) beams[i].mSceneNode->attachObject(beams[i].mEntity);
		}
	}
	for (i=0; i<free_contacter; i++) contacters[i].contacted=0;
	for(std::vector <rope_t>::iterator it = ropes.begin(); it!=ropes.end(); it++) it->lockedto=0;
	for(std::vector <tie_t>::iterator it = ties.begin(); it!=ties.end(); it++)
	{
		it->beam->disabled=true;
		it->beam->mSceneNode->detachAllObjects();
	}
	for (i=0; i<free_aeroengine; i++) aeroengines[i]->reset();
	for (i=0; i<free_screwprop; i++) screwprops[i]->reset();
	for (i=0; i<free_rotator; i++) rotators[i].angle=0;
	for (i=0; i<free_wing; i++) wings[i].fa->broken=false;
	for (i=0; i<free_wheel; i++) wheels[i].speed=0.0;
	if (buoyance) buoyance->setsink(0);
	refpressure=50.0;
	addPressure(0);
	if (autopilot) resetAutopilot();
	for (i=0; i<free_flexbody; i++) flexbodies[i]->reset();

	if (reset_requested == 2)
	{
		resetAngle(cur_rot);
		resetPosition(cur_position.x, cur_position.z, false);
	}

	resetSlideNodes();
	reset_requested=0;
}

//this is called by the threads
void Beam::threadentry(int id)
{
	if (thread_mode==THREAD_HT)
	{
		int steps,i;
		float dt;
		Beam **trucks;
		int numtrucks;
		steps=tsteps;
		dt=tdt;
		trucks=ttrucks;
		numtrucks=tnumtrucks;
		float dtperstep=dt/(Real)steps;

		for (i=0; i<steps; i++)
		{
			int t;
			for (t=0; t<numtrucks; t++)
			{
				if(!trucks[t]) continue;
				//engine update
				//				if (trucks[t]->engine) trucks[t]->engine->update(dt/(Real)steps, i==0);
				if (trucks[t]->state!=SLEEPING && trucks[t]->state!=NETWORKED && trucks[t]->state!=RECYCLE)
				{
					trucks[t]->calcForcesEuler(i==0, dtperstep, i, steps, trucks, numtrucks);
					//trucks[t]->position=trucks[t]->aposition;
				}
			}
			truckTruckCollisions(dtperstep, trucks, numtrucks);
		}
		ffforce=affforce/steps;
		ffhydro=affhydro/steps;
		if (free_hydro) ffhydro=ffhydro/free_hydro;

	} else if (thread_mode==THREAD_HT2)
	{
		// work on 'this'
		if (this->state==SLEEPING || this->state==NETWORKED || this->state==RECYCLE)
		{
			debugText = "";
			return;
		}
		float dtperstep = tdt / (Real)tsteps;

		for (int i=0; i<tsteps; i++)
		{
			this->calcForcesEuler(i==0, dtperstep, i, tsteps, ttrucks, tnumtrucks);
		}
		ffforce = affforce / tsteps;
		ffhydro = affhydro / tsteps;
		if (free_hydro) ffhydro = ffhydro / free_hydro;
	}
}

//integration loop
//bool frameStarted(const FrameEvent& evt)
//this will be called once by frame and is responsible for animation of all the trucks!
//the instance called is the one of the current ACTIVATED truck
bool Beam::frameStep(Real dt, Beam** trucks, int numtrucks)
{
	/*LogManager::getSingleton().logMessage("BEAM: frame starting dt="+StringConverter::toString(dt)
	+"minx"+StringConverter::toString(minx)
	+"maxx"+StringConverter::toString(maxx)
	+"miny"+StringConverter::toString(miny)
	+"maxy"+StringConverter::toString(maxy)
	+"minz"+StringConverter::toString(minz)
	+"maxz"+StringConverter::toString(maxz)
	);
	*/
	if (!loading_finished) return true;
	if (state==SLEEPING || state==NETWORKED || state==RECYCLE) return true;
	if (dt==0) return true;
	if(mTimeUntilNextToggle>-1)
		mTimeUntilNextToggle-= dt;

	abs_timer += dt;
	if(abs_timer > 0.5f)
	{
		abs_state = !abs_state;
		abs_timer = 0.0f;
	}

	int i;
	int steps=(int)(2000.0*dt);
	if (steps>100) steps=100;
	if (dt>1.0/20.0)
	{
		dt=1.0/20.0;
		debugText="RT - Fasttrack: "+StringConverter::toString(fasted*100/(fasted+slowed))+"% "+StringConverter::toString(steps)+" steps";
	}
	else
	{
		debugText="SL - Fasttrack: "+StringConverter::toString(fasted*100/(fasted+slowed))+"% "+StringConverter::toString(steps)+" steps";
	};
	//update visual - antishaking
	//	int t;
	//	for (t=0; t<numtrucks; t++)
	//	{
	//		if (trucks[t]->state!=SLEEPING) trucks[t]->updateVisual();
	//trucks[t]->updateFlares();
	//	}

//if (free_aeroengine) debugText=String(aeroengines[0]->debug);
//debugText="Origin: "+StringConverter::toString(origin.x)+", "+StringConverter::toString(origin.y)+", "+StringConverter::toString(origin.z);


	// some scripting stuff:
#ifdef USE_ANGELSCRIPT
#if 0
	// XXX: TO BE FIXED
	if(lockedold != locked)
	{
		if(locked == LOCKED) ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_LOCKED, trucknum);
		if(locked == UNLOCKED) ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_UNLOCKED, trucknum);
		lockedold=locked;
	}
#endif //0
	if(watercontact != watercontactold)
	{
		ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_TOUCHED_WATER, trucknum);
		watercontactold = watercontact;
	}
#endif

	// send data via network?
	//if(networking)
	//	sendStreamData();


	fasted=1;
	slowed=1;
	stabsleep-=dt;
	if (replaymode && replay)
	{
		// no replay update needed if position was not changed
		if(replaypos != oldreplaypos)
		{
			unsigned long time=0;
			//replay update
			node_simple_t *nbuff = (node_simple_t *)replay->getReadBuffer(replaypos, 0, time);
			Vector3 pos = Vector3::ZERO;
			for (i=0; i<free_node; i++)
			{
				nodes[i].AbsPosition = nbuff[i].pos;
				nodes[i].RelPosition = nbuff[i].pos - origin;
				nodes[i].smoothpos = nbuff[i].pos;
				pos = pos + nbuff[i].pos;
			}
			updateSlideNodePositions();

			position=pos/(float)(free_node);
			// now beams
			beam_simple_t *bbuff = (beam_simple_t *)replay->getReadBuffer(replaypos, 1, time);
			for (i=0; i<free_beam; i++)
			{
				beams[i].scale = bbuff[i].scale;
				beams[i].broken = bbuff[i].broken;
				beams[i].disabled = bbuff[i].disabled;
			}
			//LogManager::getSingleton().logMessage("replay: " + StringConverter::toString(time));
			oldreplaypos = replaypos;
		}
	}
	else
	{
		//simulation update
		int t;

		if (thread_mode==THREAD_MONO)
		{
			ttdt=tdt;
			tdt=dt;
			float dtperstep=dt/(Real)steps;

			for (t=0; t<numtrucks; t++)
			{
				if(!trucks[t]) continue;
				trucks[t]->lastlastposition=trucks[t]->lastposition;
				trucks[t]->lastposition=trucks[t]->position;
			}
			for (i=0; i<steps; i++)
			{
				int t;
				for (t=0; t<numtrucks; t++)
				{
					if(!trucks[t]) continue;
					//engine update
					//							if (trucks[t]->engine) trucks[t]->engine->update(dt/(Real)steps, i==0);
					if (trucks[t]->state!=SLEEPING && trucks[t]->state!=NETWORKED && trucks[t]->state!=RECYCLE)
					{
						trucks[t]->calcForcesEuler(i==0, dtperstep, i, steps, trucks, numtrucks);
//							trucks[t]->position=trucks[t]->aposition;
					}
				}
				truckTruckCollisions(dtperstep, trucks, numtrucks);
			}
			//smooth
			for (t=0; t<numtrucks; t++)
			{
				if(!trucks[t]) continue;
				if (trucks[t]->reset_requested) trucks[t]->SyncReset();
				if (trucks[t]->state!=SLEEPING && trucks[t]->state!=NETWORKED && trucks[t]->state!=RECYCLE)
				{
					// calculate average position
					Vector3 aposition=Vector3::ZERO;
					int nodesnum=0;
					for (int n=0; n<trucks[t]->free_node; n++)
					{
//							trucks[t]->nodes[n].smoothpos=trucks[t]->nodes[n].tsmooth/steps;
							trucks[t]->nodes[n].smoothpos = trucks[t]->nodes[n].AbsPosition;
							if((trucks[t]->nodes[n].AbsPosition - trucks[t]->nodes[0].AbsPosition).squaredLength() > trucks[t]->nodes[n].iDistance * 25)
							{
								// loose node, ignore ...
							} else
							{
								// valid node
								aposition += trucks[t]->nodes[n].smoothpos;
								nodesnum++;
							}
//							trucks[t]->nodes[n].tsmooth=Vector3::ZERO;
					}
					trucks[t]->position = aposition / nodesnum;
				}
				if (floating_origin_enable && trucks[t]->nodes[0].RelPosition.length()>100.0)
				{
					trucks[t]->moveOrigin(trucks[t]->nodes[0].RelPosition);
				}
			}

			ffforce=affforce/steps;
			ffhydro=affhydro/steps;
			if (free_hydro) ffhydro=ffhydro/free_hydro;

		} else if (thread_mode==THREAD_HT)
		{
			//block until all threads done
			pthread_mutex_lock(&done_count_mutex);
			while (done_count>0)
				pthread_cond_wait(&done_count_cv, &done_count_mutex);
			pthread_mutex_unlock(&done_count_mutex);

			for (t=0; t<numtrucks; t++)
			{
				if(!trucks[t]) continue;
				trucks[t]->lastlastposition=trucks[t]->lastposition;
				trucks[t]->lastposition=trucks[t]->position;
			}

			//smooth
			for (t=0; t<numtrucks; t++)
			{
				if (!trucks[t]) continue;
				if (trucks[t]->reset_requested) trucks[t]->SyncReset();
				if (trucks[t]->state!=SLEEPING && trucks[t]->state!=NETWORKED && trucks[t]->state!=RECYCLE)
				{
					// average position
					Vector3 aposition=Vector3::ZERO;
					int nodesnum=0;
					for (int n=0; n<trucks[t]->free_node; n++)
					{
						trucks[t]->nodes[n].smoothpos=trucks[t]->nodes[n].AbsPosition;
						aposition += trucks[t]->nodes[n].smoothpos;
					}
					trucks[t]->position = aposition / free_node;
				}
				if (floating_origin_enable && trucks[t]->nodes[0].RelPosition.length()>100.0)
				{
					trucks[t]->moveOrigin(trucks[t]->nodes[0].RelPosition);
				}
			}

			tsteps=steps;
			ttdt=tdt;
			tdt=dt;
			ttrucks=trucks;
			tnumtrucks=numtrucks;
			//preparing workdone
			pthread_mutex_lock(&done_count_mutex);
			done_count=thread_mode;
			pthread_mutex_unlock(&done_count_mutex);

			//unblock threads
			pthread_mutex_lock(&work_mutex);
			pthread_cond_broadcast(&work_cv);
			pthread_mutex_unlock(&work_mutex);

		} else if (thread_mode==THREAD_HT2)
		{
			// just for this truck
			lastlastposition = lastposition;
			lastposition = position;

			if (reset_requested) SyncReset();

			if (state!=SLEEPING && state!=NETWORKED && state!=RECYCLE)
			{
				// average position
				Vector3 aposition=Vector3::ZERO;
				int nodesnum=0;
				for (int n=0; n<free_node; n++)
				{
					nodes[n].smoothpos=nodes[n].AbsPosition;
					aposition += nodes[n].smoothpos;
				}
				position = aposition / free_node;
			}
			if (floating_origin_enable && nodes[0].RelPosition.length()>100.0)
			{
				moveOrigin(nodes[0].RelPosition);
			}

			tsteps=steps;
			ttdt=tdt;
			tdt=dt;
			ttrucks=trucks;
			tnumtrucks=numtrucks;
		}


#ifdef FEAT_TIMING
		if(statistics)
			statistics->frameStep(dt);
#endif
		//we must take care of this
		for (int t=0; t<numtrucks; t++)
		{
			if(!trucks[t]) continue;
			//synchronous sleep
			if (trucks[t]->state==GOSLEEP) trucks[t]->state=SLEEPING;
			if (trucks[t]->state==DESACTIVATED)
			{
				trucks[t]->sleepcount++;
				if ((trucks[t]->lastposition-trucks[t]->lastlastposition).length()/dt>0.1) trucks[t]->sleepcount=7;
				if (trucks[t]->sleepcount>10) {trucks[t]->state=MAYSLEEP;trucks[t]->sleepcount=0;};
			}
		}
	}
	return true;
}

void Beam::prepareShutdown()
{
	if (thread_mode==THREAD_HT)
	{
		//block until all threads done
		pthread_mutex_lock(&done_count_mutex);
		while (done_count>0)
			pthread_cond_wait(&done_count_cv, &done_count_mutex);
		pthread_mutex_unlock(&done_count_mutex);
	};

}

void Beam::sendStreamSetup()
{
	if(!net || state == NETWORKED ) return;
	// only init stream if its local.
	// the stream is local when networking=true and networked=false

	// register the local stream
	stream_register_trucks_t reg;
	reg.status = 0;
	reg.type = 0; // 0 = truck
	reg.bufferSize = netbuffersize;
	strcpy(reg.name, realtruckfilename.c_str());

	NetworkStreamManager::getSingleton().addLocalStream(this, (stream_register_t *)&reg, sizeof(reg));
}

void Beam::sendStreamData()
{
#ifdef USE_SOCKETW
	int t = netTimer.getMilliseconds();
	if (t-last_net_time < 100)
		return;

	last_net_time = t;

	//look if the packet is too big first
	int final_packet_size = sizeof(oob_t) + sizeof(float) * 3 + first_wheel_node * sizeof(float) * 3 + free_wheel * sizeof(float);
	if(final_packet_size > (int)maxPacketLen)
	{
		showError("Truck is too big to be send over the net.", "Network error!");
		exit(126);
	}

	char send_buffer[maxPacketLen];
	memset(send_buffer, 0, maxPacketLen);

	unsigned int packet_len = 0;

	// oob_t is at the beginning of the buffer
	{
		oob_t *send_oob = (oob_t *)send_buffer;
		packet_len += sizeof(oob_t);

		send_oob->time = Network::getNetTime();
		if (engine)
		{
			send_oob->engine_speed=engine->getRPM();
			send_oob->engine_force=engine->getAcc();
		}
		if(free_aeroengine>0)
		{
			float rpm =aeroengines[0]->getRPM();
			send_oob->engine_speed=rpm;
		}

		send_oob->flagmask = 0;

		blinktype b = getBlinkType();
		if (b == BLINK_LEFT)            send_oob->flagmask+=NETMASK_BLINK_LEFT;
		else if (b == BLINK_RIGHT)      send_oob->flagmask+=NETMASK_BLINK_RIGHT;
		else if (b == BLINK_WARN)       send_oob->flagmask+=NETMASK_BLINK_WARN;

		if (lights)                     send_oob->flagmask+=NETMASK_LIGHTS;
		if (getCustomLightVisible(0))   send_oob->flagmask+=NETMASK_CLIGHT1;
		if (getCustomLightVisible(1))   send_oob->flagmask+=NETMASK_CLIGHT2;
		if (getCustomLightVisible(2))   send_oob->flagmask+=NETMASK_CLIGHT3;
		if (getCustomLightVisible(3))   send_oob->flagmask+=NETMASK_CLIGHT4;

		if (getBrakeLightVisible())		send_oob->flagmask+=NETMASK_BRAKES;
		if (getReverseLightVisible())	send_oob->flagmask+=NETMASK_REVERSE;
		if (getBeaconMode())			send_oob->flagmask+=NETMASK_BEACONS;
		if (getCustomParticleMode())    send_oob->flagmask+=NETMASK_PARTICLE;
#ifdef USE_OPENAL
		if (ssm->getTrigState(trucknum, SS_TRIG_HORN)) send_oob->flagmask+=NETMASK_HORN;
#endif //OPENAL
	}


	// then process the contents
	{
		float *send_nodes = (float *)(send_buffer + sizeof(oob_t));
		packet_len += netbuffersize;

		// copy data into the buffer
		int i;
		Vector3 refpos = nodes[0].AbsPosition;
		((float*)send_nodes)[0]=refpos.x;
		((float*)send_nodes)[1]=refpos.y;
		((float*)send_nodes)[2]=refpos.z;
		short *sbuf=(short*)(send_buffer + sizeof(oob_t)+4*3); // plus 3 floats from above
		for (i=1; i<first_wheel_node; i++)
		{
			Vector3 relpos=nodes[i].AbsPosition-refpos;
			sbuf[(i-1)*3]   = (short int)(relpos.x*300.0);
			sbuf[(i-1)*3+1] = (short int)(relpos.y*300.0);
			sbuf[(i-1)*3+2] = (short int)(relpos.z*300.0);
		}
		float *wfbuf=(float*)(send_nodes+nodebuffersize);
		for (i=0; i<free_wheel; i++)
		{
			wfbuf[i]=wheels[i].rp;
		}
	}

	//memcpy(send_buffer+sizeof(oob_t), (char*)send_buffer, send_buffer_len);
	this->addPacket(MSG2_STREAM_DATA, packet_len, send_buffer);
#endif //SOCKETW
}

void Beam::receiveStreamData(unsigned int &type, int &source, unsigned int &_streamid, char *buffer, unsigned int &len)
{
	if(state != NETWORKED) return; // this should not happen
	// TODO: FIX
	//if(this->source != source || this->streamid != streamid) return; // data not for us

	if(type == MSG2_STREAM_DATA
	   && source == (int)this->sourceid
	   && _streamid == this->streamid
	  )
	{
		pushNetwork(buffer, len);
	}
}

void Beam::calcAnimators(int flagstate, float &cstate, int &div, Real timer, float opt1, float opt2, float opt3)
{
	int flag_state=flagstate;
	Real dt = timer;
	float option1 = opt1;
	float option2 = opt2;
	float option3 = opt3;

	//boat rudder
	if (flag_state & ANIM_FLAG_BRUDDER)
	{
		int spi;
		float ctmp = 0.0f;
		for (spi=0; spi<free_screwprop; spi++)
			if(screwprops[spi]) ctmp += screwprops[spi]->getRudder();

		if (spi > 0) ctmp = ctmp / spi;
		cstate = ctmp;
		div++;
	}

	//boat throttle
	if (flag_state & ANIM_FLAG_BTHROTTLE)
	{
		int spi;
		float ctmp = 0.0f;
		for (spi=0; spi<free_screwprop; spi++)
			if(screwprops[spi]) ctmp += screwprops[spi]->getThrotle();

		if (spi > 0) ctmp = ctmp / spi;
		cstate = ctmp;
		div++;
	}

	//differential lock status
	if (flag_state & ANIM_FLAG_DIFFLOCK)
	{
		if(free_axle)
		{
			if (getAxleLockName() == "Open") cstate = 0.0f;
			if (getAxleLockName() == "Split") cstate = 0.5f;
			if (getAxleLockName() == "Locked") cstate = 1.0f;
		} else  // no axles/diffs avail, mode is split by default
			cstate=0.5f;  

		div++;
	}

	//heading
	if (flag_state & ANIM_FLAG_HEADING)
	{
		float heading = getHeadingDirectionAngle();
		// rad2deg limitedrange  -1 to +1
		cstate = (heading * 57.29578f) / 360.0f;
		div++;
	}

	//torque
	if (flag_state & ANIM_FLAG_TORQUE)
	{
		float torque=engine->getCrankFactor();
		if (torque <= 0.0f) torque = 0.0f;
		if (torque >= previousCrank)
			cstate -= torque / 10.0f;
		else
			cstate = 0.0f;

		if (cstate <= -1.0f) cstate = -1.0f;
		previousCrank = torque;
		div++;
	}

	//shifterseq, to amimate sequentiell shifting
	if (flag_state & ANIM_FLAG_SHIFTER && option3 == 3.0f)
	{
	// opt1 &opt2 = 0   this is a shifter
		if (!option1 &&  !option2)
		{
			int shifter=engine->getGear();
			if (shifter > previousGear)
			{
				cstate = 1.0f;
				animTimer = 0.2f;
			}
			if (shifter < previousGear)
			{
				cstate = -1.0f;
				animTimer = -0.2f;
			}
			previousGear = shifter;

			if (animTimer > 0.0f)
			{
				cstate = 1.0f;
				animTimer -= dt;
				if (animTimer < 0.0f)
					animTimer = 0.0f;
			}
			if (animTimer < 0.0f)
			{
				cstate = -1.0f;
				animTimer += dt;
				if (animTimer > 0.0f)
					animTimer = 0.0f;
			}
		} else
		{
			// check if option1 is a valid to get commandvalue, then get commandvalue
			if (option1 >= 1.0f && option1 <= 48.0)
				if (commandkey[int(option1)].commandValue > 0) cstate += 1.0f;
			// check if option2 is a valid to get commandvalue, then get commandvalue
			if (option2 >= 1.0f && option2 <= 48.0)
				if (commandkey[int(option2)].commandValue > 0) cstate -= 1.0f;
		}

		div++;
	}

	//shifterman1, left/right
	if (flag_state & ANIM_FLAG_SHIFTER && option3 == 1.0f)
	{
		int shifter=engine->getGear();
		if (!shifter)
		{
			cstate = -0.5f;
		} else
		if (shifter < 0)
		{
			cstate = 1.0f;
		} else
		{
			cstate -= int((shifter - 1.0) / 2.0);
		}
		div++;
	}

	//shifterman2, up/down
	if (flag_state & ANIM_FLAG_SHIFTER && option3 == 2.0f)
	{
		int shifter=engine->getGear();
		cstate = 0.5f;
		if (shifter < 0)
		{
			cstate = 1.0f;
		}
		if (shifter > 0)
		{
			// this is a really crappy if line, but somehow if (shifter/2 == int(shifter/2)) did not work reliable at all
			if (shifter == 2 || shifter == 4 || shifter == 6 || shifter == 8 || shifter == 10 || shifter == 12)
			{
				cstate = 0.0f;
			} else
			{
				cstate = 1.0f;
			}
		}
		div++;
	}

	//shifterlinear, to amimate cockpit gearselect gauge and autotransmission stick
	if (flag_state & ANIM_FLAG_SHIFTER && option3 == 4.0f)
	{
		int shifter=engine->getGear();
		int numgears= engine->getNumGears();
		cstate -= (shifter + 2.0) / (numgears + 2.0);
		div++;
	}

	//parking brake
	if (flag_state & ANIM_FLAG_PBRAKE)
	{
		float pbrake=parkingbrake;
		cstate -= pbrake;
		div++;
	}

	//speedo ( scales with speedomax )
	if (flag_state & ANIM_FLAG_SPEEDO)
	{
		float speedo=WheelSpeed / speedoMax;
		cstate -= speedo * 3.0f;
		div++;
	}

	//engine tacho ( scales with maxrpm, default is 3500 )
	if (flag_state & ANIM_FLAG_TACHO)
	{
		float tacho=engine->getRPM()/engine->getMaxRPM();
		cstate -= tacho;
		div++;
	}

	//turbo
	if (flag_state & ANIM_FLAG_TURBO)
	{
		float turbo=engine->getTurboPSI()*3.34;
		cstate -= turbo / 67.0f ;
		div++;
	}

	//brake
	if (flag_state & ANIM_FLAG_BRAKE)
	{
		float brakes=brake/brakeforce;
		cstate -= brakes;
		div++;
	}

	//accelerator
	if (flag_state & ANIM_FLAG_ACCEL)
	{
		float accel=engine->getAcc();
		cstate -= accel + 0.06f;
		//( small correction, get acc is nver smaller then 0.06.
		div++;
	}

		//clutch
	if (flag_state & ANIM_FLAG_CLUTCH)
	{
		float clutch=engine->getClutch();
		cstate -= abs(1.0f - clutch);
		div++;
	}

	//aeroengines rpm + throttle + torque ( turboprop ) + pitch ( turboprop ) + status +  fire
	int ftp=free_aeroengine;

	if (ftp > option3 - 1.0f)
	{
		int aenum = int(option3 - 1.0f);
		if (flag_state & ANIM_FLAG_RPM)
		{
			float angle;
			float pcent=aeroengines[aenum]->getRPMpc();
			if (pcent<60.0) angle=-5.0+pcent*1.9167;
			else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
			else angle=314.0;
			cstate -= angle / 314.0f;
			div++;
		}
		if (flag_state & ANIM_FLAG_THROTTLE)
		{
			float throttle=aeroengines[aenum]->getThrotle();
			cstate -= throttle;
			div++;
		}

		if (flag_state & ANIM_FLAG_AETORQUE)
			if (aeroengines[aenum]->getType()==AEROENGINE_TYPE_TURBOPROP)
			{
				Turboprop *tp=(Turboprop*)aeroengines[aenum];
				cstate=(100.0*tp->indicated_torque/tp->max_torque) / 120.0f;
				div++;
			}

		if (flag_state & ANIM_FLAG_AEPITCH)
			if (aeroengines[aenum]->getType()==AEROENGINE_TYPE_TURBOPROP)
			{
				Turboprop *tp=(Turboprop*)aeroengines[aenum];
				cstate=tp->pitch / 120.0f;
				div++;
			}

		if (flag_state & ANIM_FLAG_AESTATUS)
		{
 			if (!aeroengines[aenum]->getIgnition()) 
				cstate = 0.0f;
			else 
				cstate = 0.5f;
			if (aeroengines[aenum]->isFailed()) cstate = 1.0f;
			div++;
		}
	}

	//airspeed indicator
	if (flag_state & ANIM_FLAG_AIRSPEED)
	{
		float angle=0.0;
		float ground_speed_kt= nodes[0].Velocity.length()*1.9438;
		float altitude=nodes[0].AbsPosition.y;
		float sea_level_temperature=273.15+15.0; //in Kelvin
		float sea_level_pressure=101325; //in Pa
		float airtemperature=sea_level_temperature-altitude*0.0065; //in Kelvin
		float airpressure=sea_level_pressure*pow(1.0-0.0065*altitude/288.15, 5.24947); //in Pa
		float airdensity=airpressure*0.0000120896;//1.225 at sea level
		float kt=ground_speed_kt*sqrt(airdensity/1.225);
		cstate -= kt / 100.0f;
		div++;
	}

	//vvi indicator
	if (flag_state & ANIM_FLAG_VVI)
	{
		float vvi=nodes[0].Velocity.y*196.85;
		// limit vvi scale to +/- 6m/s
		cstate -=vvi / 6000.0f;
		if (cstate >= 1.0f) cstate = 1.0f;
		if (cstate <= -1.0f) cstate = -1.0f;
		div++;
	}

	//altimeter
	if (flag_state & ANIM_FLAG_ALTIMETER)
	{
		//altimeter indicator 1k oscillating
		if (option3 == 3.0f)
		{
			float altimeter = (nodes[0].AbsPosition.y*1.1811) / 360.0f;
			int alti_int = int(altimeter);
			float alti_mod= (altimeter - alti_int);
			cstate -= alti_mod;
		}

		//altimeter indicator 10k oscillating
		if (option3 == 2.0f)
		{
			float alti=nodes[0].AbsPosition.y*1.1811 / 3600.0f;
			int alti_int = int(alti);
			float alti_mod= (alti - alti_int);
			cstate -= alti_mod;
			if (cstate <= -1.0f) cstate = -1.0f;
		}

		//altimeter indicator 100k limited
		if (option3 == 1.0f)
		{
			float alti=nodes[0].AbsPosition.y*1.1811  / 36000.0f;
			cstate -= alti;
			if (cstate <= -1.0f) cstate = -1.0f;
		}
		div++;
	}

	//AOA
	if (flag_state & ANIM_FLAG_AOA)
	{
		float aoa=0;
		if (free_wing>4) aoa=(wings[4].fa->aoa) / 25.0f;
		if ((nodes[0].Velocity.length()*1.9438) < 10.0f) aoa=0;
		cstate -= aoa;
		if (cstate <= -1.0f) cstate = -1.0f;
		if (cstate >= 1.0f) cstate = 1.0f;
		div++;
	}

	//roll
	if (flag_state & ANIM_FLAG_ROLL)
	{
		Vector3 rollv=nodes[cameranodepos[0]].RelPosition-nodes[cameranoderoll[0]].RelPosition;
		rollv.normalise();
		float rollangle=asin(rollv.dotProduct(Vector3::UNIT_Y));
		Vector3 dirv=nodes[cameranodepos[0]].RelPosition-nodes[cameranodedir[0]].RelPosition;
		Vector3 upv=dirv.crossProduct(-rollv);
		// rad to deg
		rollangle = (rollangle * 57.2957795f);
		//flip to other side when upside down
		if (upv.y<0) rollangle= 180.0f - rollangle;
		cstate = rollangle / 180.0f;
		// dataoutpu is -0.5 to 1.5, normalize to -1 to +1 without changing the zero position. 
		// this is vital for the animateor beams and does not effect the animated props
		if (cstate >= 1.0f) cstate = cstate - 2.0f;
		div++;
	}

	//pitch
	if (flag_state & ANIM_FLAG_PITCH)
	{
		Vector3 dirv=nodes[cameranodepos[0]].RelPosition-nodes[cameranodedir[0]].RelPosition;
		dirv.normalise();
		float pitchangle=asin(dirv.dotProduct(Vector3::UNIT_Y));
		//radian to degrees with a max cstate of +/- 1.0
		cstate = (( pitchangle * 57.29578f )  / 90.0f );
		div++;
	}

	//airbrake
	if (flag_state & ANIM_FLAG_AIRBRAKE)
	{
		float airbrake=airbrakeval;
		// cstate limited to -1.0f
		cstate -= airbrake / 5.0f;
		div++;
	}

	//flaps
	if (flag_state & ANIM_FLAG_FLAP)
	{
		float flaps=flapangles[flap];
		// cstate limited to -1.0f
		cstate = flaps;
		div++;
	}

}


void Beam::calcShocks2(int beam_i, Real difftoBeamL, Real &k, Real &d)
{
	int i=beam_i;
	float beamsLep=beams[i].L*0.8f;
	float longboundprelimit=beams[i].longbound*beamsLep;
	float shortboundprelimit=-beams[i].shortbound*beamsLep;
	// this is a shock2
	float logafactor;
	//shock extending since last cycle
	if (beams[i].shock->lastpos < difftoBeamL)
	{
		//get outbound values
		k=beams[i].shock->springout;
		d=beams[i].shock->dampout;
		// add progression
		if (beams[i].longbound != 0.0f)
		{
			logafactor=difftoBeamL/(beams[i].longbound*beams[i].L);
			logafactor=logafactor*logafactor;
		} else
		{
			logafactor = 1.0f;
		}
		if (logafactor > 1.0f) logafactor = 1.0f;
		k=k+(beams[i].shock->sprogout*k*logafactor);
		d=d+(beams[i].shock->dprogout*d*logafactor);
	} else
	{
		//shock compresssing since last cycle
		//get inbound values
		k=beams[i].shock->springin;
		d=beams[i].shock->dampin;
		// add progression
		if (beams[i].shortbound != 0.0f)
		{
			logafactor=difftoBeamL/(beams[i].shortbound*beams[i].L);
			logafactor=logafactor*logafactor;
		} else
		{
			logafactor = 1.0f;
		}
		if (logafactor > 1.0f) logafactor = 1.0f;
		k=k+(beams[i].shock->sprogin*k*logafactor);
		d=d+(beams[i].shock->dprogin*d*logafactor);
	}
	if(beams[i].shock->flags & SHOCK_FLAG_SOFTBUMP)
	{
		// soft bump shocks
		if (difftoBeamL > longboundprelimit)
		{
			//reset to longbound progressive values (oscillating beam workaround)
			k=beams[i].shock->springout;
			d=beams[i].shock->dampout;
			// add progression
			if (beams[i].longbound != 0.0f)
			{
				logafactor=difftoBeamL/(beams[i].longbound*beams[i].L);
				logafactor=logafactor*logafactor;
			} else
			{
				logafactor = 1.0f;
			}
			if (logafactor > 1.0f) logafactor = 1.0f;
			k=k+(beams[i].shock->sprogout*k*logafactor);
			d=d+(beams[i].shock->dprogout*d*logafactor);
			//add shortbump progression
			if (beams[i].longbound != 0.0f)
			{
				logafactor=((difftoBeamL-longboundprelimit)*5.0f)/(beams[i].longbound*beams[i].L);
				logafactor=logafactor*logafactor;
			} else
			{
				logafactor = 1.0f;
			}
			if (logafactor > 1.0f) logafactor = 1.0f;
			k=k+(k+ 100.0f)* beams[i].shock->sprogout *logafactor;
			d=d+(d+ 100.0f)* beams[i].shock->dprogout * logafactor;
			if (beams[i].shock->lastpos > difftoBeamL)
			// rebound mode..get new values
			{
				k=beams[i].shock->springin;
				d=beams[i].shock->dampin;
			}
		} else if (difftoBeamL < shortboundprelimit)
		{
			//reset to shortbound progressive values (oscillating beam workaround)
			k=beams[i].shock->springin;
			d=beams[i].shock->dampin;
			if (beams[i].shortbound != 0.0f)
			{
				logafactor=difftoBeamL/(beams[i].shortbound*beams[i].L);
				logafactor=logafactor*logafactor;
			} else
			{
				logafactor = 1.0f;
			}
			if (logafactor > 1.0f) logafactor = 1.0f;
			k=k+(beams[i].shock->sprogin*k*logafactor);
			d=d+(beams[i].shock->dprogin*d*logafactor);
			//add shortbump progression
			if (beams[i].shortbound != 0.0f)
			{
				logafactor=((difftoBeamL-shortboundprelimit)*5.0f)/(beams[i].shortbound*beams[i].L);
				logafactor=logafactor*logafactor;
			}else
			{
				logafactor = 1.0f;
			}
			if (logafactor > 1.0f) logafactor = 1.0f;
			k=k+(k+ 100.0f)* beams[i].shock->sprogout *logafactor;
			d=d+(d+ 100.0f)* beams[i].shock->dprogout * logafactor;
			if (beams[i].shock->lastpos < difftoBeamL)
			// rebound mode..get new values
			{
				k=beams[i].shock->springout;
				d=beams[i].shock->dampout;
			}
		}
		if (difftoBeamL > beams[i].longbound*beams[i].L || difftoBeamL < -beams[i].shortbound*beams[i].L)
		{
			// block reached...hard bump in soft mode with 4x default damping
			if (k < DEFAULT_SPRING) k=DEFAULT_SPRING;
			if (d < DEFAULT_DAMP*4.0f) d = DEFAULT_DAMP*4.0f;
		}
	}

	if(beams[i].shock->flags & SHOCK_FLAG_NORMAL)
	{
		if (difftoBeamL > beams[i].longbound*beams[i].L || difftoBeamL < -beams[i].shortbound*beams[i].L)
		{
			// hard (normal) shock bump
			k=DEFAULT_SPRING;
			d=DEFAULT_DAMP;
		}
	}

	// save beam position for next sim cycle
	beams[i].shock->lastpos=difftoBeamL;
}

void Beam::calcForcesEuler(int doUpdate, Real dt, int step, int maxstep, Beam** trucks, int numtrucks)
{
	// do not calc anything if we are going to get deleted
	if(deleting) return;

	int i,j;
	if (dt==0.0) return;
	if (reset_requested) return;

	int increased_accuracy=0;
	float inverted_dt=1.0f/dt;

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStart(BeamThreadStats::WholeTruckCalc);
#endif
	//engine callback
	if (state==ACTIVATED && engine)
	{
#ifdef FEAT_TIMING
		if(statistics)
			statistics->queryStart(BeamThreadStats::TruckEngine);
#endif
		if(engine)
			engine->update(dt, doUpdate);
#ifdef FEAT_TIMING
		if(statistics)
			statistics->queryStop(BeamThreadStats::TruckEngine);
#endif
	}
	//		if (doUpdate) mWindow->setDebugText(engine->status);


#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStart(BeamThreadStats::Beams);
#endif

//springs
	Vector3 dis;
	Vector3 v;
	for (i=0; i<free_beam; i++)
	{
		//trick for exploding stuff
		if (!beams[i].disabled)
		{
			//Calculate beam length
			if (!beams[i].p2truck) {
				dis=beams[i].p1->RelPosition;
				dis-=beams[i].p2->RelPosition;
			}
			else
			{
				dis=beams[i].p1->AbsPosition;
				dis-=beams[i].p2->AbsPosition;
			}

			Real dislen=dis.squaredLength();

			Real inverted_dislen=fast_invSqrt(dislen);
			dislen=dislen*inverted_dislen;

			//Calculate beam's deviation from normal
			Real difftoBeamL = dislen - beams[i].L;

			Real k=beams[i].k;
			Real d=beams[i].d;

			if (beams[i].bounded)
			{
				// We do a binary search in bounded's values to lessen the number of ifs
				if (beams[i].bounded<=SHOCK2)
				{
					// hard (normal) shock bump
					if (beams[i].bounded==SHOCK1)
					{
						if (difftoBeamL > beams[i].longbound*beams[i].L)
						{
							// Following code interpolates between defined beam parameters and default beam parameters
							float interp_ratio=difftoBeamL-beams[i].longbound*beams[i].L;

							// hard (normal) shock bump
							k=k+(DEFAULT_SPRING-k)*interp_ratio;
							d=d+(DEFAULT_DAMP-d)*interp_ratio;
						}
						else if (difftoBeamL < -beams[i].shortbound*beams[i].L)
						{
							// Following code interpolates between defined beam parameters and default beam parameters
							float interp_ratio=-beams[i].shortbound*beams[i].L-difftoBeamL;

							// hard (normal) shock bump
							k=k+(DEFAULT_SPRING-k)*interp_ratio;
							d=d+(DEFAULT_DAMP-d)*interp_ratio;
						}
					}
					else // We assume the bounded=SHOCK2 case
					{
						calcShocks2(i, difftoBeamL, k, d);
					}
				}
				else
				{

					if (beams[i].bounded==ROPE)
					{
						if  (difftoBeamL<0.0f)
						{
							k=0.0f;
							d=d*0.1f;
						}
					}
					else // We assume the bounded=SUPPORTBEAM case
					{
						if (difftoBeamL>0.0f)
						{
							k=0.0f;
							d=d*0.1f;
							// If support beam is extended more than one beam length, break it
							if (difftoBeamL>beams[i].L)
							{
								beams[i].broken=1;
								beams[i].disabled=true;
							}
						}
					}
				}
			}

			//Calculate beam's rate of change
			v=beams[i].p1->Velocity;
			v-=beams[i].p2->Velocity;

			float flen;
			flen = -k*(difftoBeamL)-d*v.dotProduct(dis)*inverted_dislen;
			float sflen=flen;
			beams[i].stress=flen;
			flen=fabs(flen);


			// Fast test for deformation
			if (flen > beams[i].minmaxposnegstress)
			{
				if ((beams[i].type==BEAM_NORMAL || beams[i].type==BEAM_INVISIBLE) && beams[i].bounded!=SHOCK1 && k!=0.0f)
				{
					// Actual deformation tests
					// For compression
					if (sflen>beams[i].maxposstress && difftoBeamL<0.0f)
					{

						increased_accuracy=1;
						Real yield_length=beams[i].maxposstress/k;
						Real deform=difftoBeamL+yield_length*(1.0f-beams[i].plastic_coef);
						Real Lold=beams[i].L;
						beams[i].L+=deform;
						if (beams[i].L < MIN_BEAM_LENGTH) beams[i].L=MIN_BEAM_LENGTH;
						sflen=sflen-(sflen-beams[i].maxposstress)*0.5f;
						flen=sflen;
						if (beams[i].L>0.0f && beams[i].L<Lold)
						{
							beams[i].maxposstress*=Lold/beams[i].L;
						}

						//For the compression case we do not remove any of the beam's
						//strength for structure stability reasons
						//beams[i].strength=beams[i].strength+deform*k*0.5f;

						//Sound effect
						//Sound volume depends on the energy lost due to deformation (which gets converted to sound (and thermal) energy)
						/*ssm->modulate(trucknum, SS_MOD_CREAK, deform*k*(difftoBeamL+deform*0.5f));
						ssm->trigOnce(trucknum, SS_TRIG_CREAK);*/

						beams[i].minmaxposnegstress=std::min(beams[i].maxposstress, -beams[i].maxnegstress);
						beams[i].minmaxposnegstress=std::min(beams[i].minmaxposnegstress, beams[i].strength);
						if(beambreakdebug)
						{
							LogManager::getSingleton().logMessage(" YYY Beam " + StringConverter::toString(i) + " just deformed with compression force " + StringConverter::toString(flen) + " / " + StringConverter::toString(beams[i].strength) + ". It was between nodes " + StringConverter::toString(beams[i].p1->id) + " and " + StringConverter::toString(beams[i].p2->id) + ".");
						}
					} else	// For expansion
					if (sflen<beams[i].maxnegstress && difftoBeamL>0.0f)
					{
						increased_accuracy=1;
						Real yield_length=beams[i].maxnegstress/k;
						Real deform=difftoBeamL+yield_length*(1.0f-beams[i].plastic_coef);
						Real Lold=beams[i].L;
						beams[i].L+=deform;
						sflen=sflen-(sflen-beams[i].maxnegstress)*0.5f;
						flen=-sflen;
						if (Lold>0.0f && beams[i].L>Lold)
						{
							beams[i].maxnegstress*=beams[i].L/Lold;
						}
						beams[i].strength=beams[i].strength-deform*k;

						//Sound effect
						//Sound volume depends on the energy lost due to deformation (which gets converted to sound (and thermal) energy)
						/*ssm->modulate(trucknum, SS_MOD_CREAK, deform*k*(difftoBeamL+deform*0.5f));
						ssm->trigOnce(trucknum, SS_TRIG_CREAK);*/

						beams[i].minmaxposnegstress=std::min(beams[i].maxposstress, -beams[i].maxnegstress);
						beams[i].minmaxposnegstress=std::min(beams[i].minmaxposnegstress, beams[i].strength);
						if(beambreakdebug)
						{
							LogManager::getSingleton().logMessage(" YYY Beam " + StringConverter::toString(i) + " just deformed with extension force " + StringConverter::toString(flen) + " / " + StringConverter::toString(beams[i].strength) + ". It was between nodes " + StringConverter::toString(beams[i].p1->id) + " and " + StringConverter::toString(beams[i].p2->id) + ".");
						}
					}
				}

				// Test if the beam should be breaked
				if (flen > beams[i].strength)
				{
					// Sound effect.
					// Sound volume depends on spring's stored energy
#ifdef USE_OPENAL
					ssm->modulate(trucknum, SS_MOD_BREAK, 0.5*k*difftoBeamL*difftoBeamL);
					ssm->trigOnce(trucknum, SS_TRIG_BREAK);
#endif //OPENAL
					increased_accuracy=1;

					//Break the beam only when it is not connected to a node
					//which is a part of a collision triangle and has 2 "live" beams or less
					//connected to it.
					if (!((beams[i].p1->contacter && nodeBeamConnections(beams[i].p1->pos)<3) || (beams[i].p2->contacter && nodeBeamConnections(beams[i].p2->pos)<3)))
					{
						beams[i].broken=1;
						beams[i].disabled=true;
						sflen=0.0f;
						beams[i].p1->isSkin=true;
						beams[i].p2->isSkin=true;

						if(beambreakdebug)
						{
							LogManager::getSingleton().logMessage(" XXX Beam " + StringConverter::toString(i) + " just broke with force " + StringConverter::toString(flen) + " / " + StringConverter::toString(beams[i].strength) + ". It was between nodes " + StringConverter::toString(beams[i].p1->id) + " and " + StringConverter::toString(beams[i].p2->id) + ".");
						}

					} else beams[i].strength=2.0f*beams[i].minmaxposnegstress;

					//something broke, check buoyant hull
					int mk;
					for (mk=0; mk<free_buoycab; mk++)
					{
						int tmpv=buoycabs[mk]*3;
						if (buoycabtypes[mk]==BUOY_DRAGONLY) continue;
						if ((beams[i].p1==&nodes[cabs[tmpv]] || beams[i].p1==&nodes[cabs[tmpv+1]] || beams[i].p1==&nodes[cabs[tmpv+2]])
							&&(beams[i].p2==&nodes[cabs[tmpv]] || beams[i].p2==&nodes[cabs[tmpv+1]] || beams[i].p2==&nodes[cabs[tmpv+2]]))
							buoyance->setsink(1);
					}
				}
			}

			// At last update the beam forces
			Vector3 f=dis;
			f*=(sflen*inverted_dislen);
			beams[i].p1->Forces+=f;
			beams[i].p2->Forces-=f;
		}
	}

	//animate props
	for (int propi=0; propi<free_prop; propi++)
	{
		int animnum=0;
		float rx = 0.0f;
		float ry = 0.0f;
		float rz = 0.0f;

		while (props[propi].animFlags[animnum])
		{
			if (props[propi].animFlags[animnum])
			{
				float cstate = 0.0f;
				int div = 0.0f;
				int flagstate = props[propi].animFlags[animnum];
				float animOpt1 =props[propi].animOpt1[animnum];
				float animOpt2 =props[propi].animOpt2[animnum];
				float animOpt3 =props[propi].animOpt3[animnum];

				calcAnimators(flagstate, cstate, div, dt, animOpt1, animOpt2, animOpt3);

				// key triggered animations
				if ((props[propi].animFlags[animnum] & ANIM_FLAG_EVENT) && props[propi].animKey[animnum] != -1)
				{
					if (INPUTENGINE.getEventValue(props[propi].animKey[animnum]))
					{
						// keystatelock is disabled then set cstate
						if (props[propi].animKeyState[animnum] == -1.0f)
						{
							cstate += INPUTENGINE.getEventValue(props[propi].animKey[animnum]);
						} else if (!props[propi].animKeyState[animnum])
						{
							// a key was pressed and a toggle was done already, so bypass
							//toggle now
							if (!props[propi].lastanimKS[animnum])
							{
								props[propi].lastanimKS[animnum] = 1.0f;
								// use animkey as bool to determine keypress / release state of inputengine
								props[propi].animKeyState[animnum] = 1.0f;
							}
							else 
							{
								props[propi].lastanimKS[animnum] = 0.0f;
								// use animkey as bool to determine keypress / release state of inputengine
								props[propi].animKeyState[animnum] = 1.0f;
							}
						} else
						{
							// bypas mode, get the last set position and set it
							cstate +=props[propi].lastanimKS[animnum];
						}
					} else
					{
						// keyevent exists and keylock is enabled but the key isnt pressed right now = get lastanimkeystatus for cstate and reset keypressed bool animkey 
						if (props[propi].animKeyState[animnum] != -1.0f)
						{
							cstate +=props[propi].lastanimKS[animnum];
							props[propi].animKeyState[animnum] = 0.0f;
						}
					}
				}

				//propanimation placed here to avoid interference with existing hydros(cstate) and permanent prop animation
				//truck steering
				if (props[propi].animFlags[animnum] & ANIM_FLAG_STEERING) cstate += hydrodirstate;
				//aileron
				if (props[propi].animFlags[animnum] & ANIM_FLAG_AILERONS) cstate += hydroaileronstate;
				//elevator
				if (props[propi].animFlags[animnum] & ANIM_FLAG_ELEVATORS) cstate += hydroelevatorstate;
				//rudder Liftec
				if (props[propi].animFlags[animnum] & ANIM_FLAG_ARUDDER) cstate += hydrorudderstate;
				//permanent
				if (props[propi].animFlags[animnum] & ANIM_FLAG_PERMANENT) cstate += 1.0f;

				cstate *= props[propi].animratio[animnum];

				// autoanimate noflip_bouncer
				if (props[propi].animOpt5[animnum]) cstate *= (props[propi].animOpt5[animnum]);

				//rotate prop
				if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X || props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y || props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z)
				{
					float limiter = 0.0f;
					if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
					{
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X)
						{
							props[propi].rot = props[propi].rot * (Quaternion(Degree(0), Vector3::UNIT_Z) * Quaternion(Degree(0), Vector3::UNIT_Y) * Quaternion(Degree(cstate), Vector3::UNIT_X));
							props[propi].rotaX += cstate;
							limiter = props[propi].rotaX;
						}
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y)
						{
							props[propi].rot = props[propi].rot * (Quaternion(Degree(0), Vector3::UNIT_Z) * Quaternion(Degree(cstate), Vector3::UNIT_Y) * Quaternion(Degree(0), Vector3::UNIT_X));
							props[propi].rotaY += cstate;
							limiter = props[propi].rotaY;
						}
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z)
						{
							props[propi].rot = props[propi].rot * (Quaternion(Degree(cstate), Vector3::UNIT_Z) * Quaternion(Degree(0), Vector3::UNIT_Y) * Quaternion(Degree(0), Vector3::UNIT_X));
							props[propi].rotaZ += cstate;
							limiter = props[propi].rotaZ;
						}

					} else
					{
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X) rx += cstate;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y) ry += cstate;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z) rz += cstate;
					}

					bool limiterchanged = false;
					// check if a positive custom limit is set to evaluate/calc flip back
					if (props[propi].animOpt2[animnum] - props[propi].animOpt4[animnum])
					{
						if (limiter > props[propi].animOpt2[animnum])
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = props[propi].animOpt2[animnum];				// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								limiterchanged = true;
							} else
							{
								limiter = props[propi].animOpt1[animnum];				// flip to other side at limit
								limiterchanged = true;
							}
						}
					} else
					{																	// no custom limit set, use 360�
						while (limiter > 180.0f)
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = 180.0f;										// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								limiterchanged = true;
							} else 
							{
								limiter -= 360.0f;										// flip to other side at limit
								limiterchanged = true;
							}
						}
					}
	
					// check if a negative custom limit is set to evaluate/calc flip back
					if (props[propi].animOpt1[animnum] - props[propi].animOpt4[animnum])
					{
						if (limiter < (props[propi].animOpt1[animnum]))
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = props[propi].animOpt1[animnum];				// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								limiterchanged = true;
							} else
							{
								limiter = props[propi].animOpt2[animnum];				// flip to other side at limit
								limiterchanged = true;
							}
						}
					} else																// no custom limit set, use 360�
					{
						while (limiter < -180.0f)
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = -180.0f;										// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								limiterchanged = true;
							} else
							{
								limiter += 360.0f;										// flip to other side at limit including overflow
								limiterchanged = true;
							}
						}
					}
					
					if (limiterchanged)
					{
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X) props[propi].rotaX = limiter;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y) props[propi].rotaY = limiter;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z) props[propi].rotaZ = limiter;
					}
				}

				//offset prop
				float ox = props[propi].orgoffsetX;
				float oy = props[propi].orgoffsetY;
				float oz = props[propi].orgoffsetZ;

				if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X || props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y || props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z)
				{
					float offset = 0.0f;
					float autooffset = 0.0f;

					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X) offset = props[propi].orgoffsetX;
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y) offset = props[propi].orgoffsetY;
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z) offset = props[propi].orgoffsetZ;

					offset += cstate;
					if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
					{
						autooffset = offset;
																							// check if a positive custom limit is set to evaluate/calc flip back
						if (props[propi].animOpt2[animnum] - props[propi].animOpt4[animnum])
						{
							if (autooffset > props[propi].animOpt2[animnum])
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = props[propi].animOpt2[animnum];			// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								} else
									autooffset = props[propi].animOpt1[animnum];			// flip to other side at limit
							}
						} else																// no custom limit set, use 10x as default
							while (autooffset > 10.0f)
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = 10.0f;										// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								} else 
									autooffset -= 20.0f;									// flip to other side at limit including overflow
							}

																							// check if a negative custom limit is set to evaluate/calc flip back
						if (props[propi].animOpt1[animnum] - props[propi].animOpt4[animnum])
						{
							if (autooffset < (props[propi].animOpt1[animnum]))
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = props[propi].animOpt1[animnum];			// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								} else
									autooffset = props[propi].animOpt2[animnum];			// flip to other side at limit
							}
						} else																// no custom limit set, use -10x�
							while (autooffset < -10.0f)
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = -10.0f;									// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								} else 
									autooffset += 20.0f;									// flip to other side at limit including overflow
							}
					}

					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X)
					{
						props[propi].offsetx = offset;
						if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
							props[propi].orgoffsetX = autooffset;
					}
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y)
					{
						props[propi].offsety = offset;
						if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
							props[propi].orgoffsetY = autooffset;
					}
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z)
					{
						props[propi].offsetz = offset;
						if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
							props[propi].orgoffsetZ = autooffset;
					}
				}
				
			}
		animnum++;
		}
	//recalc the quaternions with final stacked rotation values ( rx, ry, rz )
	rx += props[propi].rotaX;
	ry += props[propi].rotaY;
	rz += props[propi].rotaZ;
	props[propi].rot = Quaternion(Degree(rz), Vector3::UNIT_Z) * Quaternion(Degree(ry), Vector3::UNIT_Y) * Quaternion(Degree(rx), Vector3::UNIT_X);
	}

	//skeleton colouring
	if (((skeleton && doUpdate) || replay) )
	{
		for (i=0; i<free_beam; i++)
		{
			if (!beams[i].disabled)
			{
				if (((doUpdate && skeleton == 2) || replay) && !beams[i].broken && beams[i].mEntity && beams[i].mSceneNode)
				{
					float tmp=beams[i].stress/beams[i].minmaxposnegstress;
					float sqtmp=tmp*tmp;
					beams[i].scale = (sqtmp*sqtmp)*100.0f*sign(tmp);
				}
				if (doUpdate && skeleton == 1 && !beams[i].broken && beams[i].mEntity && beams[i].mSceneNode)
				{
					int scale=(int)beams[i].scale * 100;
					if(scale>100) scale=100;
					if(scale<-100) scale=-100;
					char bname[255];
					sprintf(bname, "mat-beam-%d", scale);
					beams[i].mEntity->setMaterialName(bname);
				}
				else if(doUpdate && skeleton && beams[i].mSceneNode && (beams[i].broken || beams[i].disabled) && beams[i].mSceneNode)
				{
					beams[i].mSceneNode->detachAllObjects();
				}
			}
		}
	}

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Beams);

	if(statistics)
		statistics->queryStart(BeamThreadStats::Rigidifiers);
#endif
	//the rigidifiers
	for (i=0; i<free_rigidifier; i++)
	{
		//failsafe
		if ((rigidifiers[i].beama && rigidifiers[i].beama->broken) || (rigidifiers[i].beamc && rigidifiers[i].beamc->broken)) continue;
		//thanks to Chris Ritchey for the solution!
		Vector3 vab=rigidifiers[i].a->RelPosition-rigidifiers[i].b->RelPosition;
		Vector3 vcb=rigidifiers[i].c->RelPosition-rigidifiers[i].b->RelPosition;
		float vablen = vab.squaredLength();
		float vcblen = vcb.squaredLength();
		if (vablen == 0.0f || vcblen == 0.0f) continue;
		float inverted_vablen = fast_invSqrt(vablen);
		float inverted_vcblen = fast_invSqrt(vcblen);
		vablen = vablen*inverted_vablen;
		vcblen = vcblen*inverted_vcblen;
		vab = vab*inverted_vablen;
		vcb = vcb*inverted_vcblen;
		float vabdotvcb = vab.dotProduct(vcb);
		float alphap = vabdotvcb;
		if (alphap > 1.0f) alphap = 1.0f;
		else if (alphap<-1.0f) alphap = -1.0f;
		alphap=acos(alphap);
		float forcediv = -rigidifiers[i].k * (rigidifiers[i].alpha - alphap) + rigidifiers[i].d * (alphap - rigidifiers[i].lastalpha) * inverted_dt; //force dividend
		//forces at a
		float tmp = forcediv*inverted_vablen;
		Vector3 va = vcb * tmp - vab * (vabdotvcb * tmp);
		rigidifiers[i].a->Forces += va;
		//forces at c
		tmp = forcediv*inverted_vcblen;
		Vector3 vc = vab * tmp - vcb * (vabdotvcb * tmp);
		rigidifiers[i].c->Forces += vc;
		//reaction at b
		rigidifiers[i].b->Forces += -va - vc;
		rigidifiers[i].lastalpha=alphap;
	}
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Rigidifiers);
#endif

	//aposition=Vector3::ZERO;
	if (state==ACTIVATED) //force feedback sensors
	{
		if (doUpdate)
		{
			affforce=nodes[cameranodepos[currentcamera]].Forces;
			affhydro=0;
		}
		else
		{
			affforce+=nodes[cameranodepos[currentcamera]].Forces;
		}
		for (int i=0; i<free_hydro; i++)
		{
			if (beams[hydro[i]].hydroFlags&(HYDRO_FLAG_DIR|HYDRO_FLAG_SPEED))
				affhydro+=beams[hydro[i]].hydroRatio*beams[hydro[i]].refL*beams[hydro[i]].stress;
		}
	}


	//locks - this is not active in network mode
	for(std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
	{
		if (it->lockNode && it->locked == LOCKED)
		{
			it->lockNode->lockedPosition = it->hookNode->AbsPosition;
			it->lockNode->lockedVelocity = it->hookNode->Velocity;
			it->hookNode->Forces         = it->hookNode->Forces + it->lockNode->lockedForces;
		}
		if (it->lockNode && it->locked==PRELOCK)
		{
			//check for locking
			if ((it->hookNode->AbsPosition - it->lockNode->AbsPosition).squaredLength() < 0.00001f)
			{
				it->lockNode->lockednode=1;
				it->lockNode->lockedPosition = it->lockNode->AbsPosition;
				it->lockNode->lockedVelocity = it->lockNode->Velocity;
				it->lockNode->lockedForces   = it->lockNode->Forces;
				it->locked = LOCKED;
			}
			else
			{
				//add attraction forces
				Vector3 f = it->hookNode->AbsPosition - it->lockNode->AbsPosition;
				f.normalise();
				it->hookNode->Forces -= 100000.0 * f;
				it->lockNode->Forces += 100000.0 * f;
			}
		}
	}

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStart(BeamThreadStats::Ropes);
#endif
	if (ropes.size())
	{
		for(std::vector <rope_t>::iterator it = ropes.begin(); it!=ropes.end(); it++)
		{
			if (it->lockedto)
			{
				it->beam->p2->AbsPosition = it->lockedto->AbsPosition;
				it->beam->p2->RelPosition = it->lockedto->AbsPosition - origin; //ropes[i].lockedtruck->origin; //we have a problem here
				it->beam->p2->Velocity = it->lockedto->Velocity;
				it->lockedto->Forces = it->lockedto->Forces + it->beam->p2->Forces;
				it->beam->p2->Forces=Vector3(0,0,0);
			}
		}
	}
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Ropes);
#endif
	//mouse stuff
	if (mousenode!=-1)
	{
		Vector3 dir=mousepos-nodes[mousenode].AbsPosition;
		nodes[mousenode].Forces += mousemoveforce * dir;
	}


	// START Slidenode section /////////////////////////////////////////////////
	// these must be done before the integrator, or else the forces are not calculated properly
	updateSlideNodeForces(dt);
	// END Slidenode section   /////////////////////////////////////////////////

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStart(BeamThreadStats::Nodes);
#endif
	float tminx=nodes[0].AbsPosition.x;
	float tmaxx=tminx;
	float tminy=nodes[0].AbsPosition.y;
	float tmaxy=tminy;
	float tminz=nodes[0].AbsPosition.z;
	float tmaxz=tminz;
	for (i=0; i<free_node; i++)
	{
		//if (_isnan(nodes[i].Position.length())) LogManager::getSingleton().logMessage("Node is NaN "+StringConverter::toString(i));

		//wetness
		if (nodes[i].wetstate==DRIPPING && !nodes[i].contactless)
		{
			nodes[i].wettime+=dt;
			//if (nodes[i].wettime>5.0) nodes[i].wetstate=DRY; //dry!
			//if (!nodes[i].iswheel && dripp) dripp->alloc(nodes[i].smoothpos, nodes[i].Velocity, ColourValue::ZERO, nodes[i].wettime);
			//also for hot engine
			//if (nodes[i].isHot && dustp) dustp->alloc(nodes[i].smoothpos, nodes[i].Velocity, ColourValue::ZERO, nodes[i].wettime);
		}
		//locked nodes
		if (nodes[i].lockednode)
		{
			nodes[i].AbsPosition=nodes[i].lockedPosition;
			nodes[i].RelPosition=nodes[i].lockedPosition-origin;
			nodes[i].Velocity=nodes[i].lockedVelocity;
			nodes[i].lockedForces=nodes[i].Forces;
			nodes[i].Forces=Vector3(0,0,0);
		}

		//COLLISION
		if (!nodes[i].contactless)
		{
			nodes[i].colltesttimer+=dt;
			if (nodes[i].contacted || nodes[i].colltesttimer>0.005 || (nodes[i].iswheel && nodes[i].colltesttimer>0.0025) || increased_accuracy )
			{
				int contacted=0;
				float ns=0;
				ground_model_t *gm = 0; // this is used as result storage, so we can use it later on
				if ((contacted=collisions->groundCollision(&nodes[i], nodes[i].colltesttimer, &gm, &ns)) |
					collisions->nodeCollision(&nodes[i], i==cinecameranodepos[currentcamera], contacted, nodes[i].colltesttimer, &ns, &gm))
				{
					//FX
					if (gm && doUpdate && !nodes[i].disable_particles)
					{
						DustPool *dp = DustManager::getSingleton().getGroundModelDustPool(gm);
						if(dp)
						{
							dp->alloc(nodes[i].AbsPosition, nodes[i].Velocity, gm->fx_colour);
						}
					}

					// register wheel contact
					if (gm && useSkidmarks && nodes[i].wheelid >= 0)
					{
						if (!(nodes[i].iswheel%2))
							wheels[nodes[i].wheelid].lastContactInner = nodes[i].AbsPosition;
						else
							wheels[nodes[i].wheelid].lastContactOuter = nodes[i].AbsPosition;
						wheels[nodes[i].wheelid].lastContactType = (nodes[i].iswheel%2);
						wheels[nodes[i].wheelid].lastSlip = ns;
						wheels[nodes[i].wheelid].lastGroundModel = gm;
					}

					// note last ground model
					lastFuzzyGroundModel = gm;
				}
				nodes[i].colltesttimer=0.0;
			}
		}


		// record g forces on cameras
		if (i==cameranodepos[0])
		{
			cameranodeacc+=nodes[i].Forces*nodes[i].inverted_mass;
			cameranodecount++;
		}

		//integration
		if (!nodes[i].locked)
		{
			nodes[i].Velocity+=nodes[i].Forces*(dt*nodes[i].inverted_mass);
			nodes[i].RelPosition+=nodes[i].Velocity*dt;
			nodes[i].AbsPosition=origin;
			nodes[i].AbsPosition+=nodes[i].RelPosition;
		}
		if (nodes[i].AbsPosition.x>tmaxx) tmaxx=nodes[i].AbsPosition.x;
		else if (nodes[i].AbsPosition.x<tminx) tminx=nodes[i].AbsPosition.x;
		if (nodes[i].AbsPosition.y>tmaxy) tmaxy=nodes[i].AbsPosition.y;
		else if (nodes[i].AbsPosition.y<tminy) tminy=nodes[i].AbsPosition.y;
		if (nodes[i].AbsPosition.z>tmaxz) tmaxz=nodes[i].AbsPosition.z;
		else if (nodes[i].AbsPosition.z<tminz) tminz=nodes[i].AbsPosition.z;

		//prepare next loop (optimisation)
		//we start forces from zero
		//start with gravity
		nodes[i].Forces=nodes[i].gravimass;

		if (fuseAirfoil)
		{
			//aerodynamics on steroids!
			nodes[i].Forces+=fusedrag;
		}
		else
		{
			if (!disableDrag)
			{
				//add viscous drag (turbulent model)
				if (step&7 && !increased_accuracy)
				{
					//fasttrack drag
					nodes[i].Forces+=nodes[i].lastdrag;

				} else
				{
					Real speed=nodes[i].Velocity.squaredLength();//we will (not) reuse this
					speed=approx_sqrt(speed);
					//plus: turbulences
					Real defdragxspeed= DEFAULT_DRAG*speed;
					//Real maxtur=defdragxspeed*speed*0.01f;
					nodes[i].lastdrag=-defdragxspeed*nodes[i].Velocity;
					Real maxtur=defdragxspeed*speed*0.005f;
					nodes[i].lastdrag+=maxtur*Vector3(frand_11(), frand_11(), frand_11());
					nodes[i].Forces+=nodes[i].lastdrag;
				}
			}
		}
		//if in water
		watercontact=0;
		if (water)
		{
			//basic buoyance
			if (free_buoycab==0)
			{
				if (nodes[i].AbsPosition.y<water->getHeightWaves(nodes[i].AbsPosition))
				{
					watercontact=1;
					//water drag (turbulent)
					float velocityLength=nodes[i].Velocity.length();
					nodes[i].Forces-=(DEFAULT_WATERDRAG*velocityLength)*nodes[i].Velocity;
					nodes[i].Forces+=nodes[i].buoyancy*Vector3::UNIT_Y;
					//basic splashing
					// TODO: FIX SPLASH!
					/*
					if (splashp && water->getHeight()-nodes[i].AbsPosition.y<0.2 && velocityLength>2.0)
					{
						splashp->alloc(nodes[i].AbsPosition, nodes[i].Velocity);
						ripplep->alloc(nodes[i].AbsPosition, nodes[i].Velocity);
					}
					*/
					//engine stall
					if (i==cinecameranodepos[0] && engine) engine->stop();
					//wetness
					nodes[i].wetstate=WET;
				}
				else
				{
					if (nodes[i].wetstate==WET)
					{
						nodes[i].wetstate=DRIPPING;
						nodes[i].wettime=0;
					}
				}
			}
			else
			{
				if (nodes[i].AbsPosition.y<water->getHeightWaves(nodes[i].AbsPosition))
				{
					watercontact=1;
					//engine stall
					if (i==cinecameranodepos[0] && engine) engine->stop();
					//wetness
					nodes[i].wetstate=WET;
				}
				else
				{
					if (nodes[i].wetstate==WET)
					{
						nodes[i].wetstate=DRIPPING;
						nodes[i].wettime=0;
					}
				}
			}
		}
	}

	// anti-explosion guard
	// rationale behind 1e9 number:
	// - while 1e6 is reachable by a fast vehicle, it will be badly deformed and shaking due to loss of precision in calculations
	// - at 1e7 any typical RoR vehicle falls apart and stops functioning
	// - 1e9 may be reachable only by a vehicle that is 1000 times bigger than a typical RoR vehicle, and it will be a loooong trip
	// to be able to travel such long distances will require switching physics calculations to higher precision numbers
	// or taking a different approach to the simulation (truck-local coordinate system?)
	if (!inRange(tminx+tmaxx+tminy+tmaxy+tminz+tmaxz, -1e9, 1e9))
	{
		reset_requested=1; // truck exploded, schedule reset
		return; // return early to avoid propagating invalid values
	}

	minx=tminx-0.3;maxx=tmaxx+0.3;
	miny=tminy-0.3;maxy=tmaxy+0.3;
	minz=tminz-0.3;maxz=tmaxz+0.3;

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Nodes);

	if(statistics)
		statistics->queryStart(BeamThreadStats::Turboprop);
#endif
	//turboprop forces
	for (i=0; i<free_aeroengine; i++)
		if(aeroengines[i]) aeroengines[i]->updateForces(dt, doUpdate);
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Turboprop);

	if(statistics)
		statistics->queryStart(BeamThreadStats::Screwprop);
#endif
	//screwprop forces
	for (i=0; i<free_screwprop; i++)
		if(screwprops[i]) screwprops[i]->updateForces(doUpdate);
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Screwprop);
	if(statistics)
		statistics->queryStart(BeamThreadStats::Wing);
#endif
	//wing forces
	for (i=0; i<free_wing; i++)
		if(wings[i].fa) wings[i].fa->updateForces();
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Wing);

	if(statistics)
		statistics->queryStart(BeamThreadStats::FuseDrag);
#endif
	//compute fuse drag
	if (fuseAirfoil)
	{
		Vector3 wind=-fuseFront->Velocity;
		float wspeed=wind.length();
		Vector3 axis=fuseFront->RelPosition-fuseBack->RelPosition;
		float s=axis.length()*fuseWidth;
		float cz, cx, cm;
		float v=axis.getRotationTo(wind).w;
		float aoa=0;
		if (v<1.0 && v>-1.0) aoa=2.0*acos(v); //quaternion fun
		fuseAirfoil->getparams(aoa, 1.0, 0.0, &cz, &cx, &cm);

		//tropospheric model valid up to 11.000m (33.000ft)
		float altitude=fuseFront->AbsPosition.y;
		float sea_level_temperature=273.15f+15.0f; //in Kelvin
		float sea_level_pressure=101325; //in Pa
		float airtemperature=sea_level_temperature-altitude*0.0065f; //in Kelvin
		float airpressure=sea_level_pressure*approx_pow(1.0-0.0065*altitude/288.1, 5.24947); //in Pa
		float airdensity=airpressure*0.0000120896f;//1.225 at sea level

		//fuselage as an airfoil + parasitic drag (half fuselage front surface almost as a flat plane!)
		fusedrag=((cx*s+fuseWidth*fuseWidth*0.5)*0.5*airdensity*wspeed/free_node)*wind; //free_node is never null
	}
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::FuseDrag);

	if(statistics)
		statistics->queryStart(BeamThreadStats::Airbrakes);
#endif
	//airbrakes
	for (int i=0; i<free_airbrake; i++)
	{
		airbrakes[i]->applyForce();
	}
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Airbrakes);

	if(statistics)
		statistics->queryStart(BeamThreadStats::Buoyance);
#endif
	//water buoyance
	mrtime+=dt;
	if (free_buoycab && water)
	{
		if (!(step%20))
		{
			//clear forces
			for (i=0; i<free_buoycab; i++)
			{
				int tmpv=buoycabs[i]*3;
				nodes[cabs[tmpv]].buoyanceForce=0;
				nodes[cabs[tmpv+1]].buoyanceForce=0;
				nodes[cabs[tmpv+2]].buoyanceForce=0;
			}
			//add forces
			for (i=0; i<free_buoycab; i++)
			{
				int tmpv=buoycabs[i]*3;
				buoyance->computeNodeForce(&nodes[cabs[tmpv]], &nodes[cabs[tmpv+1]], &nodes[cabs[tmpv+2]], doUpdate, buoycabtypes[i]);
			}
		}
		//apply forces
		for (i=0; i<free_node; i++)
		{
			nodes[i].Forces+=nodes[i].buoyanceForce;
		}

	}
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Buoyance);
#endif

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStart(BeamThreadStats::Wheels);
#endif
	//wheel speed
	Real wspeed=0;
	//wheel stuff

	float engine_torque=0.0;

	// calculate torque per wheel
	if (state == ACTIVATED && engine && proped_wheels != 0)
		engine_torque = engine->getTorque()/proped_wheels;

	int propcounter=0;
	float torques[MAX_WHEELS]; // not used
	float newspeeds[MAX_WHEELS];

	float intertorque[MAX_WHEELS] = {0.0f}; //bad initialization
	//old-style viscous code
	if( free_axle == 0)
	{
		//first, evaluate torque from inter-differential locking
		for (i=0; i<proped_wheels/2-1; i++)
		{
			float speed1=(wheels[proppairs[i*2]].speed+wheels[proppairs[i*2+1]].speed)*0.5f;
			float speed2=(wheels[proppairs[i*2+2]].speed+wheels[proppairs[i*2+3]].speed)*0.5f;
			float torque=(speed1-speed2)*10000.0f;
			intertorque[i*2]-=torque*0.5f;
			intertorque[i*2+1]-=torque*0.5f;
			intertorque[i*2+2]+=torque*0.5f;
			intertorque[i*2+3]+=torque*0.5f;
		}
	}
	// new-style Axles
	// loop through all axles for interaxle torque, this is the torsion to keep
	// the axles aligned with each other as if they connected by a shaft
	for (i = 1; i < free_axle; ++i)
	{
		Ogre::Real axle_torques[2] = {0.0f, 0.0f};
		differential_data_t diff_data =
		{
			{
				(wheels[axles[i-1].wheel_1].speed + wheels[axles[i-1].wheel_2].speed) * 0.5f,
				(wheels[axles[i].wheel_1].speed + wheels[axles[i].wheel_2].speed) * 0.5f
			},
			axles[i-1].delta_rotation,
			{ axle_torques[0], axle_torques[1] },
			0, // no input torque, just calculate forces from different axle positions
			dt
		};

#if 0
		// use an open diff just for fun :)
		Axle::calcOpenDiff( diff_data );
#else
		// use the locked diff, most vehicles are setup this way...
		Axle::calcLockedDiff( diff_data );
#endif

		axles[i-1].delta_rotation = diff_data.delta_rotation;
		axles[i].delta_rotation = -diff_data.delta_rotation;

		intertorque[axles[i-1].wheel_1] = diff_data.out_torque[0];
		intertorque[axles[i-1].wheel_2] = diff_data.out_torque[0];
		intertorque[axles[i].wheel_1] = diff_data.out_torque[1];
		intertorque[axles[i].wheel_2] = diff_data.out_torque[1];
	}

	// loop through all the wheels (new style again)
	for (i = 0; i < free_axle; ++i)
	{
		Ogre::Real axle_torques[2] = {0.0f, 0.0f};
		wheel_t *axle_wheels[2] = { &wheels[axles[i].wheel_1], &wheels[axles[i].wheel_2] };

		differential_data_t diff_data =
		{
			{ axle_wheels[0]->speed, axle_wheels[1]->speed },
			axle_wheels[0]->delta_rotation,
			{ axle_torques[0], axle_torques[1] },
			// twice the torque since this is for two wheels, plus extra torque from
			// inter-axle torsion
			2.0f * engine_torque + intertorque[axles[i].wheel_1],
			dt
		};

		axles[i].calcTorque( diff_data );

		axle_wheels[0]->delta_rotation = diff_data.delta_rotation;
		axle_wheels[1]->delta_rotation = -diff_data.delta_rotation;

		intertorque[axles[i].wheel_1] = diff_data.out_torque[0];
		intertorque[axles[i].wheel_2] = diff_data.out_torque[1];
	}

	for (i=0; i<free_wheel; i++)
	{
		Real speedacc=0.0;

		//total torque estimation
		Real total_torque = 0.0;
		if (wheels[i].propulsed > 0)
			total_torque=((free_axle == 0) ? engine_torque : intertorque[i]);

		//braking
		//ignore all braking code if the current wheel is not braked...
		if (wheels[i].braked)
		{
			//handbrake
			float hbrake = 0.0f;
			if (parkingbrake && wheels[i].braked != 4)
				hbrake = hbrakeforce;
			//directional braking
			float dbrake=0.0;
			if (wheels[i].braked==2 && hydrodirstate>0.0 && WheelSpeed<20.0) dbrake=brakeforce*hydrodirstate;
			if (wheels[i].braked==3 && hydrodirstate<0.0 && WheelSpeed<20.0) dbrake=brakeforce*-hydrodirstate;

			if ((brake != 0.0 || dbrake != 0.0 || hbrake != 0.0) && braked_wheels != 0)
			{
				if( fabs(wheels[i].speed) > 0.1f )
					total_torque -= (wheels[i].speed/fabs(wheels[i].speed))*(brake + dbrake + hbrake);
				// wheels are stopped, really this should
				else if( fabs(wheels[i].speed) > 0.0f)
					total_torque -= (wheels[i].speed/fabs(wheels[i].speed))*(brake + dbrake + hbrake)*1.2;
			}
		}
		//friction
		total_torque -= wheels[i].speed*1.0; //it is important to keep some wheel friction to avoid numerical instabilities

		// old-style
		if ( free_axle == 0 && wheels[i].propulsed > 0)
		{
			//differential locking
			if (i%2)
				total_torque-=(wheels[i].speed-wheels[i-1].speed)*10000.0;
			else
				total_torque-=(wheels[i].speed-wheels[i+1].speed)*10000.0;
			//inter differential locking
			total_torque+=intertorque[propcounter];
			propcounter++;
		}

		//application to wheel
		torques[i]=total_torque;
		Vector3 axis=wheels[i].refnode1->RelPosition-wheels[i].refnode0->RelPosition;
		float axis_precalc=total_torque/(Real)(wheels[i].nbnodes);
		axis=fast_normalise(axis);

		for (j=0; j<wheels[i].nbnodes; j++)
		{
			Vector3 radius;
			if (j%2)
				radius=wheels[i].nodes[j]->RelPosition-wheels[i].refnode1->RelPosition;
			else
				radius=wheels[i].nodes[j]->RelPosition-wheels[i].refnode0->RelPosition;
				float inverted_rlen=fast_invSqrt(radius.squaredLength());

			if (wheels[i].propulsed==2)
				radius=-radius;

			Vector3 dir=axis.crossProduct(radius);
			wheels[i].nodes[j]->Forces+=dir*(axis_precalc*inverted_rlen*inverted_rlen);
			//wheel speed
			if (j%2) speedacc+=(wheels[i].nodes[j]->Velocity-wheels[i].refnode1->Velocity).dotProduct(dir)*inverted_rlen;
			else speedacc+=(wheels[i].nodes[j]->Velocity-wheels[i].refnode0->Velocity).dotProduct(dir)*inverted_rlen;
		}
		//wheel speed
		newspeeds[i]=speedacc/wheels[i].nbnodes;
		if (wheels[i].propulsed==1)
			wspeed+=newspeeds[i];
		//for network
		wheels[i].rp+=(newspeeds[i]/wheels[i].radius)*dt;
		//reaction torque
		Vector3 rradius=wheels[i].arm->RelPosition-wheels[i].near_attach->RelPosition;
		Vector3 radius=Plane(axis, wheels[i].near_attach->RelPosition).projectVector(rradius);
		Real rlen=radius.length(); //length of the projected arm
		float offset=(rradius-radius).length(); //length of the error arm
		axis=total_torque*axis;
		if(rlen>0.01)
		{
			radius=radius/(2.0f*rlen*rlen);
			Vector3 cforce=axis.crossProduct(radius);
			//modulate the force according to induced torque error
			if (offset*2.0>rlen) cforce=Vector3::ZERO; // too much error!
			else cforce=(1.0f-((offset*2.0f)/rlen))*cforce; //linear modulation
			wheels[i].arm->Forces-=cforce;
			wheels[i].near_attach->Forces+=cforce;
		}
	}
	//LogManager::getSingleton().logMessage("torque "+StringConverter::toString(torques[0])+" "+StringConverter::toString(torques[1])+" "+StringConverter::toString(torques[2])+" "+StringConverter::toString(torques[3])+" speed "+StringConverter::toString(newspeeds[0])+" "+StringConverter::toString(newspeeds[1])+" "+StringConverter::toString(newspeeds[2])+" "+StringConverter::toString(newspeeds[3]));
	for (i=0; i<free_wheel; i++) wheels[i].speed=newspeeds[i];
	//wheel speed
	if (proped_wheels) wspeed/=(float)proped_wheels;
	lastwspeed=wspeed;
	WheelSpeed=wspeed;

	if(patchEngineTorque)
	{
		if (engine && free_wheel)
		{
			engine->setSpin(wspeed*9.549);
		}
	} else
	{
		if (engine && free_wheel && wheels[0].radius != 0)
		{
			engine->setSpin(wspeed*9.549/wheels[0].radius);
		}
	}


#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Wheels);

	if(statistics)
		statistics->queryStart(BeamThreadStats::Shocks);
#endif
	//update position
//		if(free_node != 0)
//			aposition/=(Real)(free_node);
	//variable shocks for stabilisation
	if (free_active_shock && stabcommand)
	{
		if ((stabcommand==1 && stabratio<0.1) || (stabcommand==-1 && stabratio>-0.1))
			stabratio=stabratio+(float)stabcommand*dt*STAB_RATE;
		for (i=0; i<free_shock; i++)
		{
			// active shocks now
			if (shocks[i].flags & SHOCK_FLAG_RACTIVE)
				beams[shocks[i].beamid].L=beams[shocks[i].beamid].refL*(1.0+stabratio);
			else if (shocks[i].flags & SHOCK_FLAG_LACTIVE)
				beams[shocks[i].beamid].L=beams[shocks[i].beamid].refL*(1.0-stabratio);
		}
	}
	//auto shock adjust
	if (free_active_shock && doUpdate)
	{
		Vector3 dir=nodes[cameranodepos[0]].RelPosition-nodes[cameranoderoll[0]].RelPosition;
		dir.normalise();
		float roll=asin(dir.dotProduct(Vector3::UNIT_Y));
		//			mWindow->setDebugText("Roll:"+ StringConverter::toString(roll));
		if (fabs(roll)>0.2) stabsleep=-1.0; //emergency timeout stop
		if (fabs(roll)>0.03 && stabsleep<0.0)
		{
			if (roll>0.0 && stabcommand!=-1) stabcommand=1;
			else if (roll<0.0 && stabcommand!=1) stabcommand=-1; else {stabcommand=0;stabsleep=3.0;};
		}
		else stabcommand=0;

#ifdef USE_OPENAL
		if (stabcommand && fabs(stabratio)<0.1)
			ssm->trigStart(trucknum, SS_TRIG_AIR);
		else
			ssm->trigStop(trucknum, SS_TRIG_AIR);
#endif //OPENAL

	}
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Shocks);

	if(statistics)
		statistics->queryStart(BeamThreadStats::Hydros);
#endif
	//direction
	if (hydrodirstate!=0 || hydrodircommand!=0)
	{
		float rate=1;
		if(hydroSpeedCoupling)
		{
			rate=40.0/(10.0+fabs(wspeed/2.0));

			// minimum rate: 20% --> enables to steer high velocity trucks
			if(rate<1.2) rate = 1.2;
		}
		if (hydrodircommand!=0)
		{
			if (hydrodirstate > hydrodircommand)
				hydrodirstate -= dt * rate;
			else
				hydrodirstate += dt * rate;
		}
		if(hydroSpeedCoupling)
		{
			float dirdelta=dt;
			if      (hydrodirstate >  dirdelta) hydrodirstate -= dirdelta;
			else if (hydrodirstate < -dirdelta) hydrodirstate += dirdelta;
			else hydrodirstate=0;
		}
	}
	//aileron
	if (hydroaileronstate!=0 || hydroaileroncommand!=0)
	{
		if (hydroaileroncommand!=0)
			if (hydroaileronstate>(hydroaileroncommand))
				hydroaileronstate-=dt*4.0;
			else
				hydroaileronstate+=dt*4.0;
		float delta=dt;
		if (hydroaileronstate>delta) hydroaileronstate-=delta;
		else if (hydroaileronstate<-delta) hydroaileronstate+=delta;
		else hydroaileronstate=0;
	}
	//rudder
	if (hydrorudderstate!=0 || hydroruddercommand!=0)
	{
		if (hydroruddercommand!=0)
			if (hydrorudderstate>(hydroruddercommand))
				hydrorudderstate-=dt*4.0;
			else
				hydrorudderstate+=dt*4.0;
		float delta=dt;
		if (hydrorudderstate>delta) hydrorudderstate-=delta;
		else if (hydrorudderstate<-delta) hydrorudderstate+=delta;
		else hydrorudderstate=0;
	}
	//elevator
	if (hydroelevatorstate!=0 || hydroelevatorcommand!=0)
	{
		if (hydroelevatorcommand!=0)
			if (hydroelevatorstate>(hydroelevatorcommand))
				hydroelevatorstate-=dt*4.0;
			else
				hydroelevatorstate+=dt*4.0;
		float delta=dt;
		if (hydroelevatorstate>delta) hydroelevatorstate-=delta;
		else if (hydroelevatorstate<-delta) hydroelevatorstate+=delta;
		else hydroelevatorstate=0;
	}
	//update length, dirstate between -1.0 and 1.0
	for (int i=0; i<free_hydro; i++)
	{
		//compound hydro
		float cstate=0.0f;
		int div=0;
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_SPEED)
		{
			//special treatment for SPEED
			if (WheelSpeed<12.0)
				cstate += hydrodirstate*(12.0-WheelSpeed)/12.0;
			div++;
		}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_DIR) {cstate+=hydrodirstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_AILERON) {cstate+=hydroaileronstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_RUDDER) {cstate+=hydrorudderstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_ELEVATOR) {cstate+=hydroelevatorstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_AILERON) {cstate-=hydroaileronstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_RUDDER) {cstate-=hydrorudderstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_ELEVATOR) {cstate-=hydroelevatorstate;div++;}

		// Animators following, if no animator, skip all the tests...
		int flagstate = beams[hydro[i]].animFlags;
		if(flagstate)
		{
			float animoption = beams[hydro[i]].animOption;
			calcAnimators(flagstate, cstate, div, dt, 0.0f, 0.0f, animoption);
			}

		if (div)
		{
			cstate=cstate/(float)div;
			if(hydroInertia)
				cstate=hydroInertia->calcCmdKeyDelay(cstate,i,dt);

			if (!(beams[hydro[i]].hydroFlags & HYDRO_FLAG_SPEED))
				hydrodirwheeldisplay=cstate;
			beams[hydro[i]].L=beams[hydro[i]].Lhydro*(1.0-cstate*beams[hydro[i]].hydroRatio);
		}
	}

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Hydros);
#endif

	// forward things to trailers
	if (state==ACTIVATED && forwardcommands)
	{
		int i,j;
		for (i=0; i<numtrucks; i++)
		{
			if(!trucks[i]) continue;
			if (trucks[i]->state==DESACTIVATED && trucks[i]->importcommands)
			{
				// forward commands
				for (j=1; j<MAX_COMMANDS; j++)
					trucks[i]->commandkey[j].commandValue = commandkey[j].commandValue;

				// just send brake and lights to the connected truck, and no one else :)
				for(std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
				{
					if(!it->lockTruck) continue;
					// forward brake
					it->lockTruck->brake = brake;
					it->lockTruck->parkingbrake = parkingbrake;

					// forward lights
					it->lockTruck->lights = lights;
					it->lockTruck->blinkingtype = blinkingtype;
					//for(int k=0;k<4;k++)
					//	lockTruck->setCustomLight(k, getCustomLight(k));

				}
			}
		}
	}

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStart(BeamThreadStats::Commands);
#endif
	// commands
	if (hascommands)
	{
		int active=0;
		int requested=0;
		float work=0.0;
		if (engine)
			canwork=(engine->getRPM()>800.0);
		else
			canwork=1;
		float crankfactor=1;
		if (engine) crankfactor=engine->getCrankFactor();

		// speed up machines
		if(driveable==MACHINE)
			crankfactor = 2;

		for (i=0; i<=MAX_COMMANDS; i++)
			for (int j=0; j < (int)commandkey[i].beams.size(); j++)
				beams[abs(commandkey[i].beams[j])].autoMoveLock=false;

		for (i=0; i<=MAX_COMMANDS; i++)
			for (int j=0; j < (int)commandkey[i].beams.size(); j++)
				if(commandkey[i].commandValue >= 0.5)
					beams[abs(commandkey[i].beams[j])].autoMoveLock=true;


		// only process ties if there is enough force available
		if(canwork)
		{
			bool requestpower = false;
			// go through all ties and process them
			for(std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
			{
				// only process tying ties
				if(!it->tying) continue;

				// division through zero guard
				if(it->beam->refL == 0 || it->beam->L == 0) continue;

				float clen = it->beam->L / it->beam->refL;
				if (clen > it->beam->commandShort)
				{
					float dl = it->beam->L;
					it->beam->L *= (1.0 - it->beam->commandRatioShort * dt / it->beam->L);
					dl = fabs(dl - it->beam->L);
					requestpower = true;
					active++;
					work += fabs(it->beam->stress) * dl;
				} else
				{
					// tying finished, end reached
					it->tying = false;
				}

				// check if we hit a certain force limit, then abort the tying process
				if (fabs(it->beam->stress) > it->beam->maxtiestress)
					it->tying = false;
			}
			if(requestpower)
				requested++;
		}

		// now process normal commands
		for (i=0; i<=MAX_COMMANDS; i++)
		{
			int j;

			bool requestpower = false;
			for (j=0; j < (int)commandkey[i].beams.size(); j++)
			{
				int bbeam=commandkey[i].beams[j];
				int bbeam_abs=abs(bbeam);

				// restrict forces
				if(beams[bbeam_abs].isforcerestricted && crankfactor > 1)
					crankfactor=1;

				float v = commandkey[i].commandValue;
				/*
				if(i==1)
				LogManager::getSingleton().logMessage(StringConverter::toString(v) + "/" + StringConverter::toString(beams[bbeam].autoMovingMode));
				*/

				// self centering
				if(beams[bbeam_abs].iscentering && !beams[bbeam_abs].autoMoveLock)
				{
					// check for some error
					if(beams[bbeam_abs].refL == 0 || beams[bbeam_abs].L == 0)
						continue;

					float current = (beams[bbeam_abs].L/beams[bbeam_abs].refL);
					/*
					if(i==1)
					LogManager::getSingleton().logMessage("centering: "+ \
					StringConverter::toString(current)+" / "+ \
					StringConverter::toString(beams[bbeam_abs].centerLength)+ " / " + \
					StringConverter::toString(v)+" / ");
					*/

					// hold condition
					if(fabs(current-beams[bbeam_abs].centerLength) < 0.0001)
					{
						beams[bbeam_abs].autoMovingMode = 0;
						/*
						if(i==1)
						LogManager::getSingleton().logMessage("centering complete");
						*/
					}
					else
					{
						// determine direction
						if(current > beams[bbeam_abs].centerLength)
							beams[bbeam_abs].autoMovingMode = -1;
						else
							beams[bbeam_abs].autoMovingMode = 1;
					}
				}

				if(beams[bbeam_abs].refL != 0 && beams[bbeam_abs].L != 0)
				{
					if (bbeam>0)
					{
						float clen = beams[bbeam].L/beams[bbeam].refL;
						if (clen<beams[bbeam].commandLong)
						{
							float dl=beams[bbeam].L;

							if(beams[bbeam].isOnePressMode==2)
							{
								// one press + centering
								//String sMode = (beams[bbeam].pressedCenterMode?"YES":"NO");
								//LogManager::getSingleton().logMessage(sMode+"|"+StringConverter::toString(clen)+" / "+StringConverter::toString(beams[bbeam].centerLength));
								if(beams[bbeam].autoMovingMode > 0 && clen > beams[bbeam].centerLength && !beams[bbeam].pressedCenterMode)
								{
									beams[bbeam].pressedCenterMode = true;
									beams[bbeam].autoMovingMode=0;
								}
								else if(beams[bbeam].autoMovingMode < 0 && clen > beams[bbeam].centerLength && beams[bbeam].pressedCenterMode)
									beams[bbeam].pressedCenterMode = false;
							}
							if(beams[bbeam].isOnePressMode>0)
							{
								bool key = (v > 0.5);
								if(beams[bbeam].autoMovingMode <= 0 && key)
								{
									//LogManager::getSingleton().logMessage("LONG auto-moving-start!");
									beams[bbeam].autoMovingMode=1;
								}
								else if(beams[bbeam].autoMovingMode==1 && !key)
								{
									//LogManager::getSingleton().logMessage("LONG auto-moving step2!");
									beams[bbeam].autoMovingMode=2;
								}
								else if(beams[bbeam].autoMovingMode==2 && key)
								{
									//LogManager::getSingleton().logMessage("LONG auto-moving-end step1!");
									beams[bbeam].autoMovingMode=3;
								}
								else if(beams[bbeam].autoMovingMode==3 && !key)
								{
									//LogManager::getSingleton().logMessage("LONG auto-moving-end step2!");
									beams[bbeam].autoMovingMode=0;
								}
							}

							if(cmdInertia)
								v=cmdInertia->calcCmdKeyDelay(v,i,dt);

							if(beams[bbeam].autoMovingMode > 0)
								v = 1;

							if(v>0.5)
								requestpower=true;

							if(!canwork)
								continue;

							beams[bbeam].L *= (1.0 + beams[bbeam].commandRatioLong * v * crankfactor * dt / beams[bbeam].L);;
							dl=fabs(dl-beams[bbeam].L);
							if(v>0.5)
							{
								active++;
								work+=fabs(beams[bbeam].stress)*dl;
							}
						} else
						{
							// beyond lenght
							if(beams[bbeam].isOnePressMode>0 && beams[bbeam].autoMovingMode > 0)
							{
								//LogManager::getSingleton().logMessage("LONG auto-moving-end!");
								beams[bbeam].autoMovingMode=0;
							}
						}
					} else
					{
						bbeam=-bbeam;
						float clen = beams[bbeam].L/beams[bbeam].refL;
						if (clen>beams[bbeam].commandShort)
						{
							float dl=beams[bbeam].L;

							if(beams[bbeam].isOnePressMode==2)
							{
								// one press + centering
								//String sMode = (beams[bbeam].pressedCenterMode?"YES":"NO");
								//LogManager::getSingleton().logMessage(sMode+"|"+StringConverter::toString(clen)+" / "+StringConverter::toString(beams[bbeam].centerLength));
								if(beams[bbeam].autoMovingMode < 0 && clen < beams[bbeam].centerLength && !beams[bbeam].pressedCenterMode)
								{
									beams[bbeam].pressedCenterMode = true;
									beams[bbeam].autoMovingMode=0;
								}
								else if(beams[bbeam].autoMovingMode > 0 && clen < beams[bbeam].centerLength && beams[bbeam].pressedCenterMode)
									beams[bbeam].pressedCenterMode = false;
							}
							if(beams[bbeam].isOnePressMode>0)
							{
								bool key = (v > 0.5);
								if(beams[bbeam].autoMovingMode >=0 && key)
								{
									//LogManager::getSingleton().logMessage("SHORT auto-moving-start!");
									beams[bbeam].autoMovingMode=-1;
								}
								else if(beams[bbeam].autoMovingMode==-1 && !key)
								{
									//LogManager::getSingleton().logMessage("SHORT auto-moving step2!");
									beams[bbeam].autoMovingMode=-2;
								}
								else if(beams[bbeam].autoMovingMode==-2 && key)
								{
									//LogManager::getSingleton().logMessage("SHORT auto-moving-end step1!");
									beams[bbeam].autoMovingMode=-3;
								}
								else if(beams[bbeam].autoMovingMode==-3 && !key)
								{
									//LogManager::getSingleton().logMessage("SHORT auto-moving-end step2!");
									beams[bbeam].autoMovingMode=0;
								}
							}

							if(cmdInertia)
								v=cmdInertia->calcCmdKeyDelay(v,i,dt);

							if(beams[bbeam].autoMovingMode < 0)
								v = 1;

							if(v>0.5)
								requestpower=true;

							if(!canwork)
								continue;

							beams[bbeam].L *= (1.0 - beams[bbeam].commandRatioShort * v * crankfactor * dt / beams[bbeam].L);
							dl=fabs(dl-beams[bbeam].L);
							if(v>0.5)
							{
								requestpower=true;
								active++;
								work+=fabs(beams[bbeam].stress)*dl;
							}
						} else
						{
							// beyond lenght
							if(beams[bbeam].isOnePressMode>0 && beams[bbeam].autoMovingMode < 0)
							{
								//LogManager::getSingleton().logMessage("SHORT auto-moving-end!");
								beams[bbeam].autoMovingMode=0;
							}
						}
					};
				}
			}
			//also for rotators
			for (j=0; j < (int)commandkey[i].rotators.size(); j++)
			{
				if ((commandkey[i].rotators[j])>0)
				{
					int rota = commandkey[i].rotators[j] - 1;
					float value=0;
					if(rotaInertia)
					{
						value=rotaInertia->calcCmdKeyDelay(commandkey[i].commandValue,i,dt);
					}
					if(value>0.5f)
						requestpower=true;
					rotators[rota].angle += rotators[rota].rate * value * crankfactor * dt;
				}
				else
				{
					int rota =- (commandkey[i].rotators[j]) - 1;
					float value=0;
					if(rotaInertia)
					{
						value=rotaInertia->calcCmdKeyDelay(commandkey[i].commandValue,i,dt);
					}
					if(value>0.5f)
						requestpower=true;
					rotators[rota].angle -= rotators[rota].rate * value * crankfactor * dt;
				}
			}
			if(requestpower)
				requested++;

		}

		if (engine)
		{
			engine->hydropump=work;
			engine->prime=requested;
		}
		if (doUpdate && state==ACTIVATED)
		{
#ifdef USE_OPENAL
			if (active)
			{
				ssm->trigStart(trucknum, SS_TRIG_PUMP);
				float pump_rpm=660.0*(1.0-(work/(float)active)/100.0);
				ssm->modulate(trucknum, SS_MOD_PUMP, pump_rpm);
			}
			else
				ssm->trigStop(trucknum, SS_TRIG_PUMP);
#endif //OPENAL
		}
		//rotators
		for (i=0; i<free_rotator; i++)
		{
			//compute rotation axis
			Vector3 axis=nodes[rotators[i].axis1].RelPosition-nodes[rotators[i].axis2].RelPosition;
			//axis.normalise();
			axis=fast_normalise(axis);
			//find the reference plane
			Plane pl=Plane(axis, 0);
			//for each pair
			int k;
			for (k=0; k<2; k++)
			{
				//find the reference vectors
				Vector3 ref1=pl.projectVector(nodes[rotators[i].axis2].RelPosition-nodes[rotators[i].nodes1[k]].RelPosition);
				Vector3 ref2=pl.projectVector(nodes[rotators[i].axis2].RelPosition-nodes[rotators[i].nodes2[k]].RelPosition);
				//theory vector
				Vector3 th1=Quaternion(Radian(rotators[i].angle+3.14159/2.0), axis)*ref1;
				//find the angle error
				float aerror=asin((th1.normalisedCopy()).dotProduct(ref2.normalisedCopy()));
				//			mWindow->setDebugText("Error:"+ StringConverter::toString(aerror));
				//exert forces
				float rigidity=10000000.0;
				Vector3 dir1=ref1.crossProduct(axis);
				//dir1.normalise();
				dir1=fast_normalise(dir1);
				Vector3 dir2=ref2.crossProduct(axis);
				//dir2.normalise();
				dir2=fast_normalise(dir2);
				float ref1len=ref1.length();
				float ref2len=ref2.length();
				nodes[rotators[i].nodes1[k]].Forces+=(aerror*ref1len*rigidity)*dir1;
				nodes[rotators[i].nodes2[k]].Forces-=(aerror*ref2len*rigidity)*dir2;
				//symmetric
				nodes[rotators[i].nodes1[k+2]].Forces-=(aerror*ref1len*rigidity)*dir1;
				nodes[rotators[i].nodes2[k+2]].Forces+=(aerror*ref2len*rigidity)*dir2;
			}
		}

	}
#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Commands);

	if(statistics)
		statistics->queryStop(BeamThreadStats::WholeTruckCalc);
#endif

	//we also store a new replay frame
	if(replay)
	{
		replayTimer += dt;
		if(replayTimer > replayPrecision)
		{
			// store nodes
			node_simple_t *nbuff = (node_simple_t *)replay->getWriteBuffer(0);
			for (i=0; i<free_node; i++)
				nbuff[i].pos = nodes[i].AbsPosition;

			// store beams
			beam_simple_t *bbuff = (beam_simple_t *)replay->getWriteBuffer(1);
			for (i=0; i<free_beam; i++)
			{
				bbuff[i].scale = beams[i].scale;
				bbuff[i].broken = beams[i].broken;
				bbuff[i].disabled = beams[i].disabled;
			}

			replay->writeDone();
			replayTimer = 0.0f;
		}
	}

}

//truck-truck collisions
void Beam::truckTruckCollisions(Real dt, Beam** trucks, int numtrucks)
{

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStart(BeamThreadStats::Contacters);
#endif

	float trwidth;

	int t;
	int hitnodeid;
	int hittruckid;
	int i;

	float inverted_dt=1.0f/dt;

	pointCD->update(trucks, numtrucks);

	int colltype=0;
	int tmpv;
	Matrix3 forward;
	bool calcforward;
	Vector3 bx;
	Vector3 by;
	Vector3 bz;
	Vector3 point;
	Vector3 forcevec;
	Vector3 vecrelVel;
	Vector3 plnormal;
	node_t* no;
	node_t* na;
	node_t* nb;
	node_t* hitnode;
	Beam* hittruck;

	for (t=0; t<numtrucks; t++)
	{
		//If you change any of the below "ifs" concerning trucks then you should
		//also consider changing the parallel "ifs" inside PointColDetector
		//see "pointCD" above.
		//Performance some times forces ugly architectural designs....
		if (!trucks[t] || trucks[t]->state==SLEEPING || trucks[t]->state==RECYCLE || trucks[t]->state==NETWORKED) continue;

		trwidth=trucks[t]->collrange;

		for (i=0; i<trucks[t]->free_collcab; i++)
		{
			if (trucks[t]->collcabrate[i].rate>0)
			{
				trucks[t]->collcabrate[i].rate--;
				continue;
			}

			if (trucks[t]->collcabrate[i].distance<1) trucks[t]->collcabrate[i].distance=1;

			tmpv=trucks[t]->collcabs[i]*3;
			no=&trucks[t]->nodes[trucks[t]->cabs[tmpv]];
			na=&trucks[t]->nodes[trucks[t]->cabs[tmpv+1]];
			nb=&trucks[t]->nodes[trucks[t]->cabs[tmpv+2]];

			pointCD->query(no->AbsPosition
				, na->AbsPosition
				, nb->AbsPosition, trwidth*trucks[t]->collcabrate[i].distance);

			calcforward=true;
			for (int h=0; h<pointCD->hit_count;h++)
			{
				hitnodeid=pointCD->hit_list[h]->nodeid;
				hittruckid=pointCD->hit_list[h]->truckid;
				hitnode=&trucks[hittruckid]->nodes[hitnodeid];

				//ignore self-contact
				if (hittruckid==t)
				{
					//ignore wheel/chassis self contact
					//if (hitnode->iswheel && !(trucks[t]->requires_wheel_contact)) continue;
					if (hitnode->iswheel) continue;
					if (no==hitnode || na==hitnode || nb==hitnode) continue;
				}

				hittruck=trucks[hittruckid];

				//calculate transform matrices
				if (calcforward)
				{
					calcforward=false;
					bx=na->RelPosition;
					by=nb->RelPosition;
					bx-=no->RelPosition;
					by-=no->RelPosition;
					bz=bx.crossProduct(by);
					bz=fast_normalise(bz);
					//coordinates change matrix
					forward.SetColumn(0, bx);
					forward.SetColumn(1, by);
					forward.SetColumn(2, bz);
					forward=forward.Inverse();
				}

				//change coordinates
				point=forward * (hitnode->AbsPosition - no->AbsPosition);

				//test
				if (point.x>=0 && point.y>=0 && (point.x+point.y)<=1.0 && point.z<=trwidth && point.z>=-trwidth)
				{
					//collision
					plnormal=bz;

					//some more accuracy for the normal
					plnormal.normalise();

					float penetration=0.0f;

					//Find which side most of the connected nodes (through beams) are
					if (hittruckid!=t && hittruck->nodetonodeconnections[hitnodeid].size()>3)
					{
						//float sumofdistances=0.0f;
						int posside=0;
						int negside=0;
						float tmppz=point.z;
						float distance;

						for (unsigned int ni=0;ni<hittruck->nodetonodeconnections[hitnodeid].size();ni++)
						{
							distance=plnormal.dotProduct(hittruck->nodes[hittruck->nodetonodeconnections[hitnodeid][ni]].AbsPosition-no->AbsPosition);
							if (distance>=0) posside++; else negside++;
						}

						//Current hitpoint's position has triple the weight
						if (point.z>=0) posside+=3;
						else negside+=3;

						if (negside>posside)
						{
							plnormal=-plnormal;
							tmppz=-tmppz;
						}

						penetration=(trwidth-tmppz);
					}
					else
					{
						//If we are on the other side of the triangle invert the triangle's normal
						if (point.z<0) plnormal=-plnormal;
						penetration=(trwidth-fabs(point.z));
					}

					//Find the point's velocity relative to the triangle
					vecrelVel=(hitnode->Velocity-
					(no->Velocity*(-point.x-point.y+1.0f)+na->Velocity*point.x
						+nb->Velocity*point.y));

					//Find the velocity perpendicular to the triangle
					float velForce=vecrelVel.dotProduct(plnormal);
					//if it points away from the triangle the ignore it (set it to 0)
					if (velForce<0.0f) velForce=-velForce;
					else velForce=0.0f;

					//Velocity impulse
					float vi=hitnode->mass*inverted_dt*(velForce+inverted_dt*penetration)*0.5f;

					//The force that the triangle puts on the point
					float trfnormal=(no->Forces*(-point.x-point.y+1.0f)+na->Forces*point.x
						+nb->Forces*point.y).dotProduct(plnormal);
					//(applied only when it is towards the point)
					if (trfnormal<0.0f) trfnormal=0.0f;

					//The force that the point puts on the triangle
					float pfnormal=hitnode->Forces.dotProduct(plnormal);
					//(applied only when it is towards the triangle)
					if (pfnormal>0.0f) pfnormal=0.0f;

					float fl=(vi+trfnormal-pfnormal)*0.5f;

					forcevec=Vector3::ZERO;
					float nso;

					//Calculate the collision forces
					collisions->primitiveCollision(hitnode, forcevec, vecrelVel, plnormal, ((float) dt), collisions->defaultgm, &nso, penetration, fl);

					hitnode->Forces+=forcevec;

					// no network special case for now
					//if(trucks[t]->state==NETWORKED)

					no->Forces-=(-point.x-point.y+1.0f)*forcevec;
					na->Forces-=(point.x)*forcevec;
					nb->Forces-=(point.y)*forcevec;
				}
			}

			if (calcforward)
			{
				trucks[t]->collcabrate[i].rate=((trucks[t]->collcabrate[i].distance)-1);
				if (trucks[t]->collcabrate[i].distance<13) trucks[t]->collcabrate[i].distance++;
			} else
			{
				trucks[t]->collcabrate[i].distance/=2;
				trucks[t]->collcabrate[i].rate=0;
			}
		}
	}

#ifdef FEAT_TIMING
	if(statistics)
		statistics->queryStop(BeamThreadStats::Contacters);
#endif
}

// call this once per frame in order to update the skidmarks
void Beam::updateSkidmarks()
{
	for(int i=0;i<free_wheel;i++)
	{
		// ignore wheels without data
		if(wheels[i].lastContactInner == Vector3::ZERO && wheels[i].lastContactOuter == Vector3::ZERO) continue;
		// create skidmark object for wheels with data if not existing
		if(!skidtrails[i])
			skidtrails[i] = new Skidmark(tsm, &wheels[i], hfinder, beamsRoot, 300, 200);

		skidtrails[i]->updatePoint();
	}

	//LogManager::getSingleton().logMessage("updating skidmark visuals");
	for(int i=0;i<free_wheel;i++)
		if(skidtrails[i]) skidtrails[i]->update();
}


Quaternion Beam::specialGetRotationTo(const Vector3& src, const Vector3& dest) const
{
	// Based on Stan Melax's article in Game Programming Gems
	Quaternion q;
	// Copy, since cannot modify local
	Vector3 v0 = src;
	Vector3 v1 = dest;
	v0.normalise();
	v1.normalise();


	// NB if the crossProduct approaches zero, we get unstable because ANY axis will do
	// when v0 == -v1
	Real d = v0.dotProduct(v1);
	// If dot == 1, vectors are the same
	if (d >= 1.0f)
	{
		return Quaternion::IDENTITY;
	}
	if (d < (1e-6f - 1.0f))
	{
		// Generate an axis
		Vector3 axis = Vector3::UNIT_X.crossProduct(src);
		if (axis.isZeroLength()) // pick another if colinear
			axis = Vector3::UNIT_Y.crossProduct(src);
		axis.normalise();
		q.FromAngleAxis(Radian(PI), axis);
	}
	else
	{
		Real s = fast_sqrt( (1+d)*2 );
		if (s==0) return Quaternion::IDENTITY;

		Vector3 c = v0.crossProduct(v1);
		Real invs = 1 / s;


		q.x = c.x * invs;
		q.y = c.y * invs;
		q.z = c.z * invs;
		q.w = s * 0.5;
	}
	return q;
}



void Beam::prepareInside(bool inside)
{
	isInside=inside;
	if(inside)
	{
		//going inside

		// activate cabin lights if lights are turned on
		if(lights && cablightNode && cablight)
		{
			cablightNode->setVisible(true);
			cablight->setVisible(true);
		}

		//disabling shadow
		if (cabNode && cabNode->numAttachedObjects() && cabNode->getAttachedObject(0)) ((Entity*)(cabNode->getAttachedObject(0)))->setCastShadows(false);
		int i;
		for (i=0; i<free_prop; i++)
		{
			if (props[i].snode && props[i].snode->numAttachedObjects()) props[i].snode->getAttachedObject(0)->setCastShadows(false);
			if (props[i].wheel && props[i].wheel->numAttachedObjects()) props[i].wheel->getAttachedObject(0)->setCastShadows(false);
		}
		for (i=0; i<free_wheel; i++) if(vwheels[i].cnode->numAttachedObjects()) vwheels[i].cnode->getAttachedObject(0)->setCastShadows(false);
		for (i=0; i<free_beam; i++) if (beams[i].mEntity) beams[i].mEntity->setCastShadows(false);

		if (cabNode)
		{
			char transmatname[256];
			sprintf(transmatname, "%s-trans", texname);
			MaterialPtr transmat=(MaterialPtr)(MaterialManager::getSingleton().getByName(transmatname));
			transmat->setReceiveShadows(false);
		}
		//setting camera
		mCamera->setNearClipDistance( 0.1 );
		//activate mirror
		if (mirror) mirror->setActive(true);
		//enable transparent seat
		MaterialPtr seatmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("driversseat"));
		seatmat->setDepthWriteEnabled(false);
		seatmat->setSceneBlending(SBT_TRANSPARENT_ALPHA);
	}
	else
	{
		//going outside

		// disable cabin light before going out
		if(cablightNode && cablight)
		{
			cablightNode->setVisible(false);
			cablight->setVisible(false);
		}

		//enabling shadow
		if (cabNode && cabNode->numAttachedObjects() && cabNode->getAttachedObject(0)) ((Entity*)(cabNode->getAttachedObject(0)))->setCastShadows(true);
		int i;
		for (i=0; i<free_prop; i++)
		{
			if (props[i].snode && props[i].snode->numAttachedObjects()) props[i].snode->getAttachedObject(0)->setCastShadows(true);
			if (props[i].wheel && props[i].wheel->numAttachedObjects()) props[i].wheel->getAttachedObject(0)->setCastShadows(true);
		}
		for (i=0; i<free_wheel; i++) if(vwheels[i].cnode->numAttachedObjects()) vwheels[i].cnode->getAttachedObject(0)->setCastShadows(true);
		for (i=0; i<free_beam; i++) if (beams[i].mEntity) beams[i].mEntity->setCastShadows(true);

		if (cabNode)
		{
			char transmatname[256];
			sprintf(transmatname, "%s-trans", texname);
			MaterialPtr transmat=(MaterialPtr)(MaterialManager::getSingleton().getByName(transmatname));
			transmat->setReceiveShadows(true);
		}
		//setting camera
		mCamera->setNearClipDistance( 0.5 );
		//desactivate mirror
		if (mirror) mirror->setActive(false);
		//disable transparent seat
		MaterialPtr seatmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("driversseat"));
		seatmat->setDepthWriteEnabled(true);
		seatmat->setSceneBlending(SBT_REPLACE);
	}
}


void Beam::lightsToggle(Beam** trucks, int trucksnum)
{
	// no lights toggling in skeleton mode because of possible bug with emissive texture
	if(skeleton)
		return;
	int i;
	//export light command
	if (trucks!=0 && state==ACTIVATED && forwardcommands)
	{
		int i;
		for (i=0; i<trucksnum; i++)
		{
			if(!trucks[i]) continue;
			if (trucks[i]->state==DESACTIVATED && trucks[i]->importcommands) trucks[i]->lightsToggle(trucks, trucksnum);
		}
	}
	lights=!lights;
	if(cablight && cablightNode && isInside)
		cablightNode->setVisible((lights!=0));
	if (!lights)
	{
		for (i=0; i<free_flare; i++)
		{
			if(flares[i].type == 'f')
			{
				flares[i].snode->setVisible(false);
				if(flares[i].bbs) flares[i].snode->detachAllObjects();
				if (flares[i].light) flares[i].light->setVisible(false);
				flares[i].isVisible=false;
			}
		}
		if (hasEmissivePass)
		{
			char clomatname[256];
			sprintf(clomatname, "%s-noem", texname);
			if(cabNode->numAttachedObjects())
			{
				Entity* ent=((Entity*)(cabNode->getAttachedObject(0)));
				int numsubent=ent->getNumSubEntities();
				for (i=0; i<numsubent; i++)
				{
					SubEntity *subent=ent->getSubEntity(i);
					if (!strcmp((subent->getMaterialName()).c_str(), texname)) subent->setMaterialName(clomatname);
				}
				//			((Entity*)(cabNode->getAttachedObject(0)))->setMaterialName(clomatname);
			}
		}
	}
	else
	{
		for (i=0; i<free_flare; i++)
		{
			if(flares[i].type == 'f')
			{
				if (flares[i].light) flares[i].light->setVisible(true);
				flares[i].isVisible=true;
				if(flares[i].bbs) flares[i].snode->attachObject(flares[i].bbs);
			}
		}
		if (hasEmissivePass)
		{
			char clomatname[256];
			sprintf(clomatname, "%s-noem", texname);
			if(cabNode->numAttachedObjects())
			{
				Entity* ent=((Entity*)(cabNode->getAttachedObject(0)));
				int numsubent=ent->getNumSubEntities();
				for (i=0; i<numsubent; i++)
				{
					SubEntity *subent=ent->getSubEntity(i);
					if (!strcmp((subent->getMaterialName()).c_str(), clomatname)) subent->setMaterialName(texname);
				}
				//			((Entity*)(cabNode->getAttachedObject(0)))->setMaterialName(texname);
			}
		}
	};
#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_LIGHT_TOGGLE, trucknum);
#endif

}

void Beam::updateFlares(float dt, bool isCurrent)
{
	bool enableAll = true;
	if(flaresMode==0)
		return;
	if(flaresMode==2 && !isCurrent)
		enableAll=false;
	int i;
	//okay, this is just ugly, we have flares in props!
	//we have to update them here because they run
	if (beacon)
	{
		for (i=0; i<free_prop; i++)
		{
			if (props[i].beacontype=='b')
			{
				//update light
				Quaternion orientation=props[i].snode->getOrientation();
				props[i].light[0]->setPosition(props[i].snode->getPosition()+orientation*Vector3(0,0,0.12));
				props[i].bpos[0]+=dt*props[i].brate[0];//rotate baby!
				props[i].light[0]->setDirection(orientation*Vector3(cos(props[i].bpos[0]),sin(props[i].bpos[0]),0));
				//billboard
				Vector3 vdir=props[i].light[0]->getPosition()-mCamera->getPosition();
				float vlen=vdir.length();
				if (vlen>100.0) {props[i].bbsnode[0]->setVisible(false);continue;};
				//normalize
				vdir=vdir/vlen;
				props[i].bbsnode[0]->setPosition(props[i].light[0]->getPosition()-vdir*0.1);
				float amplitude=props[i].light[0]->getDirection().dotProduct(vdir);
				if (amplitude>0)
				{
					props[i].bbsnode[0]->setVisible(true);
					props[i].bbs[0]->setDefaultDimensions(amplitude*amplitude*amplitude, amplitude*amplitude*amplitude);
				}
				else
				{
					props[i].bbsnode[0]->setVisible(false);
				}
				props[i].light[0]->setVisible(enableAll);
			}
			if (props[i].beacontype=='p')
			{
				int k;
				for (k=0; k<4; k++)
				{
					//update light
					Quaternion orientation=props[i].snode->getOrientation();
					switch (k)
					{
					case 0: props[i].light[k]->setPosition(props[i].snode->getPosition()+orientation*Vector3(-0.64,0,0.14));break;
					case 1: props[i].light[k]->setPosition(props[i].snode->getPosition()+orientation*Vector3(-0.32,0,0.14));break;
					case 2: props[i].light[k]->setPosition(props[i].snode->getPosition()+orientation*Vector3(+0.32,0,0.14));break;
					case 3: props[i].light[k]->setPosition(props[i].snode->getPosition()+orientation*Vector3(+0.64,0,0.14));break;
					}
					props[i].bpos[k]+=dt*props[i].brate[k];//rotate baby!
					props[i].light[k]->setDirection(orientation*Vector3(cos(props[i].bpos[k]),sin(props[i].bpos[k]),0));
					//billboard
					Vector3 vdir=props[i].light[k]->getPosition()-mCamera->getPosition();
					float vlen=vdir.length();
					if (vlen>100.0) {props[i].bbsnode[k]->setVisible(false);continue;};
					//normalize
					vdir=vdir/vlen;
					props[i].bbsnode[k]->setPosition(props[i].light[k]->getPosition()-vdir*0.2);
					float amplitude=props[i].light[k]->getDirection().dotProduct(vdir);
					if (amplitude>0)
					{
						props[i].bbsnode[k]->setVisible(true);
						props[i].bbs[k]->setDefaultDimensions(amplitude*amplitude*amplitude, amplitude*amplitude*amplitude);
					}
					else
					{
						props[i].bbsnode[k]->setVisible(false);
					}
					props[i].light[k]->setVisible(enableAll);
				}
			}
			if (props[i].beacontype=='r')
			{
				//update light
				Quaternion orientation=props[i].snode->getOrientation();
				props[i].light[0]->setPosition(props[i].snode->getPosition()+orientation*Vector3(0,0,0.06));
				props[i].bpos[0]+=dt*props[i].brate[0];//rotate baby!
				//billboard
				Vector3 vdir=props[i].light[0]->getPosition()-mCamera->getPosition();
				float vlen=vdir.length();
				if (vlen>100.0) {props[i].bbsnode[0]->setVisible(false);continue;};
				//normalize
				vdir=vdir/vlen;
				props[i].bbsnode[0]->setPosition(props[i].light[0]->getPosition()-vdir*0.1);
				bool visible=false;
				if (props[i].bpos[0]>1.0)
				{
					props[i].bpos[0]=0.0;
					visible=true;
				}
				visible = visible && enableAll;
				props[i].light[0]->setVisible(visible);
				props[i].bbsnode[0]->setVisible(visible);

			}
			if (props[i].beacontype=='R' || props[i].beacontype=='L')
			{
				Vector3 mposition=nodes[props[i].noderef].smoothpos+props[i].offsetx*(nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos)+props[i].offsety*(nodes[props[i].nodey].smoothpos-nodes[props[i].noderef].smoothpos);
				//billboard
				Vector3 vdir=mposition-mCamera->getPosition();
				float vlen=vdir.length();
				if (vlen>100.0) {props[i].bbsnode[0]->setVisible(false);continue;};
				//normalize
				vdir=vdir/vlen;
				props[i].bbsnode[0]->setPosition(mposition-vdir*0.1);
			}
			if (props[i].beacontype=='w')
			{
				Vector3 mposition=nodes[props[i].noderef].smoothpos+props[i].offsetx*(nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos)+props[i].offsety*(nodes[props[i].nodey].smoothpos-nodes[props[i].noderef].smoothpos);
				props[i].light[0]->setPosition(mposition);
				props[i].bpos[0]+=dt*props[i].brate[0];//rotate baby!
				//billboard
				Vector3 vdir=mposition-mCamera->getPosition();
				float vlen=vdir.length();
				if (vlen>100.0) {props[i].bbsnode[0]->setVisible(false);continue;};
				//normalize
				vdir=vdir/vlen;
				props[i].bbsnode[0]->setPosition(mposition-vdir*0.1);
				bool visible=false;
				if (props[i].bpos[0]>1.0)
				{
					props[i].bpos[0]=0.0;
					visible=true;
				}
				visible = visible && enableAll;
				props[i].light[0]->setVisible(visible);
				props[i].bbsnode[0]->setVisible(visible);
			}
		}
	}
	//the flares
	bool keysleep=false;
	for (i=0; i<free_flare; i++)
	{
		// let the light blink
		if(flares[i].blinkdelay != 0)
		{
			flares[i].blinkdelay_curr -= dt;
			if(flares[i].blinkdelay_curr <= 0)
			{
				flares[i].blinkdelay_curr = flares[i].blinkdelay;
				flares[i].blinkdelay_state = !flares[i].blinkdelay_state;
			}
		}
		else
		{
			flares[i].blinkdelay_state = true;
		}
		//LogManager::getSingleton().logMessage(StringConverter::toString(flares[i].blinkdelay_curr));
		// manage light states
		bool isvisible = true; //this must be true to be able to switch on the frontlight
		if (flares[i].type == 'f') {
			materialFunctionMapper->toggleFunction(i, (lights==1));
			if (!lights)
				continue;
		} else if(flares[i].type == 'b') {
			isvisible = getBrakeLightVisible();
		} else if(flares[i].type == 'R') {
			if(engine)
				isvisible = getReverseLightVisible();
			else
				isvisible=false;
		} else if(flares[i].type == 'u' && flares[i].controlnumber != -1) {
			if(state==ACTIVATED) // no network!!
			{
				// networked customs are set directly, so skip this
				if (INPUTENGINE.getEventBoolValue(EV_TRUCK_LIGHTTOGGLE1 + (flares[i].controlnumber - 1)) && mTimeUntilNextToggle <= 0)
				{
					flares[i].controltoggle_status = ! flares[i].controltoggle_status;
					keysleep = true;
				}
			}
			isvisible = flares[i].controltoggle_status;

		} else if (flares[i].type == 'l') {
			isvisible = (blinkingtype == BLINK_LEFT || blinkingtype == BLINK_WARN);
		} else if (flares[i].type == 'r') {
			isvisible = (blinkingtype == BLINK_RIGHT || blinkingtype == BLINK_WARN);
		}
		// apply blinking
		isvisible = isvisible && flares[i].blinkdelay_state;

		// update material Bindings
		materialFunctionMapper->toggleFunction(i, isvisible);

		flares[i].snode->setVisible(isvisible);
		if (flares[i].light)
			flares[i].light->setVisible(isvisible && enableAll);
		flares[i].isVisible=isvisible;

		Vector3 normal=(nodes[flares[i].nodey].smoothpos-nodes[flares[i].noderef].smoothpos).crossProduct(nodes[flares[i].nodex].smoothpos-nodes[flares[i].noderef].smoothpos);
		normal.normalise();
		Vector3 mposition=nodes[flares[i].noderef].smoothpos+flares[i].offsetx*(nodes[flares[i].nodex].smoothpos-nodes[flares[i].noderef].smoothpos)+flares[i].offsety*(nodes[flares[i].nodey].smoothpos-nodes[flares[i].noderef].smoothpos);
		Vector3 vdir=mposition-mCamera->getPosition();
		float vlen=vdir.length();
		// not visible from 500m distance
		if (vlen > 500.0)
		{
			flares[i].snode->setVisible(false);
			continue;
		};
		//normalize
		vdir=vdir/vlen;
		float amplitude=normal.dotProduct(vdir);
		flares[i].snode->setPosition(mposition-0.1*amplitude*normal*flares[i].offsetz);
		flares[i].snode->setDirection(normal);
		float fsize = flares[i].size;
		if(fsize < 0)
		{
			amplitude=1;
			fsize*=-1;
		}
		if (flares[i].light)
		{
			flares[i].light->setPosition(mposition-0.2*amplitude*normal);
			// point the real light towards the ground a bit
			flares[i].light->setDirection(-normal - Vector3(0, 0.2, 0));
		}
		if (flares[i].isVisible)
		{
			if(amplitude>0)
			{
				flares[i].bbs->setDefaultDimensions(amplitude * fsize, amplitude * fsize);
				flares[i].snode->setVisible(true);
			}
			else
			{
				flares[i].snode->setVisible(false);
			}
		}
		//flares[i].bbs->_updateBounds();
	}
	if(keysleep)
		mTimeUntilNextToggle = 0.2;

}

void Beam::setBlinkType(blinktype blink)
{
#ifdef USE_OPENAL
	blinkingtype = blink;
	if(blink == BLINK_NONE)
		ssm->trigStop(trucknum, SS_TRIG_TURN_SIGNAL);
	else
		ssm->trigStart(trucknum, SS_TRIG_TURN_SIGNAL);
#endif //OPENAL
}

void Beam::autoBlinkReset()
{
	blinktype blink=getBlinkType();

	if(blink == BLINK_LEFT && hydrodirstate < -0.1)
		// passed the treshold: the turn signal gets locked
		blinktreshpassed = true;

	if(blink == BLINK_LEFT && blinktreshpassed && hydrodirstate > -0.1)
	{
		// steering wheel turned back: turn signal gets autmatically unlocked
		setBlinkType(BLINK_NONE);
		blinktreshpassed = false;
	}

	// same for the right turn signal
	if(blink == BLINK_RIGHT && hydrodirstate > 0.1)
		blinktreshpassed = true;

	if(blink == BLINK_RIGHT && blinktreshpassed && hydrodirstate < 0.1)
	{
		setBlinkType(BLINK_NONE);
		blinktreshpassed = false;
	}
}

void Beam::updateProps()
{
	int i;
	//the props
	for (i=0; i<free_prop; i++)
	{
		if (!props[i].snode) continue;
		Vector3 normal=(nodes[props[i].nodey].smoothpos-nodes[props[i].noderef].smoothpos).crossProduct(nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos);
		normal.normalise();
		//position
		Vector3 mposition=nodes[props[i].noderef].smoothpos+props[i].offsetx*(nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos)+props[i].offsety*(nodes[props[i].nodey].smoothpos-nodes[props[i].noderef].smoothpos);
		props[i].snode->setPosition(mposition+normal*props[i].offsetz);
		//orientation
		Vector3 refx=nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos;
		refx.normalise();
		Vector3 refy=refx.crossProduct(normal);
		Quaternion orientation=Quaternion(refx, normal, refy)*props[i].rot;
		props[i].snode->setOrientation(orientation);
		if (props[i].wheel)
		{
			//display wheel
			Quaternion brot=Quaternion(Degree(-59.0), Vector3::UNIT_X);
			brot=brot*Quaternion(Degree(hydrodirwheeldisplay*props[i].wheelrotdegree), Vector3::UNIT_Y);
			props[i].wheel->setPosition(mposition+normal*props[i].offsetz+orientation*props[i].wheelpos);
			props[i].wheel->setOrientation(orientation*brot);
		}
	}
	//we also consider airbrakes as props
	for (i=0; i<free_airbrake; i++) airbrakes[i]->updatePosition((float)airbrakeval/5.0);
}

void Beam::toggleCustomParticles()
{
	cparticle_mode = !cparticle_mode;
	for (int i=0; i<free_cparticle; i++)
	{
		cparticles[i].active=!cparticles[i].active;
		for (int j=0; j<cparticles[i].psys->getNumEmitters(); j++)
		{
			cparticles[i].psys->getEmitter(j)->setEnabled(cparticles[i].active);
		}
	}

#ifdef USE_ANGELSCRIPT
	//ScriptEvent - Particle Toggle
	ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_CPARTICLES_TOGGLE, trucknum);
#endif

}

void Beam::updateSoundSources()
{
#ifdef USE_OPENAL
	for (int i=0; i<free_soundsource; i++)
	{
		soundsources[i].ssi->setPosition(nodes[soundsources[i].nodenum].AbsPosition, nodes[soundsources[i].nodenum].Velocity);
	}
	//also this, so it is updated always, and for any vehicle
	ssm->modulate(trucknum, SS_MOD_AIRSPEED, nodes[0].Velocity.length()*1.9438);
	ssm->modulate(trucknum, SS_MOD_WHEELSPEED, WheelSpeed*3.6);
#endif //OPENAL
}

void Beam::updateVisual(float dt)
{
	int i;
	Vector3 ref=Vector3(0.0,1.0,0.0);
	autoBlinkReset();
	//sounds too
	updateSoundSources();

	if(deleting) return;
	if(debugVisuals) updateDebugOverlay();

	//dust
	DustManager::getSingleton().update(WheelSpeed);
	/*
	if (dustp && state==ACTIVATED) dustp->update(WheelSpeed);
	if (dripp) dripp->update(WheelSpeed);
	if (splashp) splashp->update(WheelSpeed);
	if (ripplep) ripplep->update(WheelSpeed);
	if (sparksp) sparksp->update(WheelSpeed);
	if (clumpp) clumpp->update(WheelSpeed);
	*/
	//update custom particle systems
	for (int i=0; i<free_cparticle; i++)
	{
			Vector3 pos=nodes[cparticles[i].emitterNode].smoothpos;
			Vector3 dir=pos-nodes[cparticles[i].directionNode].smoothpos;
		//dir.normalise();
		dir=fast_normalise(dir);
			cparticles[i].snode->setPosition(pos);
			for (int j=0; j<cparticles[i].psys->getNumEmitters(); j++)
			{
				cparticles[i].psys->getEmitter(j)->setDirection(dir);
			}
	}
	// update exhausts
	if (!disable_smoke && engine && exhausts.size() > 0)
	{
		std::vector < exhaust_t >::iterator it;
		for(it=exhausts.begin(); it!=exhausts.end(); it++)
		{
			if(!it->smoker)
				continue;
			Vector3 dir=nodes[it->emitterNode].smoothpos-nodes[it->directionNode].smoothpos;
			//			dir.normalise();
			ParticleEmitter *emit = it->smoker->getEmitter(0);
			it->smokeNode->setPosition(nodes[it->emitterNode].smoothpos);
			emit->setDirection(dir);
			if (engine->getSmoke()!=-1.0)
			{
				emit->setEnabled(true);
				emit->setColour(ColourValue(0.0,0.0,0.0,0.02+engine->getSmoke()*0.06));
				emit->setTimeToLive((0.02+engine->getSmoke()*0.06)/0.04);
			}
			else
			{
				emit->setEnabled(false);
			};
			emit->setParticleVelocity(1.0+engine->getSmoke()*2.0, 2.0+engine->getSmoke()*3.0);
		}
	}

	updateProps();

	for (i=0; i<free_aeroengine; i++) aeroengines[i]->updateVisuals();

	//wings
	float autoaileron=0;
	float autorudder=0;
	float autoelevator=0;
	if (autopilot)
	{
		autoaileron=autopilot->getAilerons();
		autorudder=autopilot->getRudder();
		autoelevator=autopilot->getElevator();
		autopilot->gpws_update();
	}
	autoaileron+=aileron;
	autorudder+=rudder;
	autoelevator+=elevator;
	if (autoaileron<-1.0) autoaileron=-1.0;
	if (autoaileron>1.0) autoaileron=1.0;
	if (autorudder<-1.0) autorudder=-1.0;
	if (autorudder>1.0) autorudder=1.0;
	if (autoelevator<-1.0) autoelevator=-1.0;
	if (autoelevator>1.0) autoelevator=1.0;
	for (i=0; i<free_wing; i++)
	{
		if (wings[i].fa->type=='a') wings[i].fa->setControlDeflection(autoaileron);
		if (wings[i].fa->type=='b') wings[i].fa->setControlDeflection(-autoaileron);
		if (wings[i].fa->type=='r') wings[i].fa->setControlDeflection(autorudder);
		if (wings[i].fa->type=='e' || wings[i].fa->type=='S' || wings[i].fa->type=='T') wings[i].fa->setControlDeflection(autoelevator);
		if (wings[i].fa->type=='f') wings[i].fa->setControlDeflection(flapangles[flap]);
		if (wings[i].fa->type=='c' || wings[i].fa->type=='V') wings[i].fa->setControlDeflection((autoaileron+autoelevator)/2.0);
		if (wings[i].fa->type=='d' || wings[i].fa->type=='U') wings[i].fa->setControlDeflection((-autoaileron+autoelevator)/2.0);
		if (wings[i].fa->type=='g') wings[i].fa->setControlDeflection((autoaileron+flapangles[flap])/2.0);
		if (wings[i].fa->type=='h') wings[i].fa->setControlDeflection((-autoaileron+flapangles[flap])/2.0);
		if (wings[i].fa->type=='i') wings[i].fa->setControlDeflection((-autoelevator+autorudder)/2.0);
		if (wings[i].fa->type=='j') wings[i].fa->setControlDeflection((autoelevator+autorudder)/2.0);
		wings[i].cnode->setPosition(wings[i].fa->flexit());
	}
	//setup commands for hydros
	hydroaileroncommand=autoaileron;
	hydroruddercommand=autorudder;
	hydroelevatorcommand=autoelevator;

	if(cabFadeMode>0 && dt > 0)
	{
		if(cabFadeTimer > 0)
			cabFadeTimer-=dt;
		if(cabFadeTimer < 0.1 && cabFadeMode == 1)
		{
			cabFadeMode=0;
			cabFade(0.4);
		}
		else if(cabFadeTimer < 0.1 && cabFadeMode == 2)
		{
			cabFadeMode=0;
			cabFade(1);
		}

		if(cabFadeMode == 1)
			cabFade(0.4 + 0.6 * cabFadeTimer/cabFadeTime);
		else if(cabFadeMode == 2)
			cabFade(1 - 0.6 * cabFadeTimer/cabFadeTime);
	}

	if (!skeleton)
	{
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].broken==1 && beams[i].mSceneNode) {beams[i].mSceneNode->detachAllObjects();beams[i].broken=2;}
			if (beams[i].mSceneNode!=0 && beams[i].type!=BEAM_INVISIBLE && beams[i].type!=BEAM_INVISIBLE_HYDRO && beams[i].type!=BEAM_VIRTUAL && !beams[i].disabled)
			{
				beams[i].mSceneNode->setPosition(beams[i].p1->smoothpos.midPoint(beams[i].p2->smoothpos));
				beams[i].mSceneNode->setOrientation(specialGetRotationTo(ref,beams[i].p1->smoothpos-beams[i].p2->smoothpos));
				//					beams[i].mSceneNode->setScale(default_beam_diameter/100.0,(beams[i].p1->smoothpos-beams[i].p2->smoothpos).length()/100.0,default_beam_diameter/100.0);
				beams[i].mSceneNode->setScale(beams[i].diameter, (beams[i].p1->smoothpos-beams[i].p2->smoothpos).length(), beams[i].diameter);
			};
		}
		for (i=0; i<free_wheel; i++)
		{
			if(vwheels[i].cnode) vwheels[i].cnode->setPosition(vwheels[i].fm->flexit());
		}
		if (cabMesh) cabNode->setPosition(cabMesh->flexit());
	}
	else
	{
		if(skeleton)
		{
			for (i=0; i<free_beam; i++)
			{
				if (beams[i].mSceneNode!=0 && !beams[i].disabled)
				{
					beams[i].mSceneNode->setPosition(beams[i].p1->smoothpos.midPoint(beams[i].p2->smoothpos));
					beams[i].mSceneNode->setOrientation(specialGetRotationTo(ref,beams[i].p1->smoothpos-beams[i].p2->smoothpos));
					beams[i].mSceneNode->setScale(skeleton_beam_diameter,(beams[i].p1->smoothpos-beams[i].p2->smoothpos).length(),skeleton_beam_diameter);
					//					beams[i].mSceneNode->setScale(default_beam_diameter/100.0,(beams[i].p1->smoothpos-beams[i].p2->smoothpos).length()/100.0,default_beam_diameter/100.0);
				};
			}
			for (i=0; i<free_wheel; i++)
			{
				vwheels[i].cnode->setPosition(vwheels[i].fm->flexit());
			}
			if (cabMesh) cabNode->setPosition(cabMesh->flexit());
		}
		if (skeleton == 2)
			updateSimpleSkeleton();
		//updateDebugOverlay();
	}
	//Flex body
	for (i=0; i<free_flexbody; i++) flexbodies[i]->flexit();

	if (netLabelNode && netMT && netMT->isVisible())
	{
		// this ensures that the nickname is always in a readable size
		netLabelNode->setPosition(position+Vector3(0, (maxy-miny), 0));
		Vector3 vdir=(position)-mCamera->getPosition();
		float vlen=vdir.length();
		float h = vlen/30.0;
		if(h<0.6)
			h=0.6;
		netMT->setCharacterHeight(h);
		if(vlen>1000)
			netMT->setCaption(networkUsername + "  (" + StringConverter::toString( (float)(ceil(vlen/100)/10.0) )+ " km)");
		else if (vlen>20 && vlen <= 1000)
			netMT->setCaption(networkUsername + "  (" + StringConverter::toString((int)vlen)+ " m)");
		else
			netMT->setCaption(networkUsername);

		//netMT->setAdditionalHeight((maxy-miny)+h+0.1);
		netMT->setVisible(true);
	}
}


//v=0: full detail
//v=1: no beams
void Beam::setDetailLevel(int v)
{
	if (v!=detailLevel)
	{
		if (detailLevel==0 && v==1)
		{
			//detach
			parentNode->removeChild(beamsRoot);
		}
		if (detailLevel==1 && v==0)
		{
			//attach
			parentNode->addChild(beamsRoot);
		}
		detailLevel=v;
	}

}

void Beam::preMapLabelRenderUpdate(bool mode, float charheight)
{
	static float orgcharheight=0;
	if(mode && netLabelNode)
	{
		netMT->showOnTop(true);
		orgcharheight = netMT->getCharacterHeight();
		netMT->setCharacterHeight(charheight);
		//netMT->setAdditionalHeight(0);
		netMT->setVisible(false);
	} else if(!mode && netLabelNode)
	{
		netMT->showOnTop(false);
		netMT->setCharacterHeight(orgcharheight);
		netMT->setVisible(true);
	}
}

void Beam::showSkeleton(bool meshes, bool newMode)
{
	if(lockSkeletonchange)
		return;
	lockSkeletonchange=true;
	int i;

	skeleton=1;

	if(newMode)
		skeleton=2;

	if(meshes)
	{
		cabFadeMode=1;
		cabFadeTimer=cabFadeTime;
	} else
	{
		cabFadeMode=-1;
		// directly hide meshes, no fading
		cabFade(0);
	}
	for (i=0; i<free_wheel; i++)
	{
		if(vwheels[i].cnode) vwheels[i].cnode->setVisible(false);
		if(vwheels[i].fm) vwheels[i].fm->setVisible(false);
	}
	for (i=0; i<free_prop; i++)
	{
		if(props[i].snode)
			setMeshWireframe(props[i].snode, true);
		if(props[i].wheel)
			setMeshWireframe(props[i].wheel, true);
	}

	if(!newMode)
	{
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].mSceneNode && beams[i].mEntity)
			{
				if (!beams[i].broken && beams[i].mSceneNode->numAttachedObjects()==0)
					beams[i].mSceneNode->attachObject(beams[i].mEntity);
				//material
				beams[i].mEntity->setMaterialName("mat-beam-0");
				beams[i].mEntity->setCastShadows(false);
			}
		}
	}else
	{
		if(simpleSkeletonNode)
		{
			updateSimpleSkeleton();
			simpleSkeletonNode->setVisible(true);
		}
	}

	// hide mesh wheels
	for (i=0; i<free_wheel; i++)
	{
		if(vwheels[i].fm && vwheels[i].meshwheel)
		{
			Entity *e = ((FlexMeshWheel*)(vwheels[i].fm))->getRimEntity();
			if(e)
				e->setVisible(false);
		}
	}

	// wireframe drawning for flexbody
	for(i=0; i<free_flexbody; i++)
	{
		SceneNode *s = flexbodies[i]->getSceneNode();
		if(!s)
			continue;
		setMeshWireframe(s, true);
	}

	for(std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
		if (it->beam->disabled)
			it->beam->mSceneNode->detachAllObjects();

	for(std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
		if (it->lockTruck && it->lockTruck->getTruckName() != getTruckName())
			it->lockTruck->showSkeleton();

	lockSkeletonchange=false;

#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_SKELETON_TOGGLE, trucknum);
#endif

}

void Beam::hideSkeleton(bool newMode)
{
	if(lockSkeletonchange)
		return;
	lockSkeletonchange=true;
	int i;
	skeleton=0;

	if(cabFadeMode>=0)
	{
		cabFadeMode=2;
		cabFadeTimer=cabFadeTime;
	} else
	{
		cabFadeMode=-1;
		// directly show meshes, no fading
		cabFade(1);
	}


	for (i=0; i<free_wheel; i++)
	{
		if(vwheels[i].cnode) vwheels[i].cnode->setVisible(true);
		if(vwheels[i].fm) vwheels[i].fm->setVisible(true);
	}
	for (i=0; i<free_prop; i++)
	{
		if(props[i].snode)
			setMeshWireframe(props[i].snode, false);
		if(props[i].wheel)
			setMeshWireframe(props[i].wheel, false);
	}

	if(!newMode)
	{
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].mSceneNode)
			{
				if (beams[i].type==BEAM_VIRTUAL || beams[i].type==BEAM_INVISIBLE || beams[i].type==BEAM_INVISIBLE_HYDRO) beams[i].mSceneNode->detachAllObjects();
				//material
				if (beams[i].type==BEAM_HYDRO || beams[i].type==BEAM_MARKED) beams[i].mEntity->setMaterialName("tracks/Chrome");
				else beams[i].mEntity->setMaterialName(default_beam_material);
			}
		}
	}else
	{
		if(simpleSkeletonNode)
			simpleSkeletonNode->setVisible(false);
	}

	// show mesh wheels
	for (i=0; i<free_wheel; i++)
	{
		if(vwheels[i].fm && vwheels[i].meshwheel)
		{
			Entity *e = ((FlexMeshWheel *)(vwheels[i].fm))->getRimEntity();
			if(e)
				e->setVisible(true);
		}
	}

	// normal drawning for flexbody
	for(i=0; i<free_flexbody; i++)
	{
		SceneNode *s = flexbodies[i]->getSceneNode();
		if(!s)
			continue;
		setMeshWireframe(s, false);
	}

	for(std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
		if (it->beam->disabled)
			it->beam->mSceneNode->detachAllObjects();

	for(std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
		if (it->lockTruck && it->lockTruck->getTruckName() != getTruckName())
			it->lockTruck->hideSkeleton();

	lockSkeletonchange=false;
}

void Beam::fadeMesh(SceneNode *node, float amount)
{
	for(int a=0;a<node->numAttachedObjects();a++)
	{
		Entity *e = (Entity *)node->getAttachedObject(a);
		MaterialPtr m = e->getSubEntity(0)->getMaterial();
		if(m.getPointer() == 0)
			continue;
		for(int x=0;x<m->getNumTechniques();x++)
		{
			for(int y=0;y<m->getTechnique(x)->getNumPasses();y++)
			{
				// TODO: fix this
				//m->getTechnique(x)->getPass(y)->setAlphaRejectValue(0);
				if(m->getTechnique(x)->getPass(y)->getNumTextureUnitStates() > 0)
					m->getTechnique(x)->getPass(y)->getTextureUnitState(0)->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_MANUAL, 1.0, amount);
			}
		}
	}
}

float Beam::getAlphaRejection(SceneNode *node)
{
	for(int a=0;a<node->numAttachedObjects();a++)
	{
		Entity *e = (Entity *)node->getAttachedObject(a);
		MaterialPtr m = e->getSubEntity(0)->getMaterial();
		if(m.getPointer() == 0)
			continue;
		for(int x=0;x<m->getNumTechniques();x++)
		{
			for(int y=0;y<m->getTechnique(x)->getNumPasses();y++)
			{
				return m->getTechnique(x)->getPass(y)->getAlphaRejectValue();
			}
		}
	}
	return 0;
}

void Beam::setAlphaRejection(SceneNode *node, float amount)
{
	for(int a=0;a<node->numAttachedObjects();a++)
	{
		Entity *e = (Entity *)node->getAttachedObject(a);
		MaterialPtr m = e->getSubEntity(0)->getMaterial();
		if(m.getPointer() == 0)
			continue;
		for(int x=0;x<m->getNumTechniques();x++)
		{
			for(int y=0;y<m->getTechnique(x)->getNumPasses();y++)
			{
				m->getTechnique(x)->getPass(y)->setAlphaRejectValue((unsigned char)amount);
				return;
			}
		}
	}
}
void Beam::setMeshWireframe(SceneNode *node, bool value)
{
	for(int a=0;a<node->numAttachedObjects();a++)
	{
		Entity *e = (Entity *)node->getAttachedObject(a);
		for(int se=0;se<(int)e->getNumSubEntities();se++)
		{
			MaterialPtr m = e->getSubEntity(se)->getMaterial();
			if(m.getPointer() == 0)
				continue;
			for(int x=0;x<m->getNumTechniques();x++)
				for(int y=0;y<m->getTechnique(x)->getNumPasses();y++)
					if(value)
						m->getTechnique(x)->getPass(y)->setPolygonMode(Ogre::PM_WIREFRAME);
					else
						m->getTechnique(x)->getPass(y)->setPolygonMode(Ogre::PM_SOLID);
		}
	}
}

void Beam::setMeshVisibility(bool visible)
{
	int i=0;
	for(i=0;i<free_prop;i++)
	{
		if(props[i].snode) props[i].snode->setVisible(visible);
		if(props[i].wheel) props[i].wheel->setVisible(visible);
		if(props[i].bbsnode[0]) props[i].bbsnode[0]->setVisible(visible);
		if(props[i].bbsnode[1]) props[i].bbsnode[1]->setVisible(visible);
		if(props[i].bbsnode[2]) props[i].bbsnode[2]->setVisible(visible);
		if(props[i].bbsnode[3]) props[i].bbsnode[3]->setVisible(visible);
	}
	for(i=0;i<free_flexbody;i++)
	{
		flexbodies[i]->setVisible(visible);
	}
	for(i=0;i<free_wheel;i++)
	{
		if(vwheels[i].cnode) vwheels[i].cnode->setVisible(visible);
		if(vwheels[i].fm) vwheels[i].fm->setVisible(visible);

	}
	if(cabMesh) cabNode->setVisible(visible);
	meshesVisible = visible;

	// apply to the locked truck
	for(std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
		if (it->lockTruck && it->lockTruck->getTruckName() != getTruckName())
			it->lockTruck->setMeshVisibility(visible);
}

void Beam::cabFade(float amount)
{
	static float savedCabAlphaRejection = 0;

	// truck cab
	if (cabMesh)
	{
		if(amount == 0)
		{
			cabNode->setVisible(false);
		} else
		{
			if (amount == 1)
				cabNode->setVisible(true);
			if(savedCabAlphaRejection == 0)
				savedCabAlphaRejection = getAlphaRejection(cabNode);
			if(amount == 1)
				setAlphaRejection(cabNode, savedCabAlphaRejection);
			else if (amount < 1)
				setAlphaRejection(cabNode, 0);
			fadeMesh(cabNode, amount);
		}
	}

	// wings
	for (int i=0; i<free_wing; i++)
	{
		if(amount == 0)
		{
			wings[i].cnode->setVisible(false);
		} else
		{
			if (amount == 1)
				wings[i].cnode->setVisible(true);
			fadeMesh(wings[i].cnode, amount);
		}
	}
}

void Beam::tieToggle(Beam** trucks, int trucksnum, int group)
{
	//export tie commands
	if (state==ACTIVATED && forwardcommands)
	{
		int i;
		for (i=0; i<trucksnum; i++)
		{
			if(!trucks[i]) continue;
			if (trucks[i]->state==DESACTIVATED && trucks[i]->importcommands)
				trucks[i]->tieToggle(trucks, trucksnum, group);
		}
	}

	// iterate over all ties
	for(std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
	{
		// only handle ties with correct group
		if (group != -1 && (it->group != -1 && it->group != group))
			continue;

		// if tied, untie it. And the other way round
		if (it->tied)
		{
			// tie is locked and should get unlocked

			// not tied and stop tying
			it->tied  = false;
			it->tying = false;
			if(it->lockedto) it->lockedto->used--;
			// disable the ties beam
			it->beam->disabled = 1;
			it->beam->mSceneNode->detachAllObjects();
		} else
		{
			// tie is unlocked and should get locked

			// search new remote ropable to lock to
			float mindist = it->beam->refL;
			node_t *shorter=0;
			Beam *shtruck=0;
			ropable_t *locktedto=0;
			// iterate over all trucks
			for (int t=0; t<trucksnum; t++)
			{
				if(!trucks[t]) continue;
				if (trucks[t]->state==SLEEPING) continue;
				// and their ropables
				for(std::vector <ropable_t>::iterator itr = trucks[t]->ropables.begin(); itr!=trucks[t]->ropables.end(); itr++)
				{
					// if the ropable is not multilock and used, then discard this ropable
					if(!itr->multilock && itr->used)
						continue;

					// calculate the distance and record the nearest ropable
					float dist = (it->beam->p1->AbsPosition - itr->node->AbsPosition).length();
					if (dist < mindist)
					{
						mindist = dist;
						shorter = itr->node;
						shtruck = trucks[t];
						locktedto = &(*itr);
					}
				}
			}
			// if we found a ropable, then tie towards it
			if (shorter)
			{
				//okay, we have found a rope to tie

				// enable the beam and visually display the beam
				it->beam->disabled = 0;
				if (it->beam->mSceneNode->numAttachedObjects() == 0)
					it->beam->mSceneNode->attachObject(it->beam->mEntity);

				// now trigger the tying action
				it->beam->p2 = shorter;
				it->beam->p2truck = shtruck;
				it->beam->stress = 0;
				it->beam->L = it->beam->refL;
				it->tied  = true;
				it->tying = true;
				it->lockedto = locktedto;
				it->lockedto->used++;
			}

		}
	}
#ifdef USE_ANGELSCRIPT
	//ScriptEvent - Tie toggle
	ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_TIE_TOGGLE, trucknum);
#endif
}

void Beam::ropeToggle(Beam** trucks, int trucksnum, int group)
{
	// iterate over all ropes
	for(std::vector <rope_t>::iterator it = ropes.begin(); it!=ropes.end(); it++)
	{
		// only handle ropes with correct group
		if (group != -1 && (it->group != -1 && it->group != group))
			continue;

		if (it->locked == LOCKED || it->locked == PRELOCK)
		{
			// we unlock ropes
			it->locked = UNLOCKED;
			// remove node locking
			if(it->lockedto)         it->lockedto->lockednode=0;
			if(it->lockedto_ropable) it->lockedto_ropable->used--;
			it->lockedto = 0;
			it->lockedtruck = 0;
		} else
		{
			//we lock ropes
			// search new remote ropable to lock to
			float mindist = it->beam->L;
			node_t *shorter=0;
			Beam *shtruck=0;
			ropable_t *rop=0;
			// iterate over all trucks
			for (int t=0; t<trucksnum; t++)
			{
				if(!trucks[t]) continue;
				if (trucks[t]->state==SLEEPING) continue;
				// and their ropables
				for(std::vector <ropable_t>::iterator itr = trucks[t]->ropables.begin(); itr!=trucks[t]->ropables.end(); itr++)
				{
					// if the ropable is not multilock and used, then discard this ropable
					if(!itr->multilock && itr->used)
						continue;

					// calculate the distance and record the nearest ropable
					float dist = (it->beam->p1->AbsPosition - itr->node->AbsPosition).length();
					if (dist < mindist)
					{
						mindist = dist;
						shorter = itr->node;
						shtruck = trucks[t];
						rop     = &(*itr);
					}
				}
			}
			// if we found a ropable, then lock it
			if (shorter)
			{
				//okay, we have found a rope to tie
				it->lockedto    = shorter;
				it->lockedtruck = shtruck;
				it->locked      = PRELOCK;
				it->lockedto_ropable = rop;
				it->lockedto_ropable->used++;
			}
		}
	}
}

void Beam::hookToggle(Beam** trucks, int trucksnum, int group)
{
	// iterate over all hooks
	for(std::vector <hook_t>::iterator it = hooks.begin(); it!=hooks.end(); it++)
	{
		// only handle hooks with correct group
		if (group != -1 && (it->group != -1 && it->group != group))
			continue;

		if (it->locked == LOCKED || it->locked == PRELOCK)
		{
			// we unlock ropes
			it->locked = UNLOCKED;
			// remove node locking
			if (it->lockNode) it->lockNode->lockednode=0;
			it->lockNode = 0;
			it->lockTruck = 0;
		} else
		{
			// we lock hooks
			// search new remote ropable to lock to
			float mindist = 0.4f; //hardcoded max distance to target node of 40 centimeters
			node_t *shorter=0;
			Beam *shtruck=0;
			// iterate over all trucks
			for (int t=0; t<trucksnum; t++)
			{
				if(!trucks[t]) continue;
				if (trucks[t]->state==SLEEPING) continue;

				// do we lock against all nodes or just against ropables?
				bool found = false;
				if(it->lockNodes)
				{
					// all nodes, so walk them
					for (int i=0; i<trucks[t]->free_node; i++)
					{
						// exclude this local truck and the current hooknode from the locking search
						if(this == trucks[t] && i == it->hookNode->id)
							continue;

						// measure distance
						if ((it->hookNode->AbsPosition - trucks[t]->nodes[i].AbsPosition).length() < mindist)
						{
							// we found a node, lock to it
							it->lockNode  = &(trucks[t]->nodes[i]);
							it->lockTruck = trucks[t];
							it->locked    = PRELOCK;
							found         = true; // dont check the other trucks
							break;
						}
					}
				} else
				{
					// we lock against ropables

					// and their ropables
					for(std::vector <ropable_t>::iterator itr = trucks[t]->ropables.begin(); itr!=trucks[t]->ropables.end(); itr++)
					{
						// if the ropable is not multilock and used, then discard this ropable
						if(!itr->multilock && itr->used)
							continue;

						// calculate the distance and record the nearest ropable
						float dist = (it->hookNode->AbsPosition - itr->node->AbsPosition).length();
						if (dist < mindist)
						{
							mindist = dist;
							shorter = itr->node;
							shtruck = trucks[t];
						}
					}
				}
				// if we found a ropable, then lock it
				if (shorter)
				{
					// we found a ropable, lock to it
					it->lockNode  = shorter;
					it->lockTruck = shtruck;
					it->locked    = PRELOCK;
					found         = true; // dont check the other trucks
				}

				if(found)
					// if we found some lock, we wont check all other trucks
					break;
			}
		}
	}
}

void Beam::parkingbrakeToggle()
{
	parkingbrake=!parkingbrake;

#ifdef USE_OPENAL
	if (parkingbrake)
		ssm->trigStart(trucknum, SS_TRIG_PARK);
	else
		ssm->trigStop(trucknum, SS_TRIG_PARK);
#endif // USE_OPENAL

#ifdef USE_ANGELSCRIPT
	//ScriptEvent - Parking Brake toggle
	ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_PARKINGBREAK_TOGGLE, trucknum);
#endif
}

void Beam::beaconsToggle()
{
	bool enableLight = true;
	if(flaresMode==0)
		return;
	if(flaresMode==1)
		enableLight=false;
	int i;
	beacon=!beacon;
	for (i=0; i<free_prop; i++)
	{
		if (props[i].beacontype=='b')
		{
			props[i].light[0]->setVisible(beacon && enableLight);
			props[i].bbsnode[0]->setVisible(beacon);
			if(props[i].bbs[0] && beacon && !props[i].bbsnode[0]->numAttachedObjects())
				props[i].bbsnode[0]->attachObject(props[i].bbs[0]);
			else if(props[i].bbs[0] && !beacon)
				props[i].bbsnode[0]->detachAllObjects();
		}
		else if (props[i].beacontype=='R' || props[i].beacontype=='L')
		{
			props[i].bbsnode[0]->setVisible(beacon);
			if(props[i].bbs[0] && beacon && !props[i].bbsnode[0]->numAttachedObjects())
				props[i].bbsnode[0]->attachObject(props[i].bbs[0]);
			else if(props[i].bbs[0] && !beacon)
				props[i].bbsnode[0]->detachAllObjects();
		}
		else if (props[i].beacontype=='p')
		{
			for (int k=0; k<4; k++)
			{
				props[i].light[k]->setVisible(beacon && enableLight);
				props[i].bbsnode[k]->setVisible(beacon);
				if(props[i].bbs[k] && beacon && !props[i].bbsnode[k]->numAttachedObjects())
					props[i].bbsnode[k]->attachObject(props[i].bbs[k]);
				else if(props[i].bbs[k] && !beacon)
					props[i].bbsnode[k]->detachAllObjects();
			}
		} else
		{
			for (int k=0; k<4; k++)
			{
				if(props[i].light[k])props[i].light[k]->setVisible(beacon && enableLight);
				if(props[i].bbsnode[k])props[i].bbsnode[k]->setVisible(beacon);
				if(props[i].bbs[k] && beacon && !props[i].bbsnode[k]->numAttachedObjects())
					props[i].bbsnode[k]->attachObject(props[i].bbs[k]);
				else if(props[i].bbs[k] && !beacon)
					props[i].bbsnode[k]->detachAllObjects();
			}
		}
	}

#ifdef USE_ANGELSCRIPT
	//ScriptEvent - Beacon toggle
	ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_BEACONS_TOGGLE, trucknum);
#endif
}

void Beam::setReplayMode(bool rm)
{
	if (!replay) return;
	if (replaymode && !rm)
	{
		replaypos=0;
		oldreplaypos=-1;
	}

	replaymode = rm;
	replay->setVisible(replaymode);
}


void Beam::updateDebugOverlay()
{
	if(!debugVisuals) return;
	if(nodedebugstate<0)
	{
		LogManager::getSingleton().logMessage("initializing debugVisuals with mode "+StringConverter::toString(debugVisuals));
		if(debugVisuals == 1 || (debugVisuals >= 3 && debugVisuals <= 5))
		{
			// add node labels
			for(int i=0; i<free_node; i++)
			{
				debugtext_t t;
				char nodeName[255]="", entName[255]="";
				sprintf(nodeName, "%s-nodesDebug-%d", truckname, i);
				sprintf(entName, "%s-nodesDebug-%d-Ent", truckname, i);
				Entity *b = tsm->createEntity(entName, "beam.mesh");
				t.id=i;
				t.txt = new MovableText(nodeName, "n"+StringConverter::toString(i));
				t.txt->setFontName("highcontrast_black");
				t.txt->setTextAlignment(MovableText::H_LEFT, MovableText::V_BELOW);
				//t.txt->setAdditionalHeight(0);
				t.txt->showOnTop(true);
				t.txt->setCharacterHeight(0.5);
				t.txt->setColor(ColourValue::White);

				t.node = tsm->getRootSceneNode()->createChildSceneNode();
				t.node->attachObject(t.txt);
				t.node->attachObject(b);
				t.node->setScale(Vector3(0.05,0.05,0.05));

				t.node->setPosition(nodes[i].smoothpos);
				nodes_debug.push_back(t);
			}
		} else if(debugVisuals == 2 || debugVisuals == 3 || (debugVisuals >= 6 && debugVisuals <= 11))
		{
			// add beam labels
			for(int i=0; i<free_beam; i++)
			{
				debugtext_t t;
				char nodeName[255]="";
				sprintf(nodeName, "%s-beamsDebug-%d", truckname, i);
				t.id=i;
				t.txt = new MovableText(nodeName, "b"+StringConverter::toString(i));
				t.txt->setFontName("highcontrast_black");
				t.txt->setTextAlignment(MovableText::H_LEFT, MovableText::V_BELOW);
				//t.txt->setAdditionalHeight(0);
				t.txt->showOnTop(true);
				t.txt->setCharacterHeight(1);
				t.txt->setColor(ColourValue::White);

				t.node = tsm->getRootSceneNode()->createChildSceneNode();
				t.node->attachObject(t.txt);

				Vector3 pos = beams[i].p1->smoothpos - (beams[i].p1->smoothpos - beams[i].p2->smoothpos)/2;
				t.node->setPosition(pos);
				t.node->setScale(Vector3(0.1,0.1,0.1));
				beams_debug.push_back(t);
			}
		}

		nodedebugstate=0;
		// update now
	}
	switch(debugVisuals)
	{
	case 0: // off
		return;
	case 1: // node-numbers
		// not written dynamically
		for(std::vector<debugtext_t>::iterator it=nodes_debug.begin(); it!=nodes_debug.end();it++)
			it->node->setPosition(nodes[it->id].smoothpos);
		break;
	case 2: // beam-numbers
		// not written dynamically
		for(std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
		break;
	case 3: // node-and-beam-numbers
		// not written dynamically
		for(std::vector<debugtext_t>::iterator it=nodes_debug.begin(); it!=nodes_debug.end();it++)
			it->node->setPosition(nodes[it->id].smoothpos);
		for(std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
		break;
	case 4: // node-mass
		for(std::vector<debugtext_t>::iterator it=nodes_debug.begin(); it!=nodes_debug.end();it++)
		{
			it->node->setPosition(nodes[it->id].smoothpos);
			it->txt->setCaption(StringConverter::toString(nodes[it->id].mass));
		}
		break;
	case 5: // node-locked
		for(std::vector<debugtext_t>::iterator it=nodes_debug.begin(); it!=nodes_debug.end();it++)
		{
			it->txt->setCaption((nodes[it->id].locked)?"locked":"unlocked");
			it->node->setPosition(nodes[it->id].smoothpos);
		}
		break;
	case 6: // beam-compression
		for(std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			int scale=(int)(beams[it->id].scale * 100);
			it->txt->setCaption(StringConverter::toString(scale));
		}
		break;
	case 7: // beam-broken
		for(std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			it->txt->setCaption((beams[it->id].broken)?"BROKEN":"");
		}
		break;
	case 8: // beam-stress
		for(std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			it->txt->setCaption(StringConverter::toString((float) fabs(beams[it->id].stress)));
		}
		break;
	case 9: // beam-strength
		for(std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			it->txt->setCaption(StringConverter::toString(beams[it->id].strength));
		}
		break;
	case 10: // beam-hydros
		for(std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			int v = (beams[it->id].L / beams[it->id].Lhydro) * 100;
			it->txt->setCaption(StringConverter::toString(v));
		}
		break;
	case 11: // beam-commands
		for(std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			int v = (beams[it->id].L / beams[it->id].commandLong) * 100;
			it->txt->setCaption(StringConverter::toString(v));
		}
		break;
	}
}

void Beam::updateNetworkInfo()
{
#ifdef USE_SOCKETW
	if(!net) return;
	bool remote = (state == NETWORKED);
	if(remote)
	{
		client_t *c = net->getClientInfo(sourceid);
		if(!c) return;
		networkUsername = String(c->user_name);
		networkAuthlevel = c->user_authlevel;
	} else
	{
		client_info_on_join *info = net->getLocalUserData();
		if(!info) return;
		if(!strlen(info->nickname)) return;
		networkUsername = String(info->nickname);
		networkAuthlevel = info->authstatus;
	}

	if (netLabelNode && netMT)
	{
		// ha, this caused the empty caption bug, but fixed now since we change the caption if its empty:
		netMT->setCaption(networkUsername);
		if(networkAuthlevel & AUTH_ADMIN)
		{
			netMT->setFontName("highcontrast_red");
		} else if(networkAuthlevel & AUTH_RANKED)
		{
			netMT->setFontName("highcontrast_green");
		} else
		{
			netMT->setFontName("highcontrast_black");
		}
		netLabelNode->setVisible(true);
	}
	else
	{
		char wname[256];
		sprintf(wname, "netlabel-%s",truckname);
		netMT = new MovableText(wname, ColoredTextAreaOverlayElement::StripColors(networkUsername));
		netMT->setFontName("highcontrast_black");
		netMT->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
		//netMT->setAdditionalHeight(2);
		netMT->showOnTop(false);
		netMT->setCharacterHeight(2);
		netMT->setColor(ColourValue::White);

		if(networkAuthlevel & AUTH_ADMIN)
		{
			netMT->setFontName("highcontrast_red");
		} else if(networkAuthlevel & AUTH_RANKED)
		{
			netMT->setFontName("highcontrast_green");
		} else
		{
			netMT->setFontName("highcontrast_black");
		}

		netLabelNode=parentNode->createChildSceneNode();
		netLabelNode->attachObject(netMT);
		netLabelNode->setPosition(position);
		netLabelNode->setVisible(true);
		deletion_sceneNodes.push_back(netLabelNode);
		deletion_Objects.push_back(netMT);
	}
#endif //SOCKETW
}

void Beam::deleteNetTruck()
{
	//park and recycle vehicle
	state=RECYCLE;
	if(netMT)
		netMT->setCaption("");
	resetPosition(100000, 100000, false, 100000);
	netLabelNode->setVisible(false);
}

void *threadstart(void* vid)
{
#ifdef USE_CRASHRPT
	if(SETTINGS.getSetting("NoCrashRpt").empty())
	{
		// add the crash handler for this thread
		CrThreadAutoInstallHelper cr_thread_install_helper();
		assert(cr_thread_install_helper.m_nInstallStatus==0);
	}
#endif //USE_CRASHRPT

	// 64 bit systems does have longer addresses!
	long int id;
	id=(long int)vid;
	Beam *beam=threadbeam[id];

	try
	{
		//additional exception handler required, otherwise RoR just crashes upon exception
		while (1)
		{
			//wait signal
			pthread_mutex_lock(&beam->work_mutex);

			//signal end
			pthread_mutex_lock(&beam->done_count_mutex);
			beam->done_count--;
			pthread_cond_signal(&beam->done_count_cv);
			pthread_mutex_unlock(&beam->done_count_mutex);

			pthread_cond_wait(&beam->work_cv, &beam->work_mutex);
			pthread_mutex_unlock(&beam->work_mutex);
			//do work
			beam->threadentry(id);
		}
	} catch(Ogre::Exception& e)
	{
		// try to shutdown input system upon an error
		if(InputEngine::instanceExists()) // this prevents the creating of it, if not existing
			INPUTENGINE.prepareShutdown();

		String url = "http://wiki.rigsofrods.com/index.php?title=Error_" + StringConverter::toString(e.getNumber())+"#"+e.getSource();
		showWebError("An exception has occured!", e.getFullDescription(), url);
	}
	pthread_exit(NULL);
	return NULL;
}

float Beam::getHeadingDirectionAngle()
{
	int refnode = cameranodepos[0];
	int dirnode = cameranodedir[0];
	if(refnode==-1 || dirnode == -1)
		return 0;
	Vector3 idir=nodes[refnode].RelPosition - nodes[dirnode].RelPosition;
	return atan2(idir.dotProduct(Vector3::UNIT_X), idir.dotProduct(-Vector3::UNIT_Z));
}

bool Beam::getReverseLightVisible()
{
	if(state==NETWORKED)
		return netReverseLight;
	if (!engine) return 0;
	return (engine->getGear() < 0);
}

// Utility functions ///////////////////////////////////////////////////////////
beam_t* Beam::getBeam(unsigned int node1ID, unsigned int node2ID)
{

	for(unsigned  int j = 0; j < (unsigned  int)free_beam; ++j)
		if( (beams[j].p1->id == (int)node1ID && beams[j].p2->id == (int)node2ID) ||
			(beams[j].p2->id == (int)node1ID && beams[j].p1->id == (int)node2ID) )
			return &beams[j];

	return NULL;
}

beam_t* Beam::getBeam(node_t* node1, node_t* node2)
{
	// check for nulls
	if( !node1 || !node2 ) return NULL;
	return getBeam( node1->id, node2->id);
}

node_t* Beam::getNode(unsigned int id)
{
	for( unsigned int i = 0 ; i < (unsigned  int)free_node; ++i)
		if( nodes[i].id == (int)id )
			return &nodes[i];

	// node not found
	return NULL;
}

//Returns the number of active (non bounded) beams connected to a node
int Beam::nodeBeamConnections(int nodeid)
{
	int totallivebeams=0;
	for (unsigned int ni=0; ni<nodebeamconnections[nodeid].size(); ++ni)
	{
		if (!beams[nodebeamconnections[nodeid][ni]].disabled && !beams[nodebeamconnections[nodeid][ni]].bounded) totallivebeams++;
	}
	return totallivebeams;
}

bool Beam::isTied()
{
	for(std::vector <tie_t>::iterator it=ties.begin(); it!=ties.end();it++)
		if(it->tied)
			return true;
	return false;
}

bool Beam::isLocked()
{
	for(std::vector <hook_t>::iterator it=hooks.begin(); it!=hooks.end();it++)
		if(it->locked==LOCKED)
			return true;
	return false;
}
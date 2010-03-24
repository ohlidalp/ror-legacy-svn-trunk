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
#ifndef __Beam_H__
#define __Beam_H__

#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#include <windows.h>
	// DO NOT ENABLE THIS IN RELEASE BUILDS:
	//#define TIMING 1
	#undef TIMING
#else
	#undef TIMING
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

using namespace Ogre;

#define MAX_TRUCKS 64

#include <pthread.h>
#include <string>

#include "SocketW.h"
#include "rornet.h"
//#include "OgreTerrainSceneManager.h"
#include "MovableText.h"
#include "engine.h"
#include "networkinfo.h"
#include "CacheSystem.h"
#include "aeroengine.h"
#include "CmdKeyInertia.h"
#include "skin.h"
#include "Differentials.h"
#include "approxmath.h"
#include "PositionStorage.h"
#include "Streamable.h"
#include "groundmodel.h"
#include "rormemory.h"
#include <vector>

//#include "scriptCommands.h"
#include <vector>
class Airbrake;
class Flexable;
class FlexBody;

//#include "collisions.h"
typedef struct _collision_box collision_box_t;

#define THREAD_MONO 0
#define THREAD_HT 1
#define THREAD_HT2 2

#define MAX_NODES 1000
#define MAX_BEAMS 5000
#define MAX_ROTATORS 20
#define MAX_CONTACTERS 2000
#define MAX_HYDROS 1000
#define MAX_WHEELS 64
#define MAX_SUBMESHES 500
#define MAX_TEXCOORDS 3000
#define MAX_CABS 3000
#define MAX_SHOCKS MAX_BEAMS
#define MAX_ROPES 64
#define MAX_ROPABLES 64
#define MAX_TIES 64
//#define MAX_FLARES 200 // transformed into vector
#define MAX_PROPS 200
#define MAX_COMMANDS 48
#define MAX_CAMERAS 10
#define MAX_RIGIDIFIERS 100

#define MAX_FLEXBODIES 64

#define MAX_AEROENGINES 8

#define MAX_SCREWPROPS 8

#define MAX_SOUNDSCRIPTS_PER_TRUCK 128

#define DEFAULT_RIGIDIFIER_SPRING 1000000.0
#define DEFAULT_RIGIDIFIER_DAMP 50000.0

#define MAX_AIRBRAKES 20

#define DEFAULT_SPRING 9000000.0
//should be 60000
#define DEFAULT_DAMP 12000.0
//#define DEFAULT_DAMP 60000.0
//mars
//#define DEFAULT_GRAVITY -3.8
//earth
#define DEFAULT_GRAVITY -9.8
#define DEFAULT_DRAG 0.05
#define DEFAULT_BEAM_DIAMETER 0.05
#define DEFAULT_COLLISION_RANGE 0.02f
#define MIN_BEAM_LENGTH 0.1f
#define INVERTED_MIN_BEAM_LENGTH 1.0f/MIN_BEAM_LENGTH
#define BEAM_SKELETON_DIAMETER 0.01

#define DEFAULT_WATERDRAG 10.0
//buoyancy force per node in Newton
//#define DEFAULT_BUOYANCY 700.0

// version 1 = pre 0.32
// version 2 = post 0.32
#define TRUCKFILEFORMATVERSION 3

#define IRON_DENSITY 7874.0
#define BEAM_BREAK 1000000.0
#define BEAM_DEFORM 400000.0
#define BEAM_CREAK_DEFAULT  100000.0
#define WHEEL_FRICTION_COEF 2.0
#define CHASSIS_FRICTION_COEF 0.5 //Chassis has 1/4 the friction of wheels.
#define SPEED_STOP 0.2

#define MAX_PRESSURE_BEAMS 4000

#define STAB_RATE 0.1

#define BEAM_NORMAL 0
#define BEAM_HYDRO 1
#define BEAM_VIRTUAL 2
#define BEAM_MARKED 3
#define BEAM_INVISIBLE 4
#define BEAM_INVISIBLE_HYDRO 5

#define NODE_NORMAL 0
#define NODE_LOADED 1

#define MAX_NETFORCE 16

//leading truck
#define ACTIVATED 0
//not leading but active
#define DESACTIVATED 1
//active but wanting to sleep
#define MAYSLEEP 2
//active but ordered to sleep ASAP (synchronously)
#define GOSLEEP 3
//static
#define SLEEPING 4
//network
#define NETWORKED 5
#define RECYCLE 6
#define DELETED 7 // special used when truck pointer is 0

#define MAX_WINGS 40

#define MAX_CPARTICLES 10

#define UNLOCKED 0
#define PRELOCK 1
#define LOCKED 2

#define NOT_DRIVEABLE 0
#define TRUCK 1
#define AIRPLANE 2
#define BOAT 3
#define MACHINE 4

#define DRY 0
#define DRIPPING 1
#define WET 2

#define NODE_FRICTION_COEF_DEFAULT   1.0f
#define NODE_VOLUME_COEF_DEFAULT     1.0f
#define NODE_SURFACE_COEF_DEFAULT    1.0f
#define NODE_LOADWEIGHT_DEFAULT     -1.0f

#define HYDRO_FLAG_SPEED        0x00000001
#define HYDRO_FLAG_DIR          0x00000002
#define HYDRO_FLAG_AILERON      0x00000004
#define HYDRO_FLAG_RUDDER       0x00000008
#define HYDRO_FLAG_ELEVATOR     0x00000010
#define HYDRO_FLAG_REV_AILERON  0x00000020
#define HYDRO_FLAG_REV_RUDDER   0x00000040
#define HYDRO_FLAG_REV_ELEVATOR 0x00000080

#define ANIM_FLAG_AIRSPEED      0x00000001
#define ANIM_FLAG_VVI           0x00000002
#define ANIM_FLAG_ALTIMETER     0x00000004
#define ANIM_FLAG_AOA           0x00000008
#define ANIM_FLAG_FLAP          0x00000010
#define ANIM_FLAG_AIRBRAKE      0x00000020
#define ANIM_FLAG_ROLL          0x00000040
#define ANIM_FLAG_PITCH         0x00000080
#define ANIM_FLAG_THROTTLE      0x00000100
#define ANIM_FLAG_RPM           0x00000200
#define ANIM_FLAG_ACCEL         0x00000400
#define ANIM_FLAG_BRAKE         0x00000800
#define ANIM_FLAG_CLUTCH        0x00001000
#define ANIM_FLAG_TACHO         0x00002000
#define ANIM_FLAG_SPEEDO        0x00004000
#define ANIM_FLAG_PBRAKE        0x00008000
#define ANIM_FLAG_TURBO         0x00010000
#define ANIM_FLAG_SHIFTER       0x00020000
#define ANIM_FLAG_AETORQUE      0x00040000
#define ANIM_FLAG_AEPITCH       0x00080000
#define ANIM_FLAG_AESTATUS      0x00100000
#define ANIM_FLAG_TORQUE        0x00200000
#define ANIM_FLAG_HEADING       0x00400000
#define ANIM_FLAG_DIFFLOCK      0x00800000
#define ANIM_FLAG_STEERING      0x01000000
#define ANIM_FLAG_EVENT         0x02000000
#define ANIM_FLAG_AILERONS      0x04000000
#define ANIM_FLAG_ARUDDER       0x08000000
#define ANIM_FLAG_BRUDDER       0x10000000
#define ANIM_FLAG_BTHROTTLE     0x20000000
#define ANIM_FLAG_PERMANENT     0x40000000
#define ANIM_FLAG_ELEVATORS     0x80000000

#define ANIM_MODE_ROTA_X		0x00000001
#define ANIM_MODE_ROTA_Y		0x00000002
#define ANIM_MODE_ROTA_Z		0x00000004
#define ANIM_MODE_OFFSET_X		0x00000008
#define ANIM_MODE_OFFSET_Y		0x00000010
#define ANIM_MODE_OFFSET_Z		0x00000020
#define ANIM_MODE_AUTOANIMATE	0x00000040
#define ANIM_MODE_NOFLIP		0x00000080
#define ANIM_MODE_BOUNCE		0x00000100

#define SHOCK_FLAG_NORMAL       0x00000001
#define SHOCK_FLAG_INVISIBLE    0x00000002
#define SHOCK_FLAG_LACTIVE      0x00000004
#define SHOCK_FLAG_RACTIVE      0x00000008
#define SHOCK_FLAG_ISSHOCK2     0x00000010
#define SHOCK_FLAG_SOFTBUMP     0x00000020

#define SHOCK1 1
#define SHOCK2 2
#define SUPPORTBEAM 3
#define ROPE 4

enum blinktype {BLINK_NONE, BLINK_LEFT, BLINK_RIGHT, BLINK_WARN};

using namespace Ogre;
struct differential_data_t;

//RaySceneQuery* nodeSceneQuery = 0;
class Beam;

// RoR's performance is very sensitive to the ordering of the parameters in this
// structure (due to cache reasons). You can easily destroy RoR's performance if you put
// something in the wrong place. Unless you know what you are doing (do you come armed
// with a cache usage tracker?), add what you wish to the bottom of the structure.
typedef struct _node
{
	Vector3 RelPosition; //relative to the local physics origin (one origin per truck) (shaky)
	Vector3 AbsPosition; //absolute position in the world (shaky)
	Vector3 Velocity;
	Vector3 Forces;
	Real inverted_mass;
	Real mass;
	Vector3 lastNormal;
	int locked;
	int iswheel; //0=no, 1, 2=wheel1  3,4=wheel2, etc...
	int wheelid;
	int masstype;
	int wetstate;
	int contactless;
	int lockednode;
	Vector3 lockedPosition; //absolute
	Vector3 lockedForces;
	Vector3 lockedVelocity;
	int contacted;
	Real friction_coef;
	Real buoyancy;
	Real volume_coef;
	Real surface_coef;
	Vector3 lastdrag;
	Vector3 gravimass;
	float wettime;
	bool isHot;
	bool overrideMass;
	bool disable_particles;
	Vector3 buoyanceForce;
	int id;
	float colltesttimer;
	Vector3 iPosition; // initial position, absolute
	Real    iDistance; // initial distance from node0 during loading - used to check for loose parts
	Vector3 smoothpos; //absolute, per-frame smooth, must be used for visual effects only
	bool iIsSkin;
	bool isSkin;
	bool contacter;
	int pos;
} node_t;

typedef struct
{
	int beamid;
	int flags;
	float lastpos;
	float springin;
	float dampin;
	float sprogin;
	float dprogin;
	float springout;
	float dampout;
	float sprogout;
	float dprogout;

}shock_t;

typedef struct
{
	int state;
	int local_node;
	int remote_truck;
	int remote_node;
	Vector3 toSendForce;
	float last_dist;
	float last_time;
} netlock_t;

typedef struct
{
	bool used;
	float birthdate;
	int node;
	Vector3 force;
} netforcelist_t;

typedef struct
{
	int rate;
	int distance;
} collcab_rate_t;

class BeamThreadStats;
class FlexMesh;
class FlexObj;
class FlexAirfoil;
class Screwprop;
class Buoyance;
class Collisions;
class DustPool;
class BeamEngine;
class HeightFinder;
class Water;
class Mirrors;
class Turboprop;
class Replay;
class Airfoil;
class Network;
class SlideNode;
class Rail;
class RailGroup;
class PointColDetector;

#ifdef LUASCRIPT
class LuaSystem;
#endif

class Skidmark;
class Autopilot;
class MaterialFunctionMapper;
class CmdKeyInertia;


// RoR's performance is very sensitive to the ordering of the parameters in this
// structure (due to cache reasons). You can easily destroy RoR's performance if you put
// something in the wrong place. Unless you know what you are doing (do you come armed
// with a cache usage tracker?), add what you wish to the bottom of the structure.
typedef struct _beam
{
	node_t *p1;
	node_t *p2;
	Beam *p2truck; //in case p2 is on another truck
	bool disabled;
	Real k; //tensile spring
	Real d; //damping factor
	Real L; //length
	Real minmaxposnegstress;
	int type;
	Real maxposstress;
	Real maxnegstress;
	Real shortbound;
	Real longbound;
	Real strength;
	Real stress;
	int bounded;
	bool broken;
	Real plastic_coef;
	SceneNode *mSceneNode; //visual
	Entity *mEntity; //visual
	Real refL; //reference length
	Real Lhydro;//hydro reference len
	Real hydroRatio;//hydro rotation ratio
	int hydroFlags;
	int animFlags;
	float animOption;
	Real commandRatioLong;
	Real commandRatioShort;
	Real commandShort;
	Real commandLong;
	Real maxtiestress;
	Real diameter;
	Vector3 lastforce;
	bool iscentering;
	int isOnePressMode;
	bool isforcerestricted;
	float iStrength; //initial strength
	Real default_deform;
	Real default_plastic_coef;
	int autoMovingMode;
	bool autoMoveLock;
	bool pressedCenterMode;
	float centerLength;
	float minendmass;
	float scale;
	shock_t *shock;
} beam_t;

typedef struct
{
	node_t *p1;
	node_t *p2;
	Real L; //length
} transient_beam_t;

class SoundScriptInstance;

typedef struct
{
	SoundScriptInstance* ssi;
	int nodenum;
} soundsource_t;

typedef struct
{
	int nodeid;
	int contacted;
	int opticontact;
} contacter_t;

typedef struct
{
	node_t* a;
	node_t* b;
	node_t* c;
	float k;
	float d;
	float alpha;
	float lastalpha;
	beam_t *beama;
	beam_t *beamc;
} rigidifier_t;

typedef struct _wheel
{
	int nbnodes;
	node_t* nodes[50];
	/**
	 * Defines the braking characteristics of a wheel. Wheels are braked by three mechanisms:
	 * A footbrake, a handbrake/parkingbrake, and directional brakes used for skidsteer steering.
	 * - 0 = no  footbrake, no  handbrake, no  direction control -- wheel is unbraked
	 * - 1 = yes footbrake, yes handbrake, no  direction control
	 * - 2 = yes footbrake, yes handbrake, yes direction control (braked when truck steers to the left)
	 * - 3 = yes footbrake, yes handbrake, yes direction control (braked when truck steers to the right)
	 * - 4 = yes footbrake, no  handbrake, no  direction control -- wheel has footbrake only, such as with the front wheels of a normal car
	 **/
	int braked;
	node_t* arm;
	node_t* near_attach;
	node_t* refnode0;
	node_t* refnode1;
	int propulsed;
	Real radius;
	Real speed;
	Real delta_rotation; //! difference in wheel position
	float rp;
	float rp1;
	float rp2;
	float rp3;
	float width;

	// for skidmarks
	Vector3 lastContactInner;
	Vector3 lastContactOuter;
	float lastSlip;
	int lastContactType;
	ground_model_t *lastGroundModel;
} wheel_t;

typedef struct _vwheel
{
	node_t *p1;
	node_t *p2;
	Flexable *fm;
	SceneNode *cnode;
	bool meshwheel;
} vwheel_t;

typedef struct hook_t
{
	int locked;
	int group;
	bool lockNodes;
	node_t *hookNode;
	node_t *lockNode;
	Beam *lockTruck;
} hook_t;

typedef struct ropable_t
{
	node_t *node;
	int group;
	bool multilock;
	int used;
} ropable_t;

typedef struct rope_t
{
	int locked;
	int group;
	beam_t *beam;
	node_t *lockedto;
	ropable_t *lockedto_ropable;
	Beam *lockedtruck;
} rope_t;


typedef struct tie_t
{
	beam_t *beam;
	ropable_t *lockedto;
	int group;
	bool tied;
	bool tying;
	float commandValue;
} tie_t;

typedef struct
{
	/*	int nfld;
	int nfrd;
	int nflu;
	int nfru;
	int nbld;
	int nbrd;
	int nblu;
	int nbru;
	*/
	FlexAirfoil *fa;
	SceneNode *cnode;
} wing_t;

typedef struct _command_tmp
{
	float commandValue;
	std::vector<int> beams;
	std::vector<int> rotators;
	Ogre::String description;
} command_t;

typedef struct
{
	int nodes1[4];
	int nodes2[4];
	int axis1; //rot axis
	int axis2;
	float angle;
	float rate;
} rotator_t;


typedef struct _flare
{
	int noderef;
	int nodex;
	int nodey;
	float offsetx;
	float offsety;
	float offsetz;
	SceneNode *snode;
	BillboardSet *bbs;
	Light *light;
	char type;
	int controlnumber;
	bool controltoggle_status;
	float blinkdelay;
	float blinkdelay_curr;
	bool blinkdelay_state;
	float size;
	bool isVisible;
} flare_t;

typedef struct _prop
{
	int noderef;
	int nodex;
	int nodey;
	float offsetx;
	float offsety;
	float offsetz;
	float rotaX;
	float rotaY;
	float rotaZ;
	float orgoffsetX;
	float orgoffsetY;
	float orgoffsetZ;
	Quaternion rot;
	SceneNode *snode;
	SceneNode *wheel;
	Vector3 wheelpos;
	int mirror;
	char beacontype;
	BillboardSet *bbs[4];
	SceneNode *bbsnode[4];
	Light *light[4];
	float brate[4];
	float bpos[4];
	int pale;
	int spinner;
	bool animated;
	float anim_x_Rot;
	float anim_y_Rot;
	float anim_z_Rot;
	float anim_x_Off;
	float anim_y_Off;
	float anim_z_Off;
	float animratio[10];
	int animFlags[10];
	int animMode[10];
	float animOpt1[10];
	float animOpt2[10];
	float animOpt3[10];
	float animOpt4[10];
	float animOpt5[10];
	int animKey[10];
	int animKeyState[10];
	int lastanimKS[10];
	Ogre::Real wheelrotdegree;
} prop_t;

typedef struct _exhaust
{
	int emitterNode;
	int directionNode;
	char material[255];
	float factor;
	bool isOldFormat;
	SceneNode *smokeNode;
	ParticleSystem* smoker;
} exhaust_t;

typedef struct _cparticle
{
	int emitterNode;
	int directionNode;
	bool active;
	SceneNode *snode;
	ParticleSystem* psys;
} cparticle_t;

typedef struct _debugtext
{
	int id;
	MovableText *txt;
	SceneNode *node;
} debugtext_t;

void *threadstart(void* vid);


// included here so that all the structs have been declared, this would not be a
// problem if the beam.h cleanup patch was applied
#include "SlideNode.h"
#include "PointColDetector.h"

static const float flapangles[6]={0.0, -0.07, -0.17, -0.33, -0.67, -1.0};

inline Ogre::Vector3 fast_normalise(Ogre::Vector3 v)
{
	return v*fast_invSqrt(v.squaredLength());
}

inline float fast_length(Ogre::Vector3 v)
{
	return fast_sqrt(v.squaredLength());
}

class Beam : public Streamable, public MemoryAllocatedObject
{
public:
	Beam() {}; // for wrapper, DO NOT USE!

	// destructor
	~Beam();

#ifdef ANGELSCRIPT
	// we have to add this to be able to use the class as reference inside scripts
	void addRef(){};
	void release(){};
#endif

	//constructor
	Beam(int tnum, SceneManager *manager, SceneNode *parent, RenderWindow* win, Network *net, float *mapsizex, float *mapsizez, Real px, Real py, Real pz, Quaternion rot, const char* fname, Collisions *icollisions, HeightFinder *mfinder, Water *w, Camera *pcam, Mirrors *mmirror, bool networked=false, bool networking=false, collision_box_t *spawnbox=NULL, bool ismachine=false, int flareMode=0, std::vector<Ogre::String> *truckconfig=0, SkinPtr skin=SkinPtr(), bool freeposition=false);
	void activate();
	void desactivate();
	void pushNetwork(char* data, int size);
	void pushNetForce(int node_id, Vector3 force);
	void expireNetForce();
	void calcNetwork();
	void addPressure(float v);
	float getPressure();
	void calc_masses2(Real total, bool reCalc=false);
	//to load a truck file
	int loadTruck(const char* fname, SceneManager *manager, SceneNode *parent, Real px, Real py, Real pz, Quaternion rot, collision_box_t *spawnbox);
	void setupDefaultSoundSources();
	void addSoundSource(SoundScriptInstance *ssi, int nodenum);
	void calcBox();
	void calcNodeConnectivityGraph();
	void updateContacterNodes();
	void moveOrigin(Vector3 offset); //move physics origin
	void changeOrigin(Vector3 newOrigin); //change physics origin
	Vector3 getPosition();
	float warea(Vector3 ref, Vector3 x, Vector3 y, Vector3 aref);
	void wash_calculator(Quaternion rot);
	void resetAngle(float rot);
	void resetPosition(float px, float pz, bool setInitPosition, float miny=-9999.0);
	void resetPosition(Ogre::Vector3 translation, bool setInitPosition);
	void mouseMove(int node, Vector3 pos, float force);
	void addCamera(int nodepos, int nodedir, int noderoll);
	void addWheel(SceneManager *manager, SceneNode *parent, Real radius, Real width, int rays, int node1, int node2, int snode, int braked, int propulsed, int torquenode, float mass, float wspring, float wdamp, char* texf, char* texb, bool meshwheel=false, float rimradius=0.0, bool rimreverse=false);
	void addWheel2(SceneManager *manager, SceneNode *parent, Real radius, Real radius2, Real width, int rays, int node1, int node2, int snode, int braked, int propulsed, int torquenode, float mass, float wspring, float wdamp, float wspring2, float wdamp2, char* texf, char* texb);
	void init_node(int pos, Real x, Real y, Real z, int type=NODE_NORMAL, Real m=10.0, int iswheel=0, Real friction=CHASSIS_FRICTION_COEF, int id=-1, int wheelid=-1, Real nfriction=NODE_FRICTION_COEF_DEFAULT, Real nvolume=NODE_VOLUME_COEF_DEFAULT, Real nsurface=NODE_SURFACE_COEF_DEFAULT, Real nloadweight=NODE_LOADWEIGHT_DEFAULT);
	int add_beam(node_t *p1, node_t *p2, SceneManager *manager, SceneNode *parent, int type, Real strength, Real spring, Real damp, Real length=-1.0, float shortbound=-1.0, float longbound=-1.0, float precomp=1.0, float diameter=DEFAULT_BEAM_DIAMETER);
	void reset(bool keepPosition = false); //call this one to reset a truck from any context
	void SyncReset(); //this one should be called only synchronously (without physics running in background)
	//this is called by the threads
	void threadentry(int id);
	void updateSkidmarks();
	ground_model_t *getLastFuzzyGroundModel() { return lastFuzzyGroundModel; };


	String debugText;
	int truckSteps;

	//integration loop
	//bool frameStarted(const FrameEvent& evt)
	//this will be called once by frame and is responsible for animation of all the trucks!
	//the instance called is the one of the current ACTIVATED truck
	bool frameStep(Real dt, Beam** trucks, int numtrucks);
	void prepareShutdown();
	void calcForcesEuler(int doUpdate, Real dt, int step, int maxsteps, Beam** trucks, int numtrucks);
	void truckTruckCollisions(Real dt, Beam** trucks, int numtrucks);
	void calcShocks2(int beam_i, Real difftoBeamL, Real &k, Real &d);
	void calcAnimators(int flagstate, float &cstate, int &div, float timer, float opt1, float opt2, float opt3);
	Quaternion specialGetRotationTo(const Vector3& src, const Vector3& dest) const;
	void prepareInside(bool inside);
	void lightsToggle(Beam** trucks, int trucksnum);
	void updateFlares(float dt, bool isCurrent=false);
	void updateProps();
	void updateVisual(float dt=0);
	void updateSoundSources();
	//v=0: full detail
	//v=1: no beams
	void setDetailLevel(int v);
	void showSkeleton(bool meshes=true, bool newMode=false);
	void hideSkeleton(bool newMode=false);

	void tieToggle(Beam** trucks, int trucksnum, int group=-1);
	void ropeToggle(Beam** trucks, int trucksnum, int group=-1);
	void hookToggle(Beam** trucks, int trucksnum, int group=-1);

	void toggleSlideNodeLock( Beam** trucks, int trucksnum, unsigned int curTruck );

	void parkingbrakeToggle();
	void beaconsToggle();
	void setReplayMode(bool rm);
	int savePosition(int position);
	int loadPosition(int position);
	void updateNetworkInfo();
	void resetAutopilot();
	void disconnectAutopilot();
	void toggleCustomParticles();
	void toggleAxleLock();	//! diff lock on or off
	Ogre::String getAxleLockName();	//! get the name of the current differential model
	int getAxleLockCount();

	//total torque
	//    Real torque;
	//total braking action
	Real brake;
//	Vector3 aposition;
	Vector3 affforce;
	Vector3 ffforce;
	Real affhydro;
	Real ffhydro;
	bool patchEngineTorque;

	void scaleTruck(float value);
	float currentScale;

	node_t nodes[MAX_NODES];
	beam_t beams[MAX_BEAMS];
	std::vector< std::vector< int > > nodetonodeconnections;
	std::vector< std::vector< int > > nodebeamconnections;

	std::vector<debugtext_t>nodes_debug, beams_debug;
	void updateDebugOverlay();
	int nodedebugstate;
	int debugVisuals;

	SceneManager *tsm;

	BeamEngine *engine;
	bool freePositioned;

	bool networking;
	int label;
	int trucknum;
	SkinPtr usedSkin;

	int cinecameranodepos[MAX_CAMERAS];
	int cameranodepos[MAX_CAMERAS];
	int cameranodedir[MAX_CAMERAS];
	int cameranoderoll[MAX_CAMERAS];
	bool revroll[MAX_CAMERAS];
	float WheelSpeed;
	int stabcommand;
	command_t commandkey[MAX_COMMANDS + 1];
	int skeleton;
	float stabratio;
	int free_shock;
	int free_active_shock; // this has no array associated with it. its just to determine if there are active shocks!
	//direction
	float hydrodircommand;
	bool hydroSpeedCoupling;
	float hydrodirstate;
	Real hydrodirwheeldisplay;
	//extra airplane axises
	float hydroaileroncommand;
	float hydroaileronstate;
	float hydroruddercommand;
	float hydrorudderstate;
	float hydroelevatorcommand;
	float hydroelevatorstate;

	bool replaymode;
	int replaylen;
	int replaypos;
	int oldreplaypos;
	int watercontact;
	int watercontactold;
	bool cparticle_enabled;
	int free_node;
	int dynamicMapMode;
	int canwork;
	int hashelp;
	char helpmat[256];
	float minx, maxx, miny, maxy, minz, maxz;
	int state;
	int sleepcount;
	//can this be drived?
	int driveable;
	int previousGear;
	float previousCrank;
	float animTimer;
	int importcommands;
	bool requires_wheel_contact;
	bool wheel_contact_requested;
	bool rescuer;
	int parkingbrake;
	int lights;
	int smokeId;
	int editorId;
	float leftMirrorAngle;
	float rightMirrorAngle;
	float *mapsizex, *mapsizez;
	float refpressure;
	int pressure_beams[MAX_PRESSURE_BEAMS];
	int free_pressure_beam;
	PointColDetector *pointCD;

	pthread_mutex_t work_mutex;
	pthread_cond_t work_cv;

	pthread_mutex_t done_count_mutex;
	pthread_cond_t done_count_cv;
	int done_count;
	prop_t props[MAX_PROPS];
	prop_t *driverSeat;
	int calculateDriverPos(Vector3 &pos, Quaternion &rot);
	float getSteeringAngle() { return hydrodircommand; };
	int free_prop;
	float default_beam_diameter;
	float default_plastic_coef;
	float skeleton_beam_diameter;
	int free_aeroengine;
	AeroEngine *aeroengines[MAX_AEROENGINES];

	int free_screwprop;
	Screwprop *screwprops[MAX_SCREWPROPS];

	float elevator;
	float rudder;
	float aileron;
	int flap;
	int free_wing;
	wing_t wings[MAX_WINGS];
	Vector3 fusedrag;
	float fadeDist;
	bool disableDrag;
	int currentcamera;
	int freecinecamera;
	float brakeforce;
	float hbrakeforce;
	bool ispolice;
	float beam_creak;
	int loading_finished;
	int freecamera;
	int first_wheel_node;
	int netbuffersize;
	int nodebuffersize;
	SceneNode *netLabelNode;

	std::string getTruckName(){return std::string(realtruckname);};
	std::vector<AuthorInfo> getAuthors(){return authors;};
	std::vector<std::string> getDescription() {return description;};

	int getBeamCount(){return free_beam;};
	beam_t *getBeams(){return beams;};
	float getDefaultDeformation(){ return default_deform;}

	int getNodeCount(){return free_node;};
	node_t *getNodes(){return nodes;};
	int nodeBeamConnections(int nodeid);

	float getTotalMass(bool withLocked=true);
	void recalc_masses();
	int getWheelNodeCount();
	void setMass(float m) { truckmass = m; };

	beam_t *addBeam(int id1, int id2);
	node_t *addNode(Ogre::Vector3 pos);
	String speedomat, tachomat;
	float speedoMax;
	bool useMaxRPMforGUI;

	Ogre::String realtruckfilename;

	wheel_t wheels[MAX_WHEELS];
	Axle axles[MAX_WHEELS/2];
	int free_wheel;
	int free_axle;
	bool beambreakdebug;


	// this must be in the header as the network stuff is using it...
	inline bool getBrakeLightVisible()
	{
		if(state==NETWORKED)
			return netBrakeLight;

//		return (brake > 0.15 && !parkingbrake);
		return (brake > 0.15);
	}

	bool getReverseLightVisible();

	inline bool getCustomLightVisible(int number)
	{
		if(netCustomLightArray[number] != -1)
			return flares[netCustomLightArray[number]].controltoggle_status;
		else
			return false;
	}

	inline void setCustomLightVisible(int number, bool visible)
	{
		if(netCustomLightArray[number] == -1)
			return;
		flares[netCustomLightArray[number]].controltoggle_status = visible;
	}

	bool getBeaconMode() { return beacon; };
	void setBlinkType(blinktype blink);
	blinktype getBlinkType() { return blinkingtype; };
	void deleteNetTruck();
	netlock_t netlock;
	Autopilot *autopilot;
	float getHeadingDirectionAngle();
	bool getCustomParticleMode() { return cparticle_mode; };
	int getLowestNode() { return lowestnode; };
	void preMapLabelRenderUpdate(bool mode, float cheight=0);
	SceneNode *cablightNode;
	float tdt;
	float ttdt;
	int airbrakeval;
	Vector3 cameranodeacc;
	int cameranodecount;
	bool abs_state;
	float abs_timer;

	Vector3 origin;
	int free_cab;
	int free_contacter;
	contacter_t contacters[MAX_CONTACTERS];

	int cabs[MAX_CABS*3];
	int subisback[MAX_SUBMESHES];
	void setMeshVisibility(bool visible);
	bool meshesVisible;
	inline bool inRange(float num, float min, float max);

	int getTruckTime() { return nettimer->getMilliseconds(); };
	int getNetTruckTimeOffset() { return net_toffset; };
	Real getMinimalCameraRadius() { return minCameraRadius; };


	Replay *getReplay() { return replay; };

	bool getSlideNodesLockInstant() { 	return slideNodesConnectInstantly; };
	void sendStreamData();
	bool isTied();
	bool isLocked();
	int tsteps;

protected:

	void updateSimpleSkeleton();
	SceneNode *simpleSkeletonNode;
	std::vector<Ogre::String> truckconfig;

	std::vector<Ogre::SceneNode*> deletion_sceneNodes;
	std::vector<Ogre::MovableObject *> deletion_Objects;

	Vector3 position;
	Vector3 lastposition;
	Vector3 lastlastposition;
	Real minCameraRadius;

	MaterialFunctionMapper *materialFunctionMapper;
	Real replayTimer;
	Real replayPrecision;

	Network *net;
	ground_model_t *lastFuzzyGroundModel;

	// this is for managing the blinkers on the truck:
	blinktype blinkingtype;
	Light *cablight;

	bool enable_wheel2;
	bool deleting;

	char default_node_options[50];
	char truckname[256];
	char realtruckname[256];
	std::vector<AuthorInfo> authors;

	int truckversion;
	char uniquetruckid[255];
	int categoryid;
	std::vector < exhaust_t > exhausts;

	std::vector<std::string> description;
	int hascommands;
	int forwardcommands;
	RenderWindow* mWindow;
	int free_beam;
	rigidifier_t rigidifiers[MAX_RIGIDIFIERS];
	int free_rigidifier;
	int hydro[MAX_HYDROS];
	int free_hydro;
	vwheel_t vwheels[MAX_WHEELS];
	Real hydrolen;
	Real truckmass;
	Real loadmass;
	int masscount;
	//number of torque points
	//    int torquenum;
	Real lastwspeed;
	int free_texcoord;
	int free_collcab;
	int free_buoycab;
	int free_sub;
	float collrange;
	int subtexcoords[MAX_SUBMESHES];
	int subcabs[MAX_SUBMESHES];
	Vector3 texcoords[MAX_TEXCOORDS];
	int collcabs[MAX_CABS];
	int collcabstype[MAX_CABS];
	collcab_rate_t collcabrate[MAX_CABS];

	int buoycabs[MAX_CABS];
	int buoycabtypes[MAX_CABS];
	FlexObj *cabMesh;
	SceneNode *cabNode;
	bool disable_smoke;
	bool heathaze;
	SceneNode *smokeNode;
	int smokeRef;
	ParticleSystem* smoker;
	shock_t shocks[MAX_SHOCKS];
	float stabsleep;
	int proped_wheels;
	int braked_wheels;
	//for inter-diffential locking
	int proppairs[MAX_WHEELS];
	//	float lastdt;
	Collisions *collisions;
	HeightFinder *hfinder;
	int fasted;
	int slowed;
	Replay *replay;
	PositionStorage *posStorage;

	std::vector <rope_t> ropes;
	std::vector <ropable_t> ropables;
	std::vector <tie_t> ties;
	std::vector <hook_t> hooks;


	cparticle_t cparticles[MAX_CPARTICLES];
	int free_cparticle;
	bool cparticle_mode;
	Beam** ttrucks;
	int tnumtrucks;
	SceneNode *parentNode;
	SceneNode *beamsRoot;
	int detailLevel;
	Water *water;
	std::vector<flare_t> flares;
	int free_flare;
	Camera *mCamera;
	char texname[1024];
	int hasEmissivePass;
	bool isInside;
	Mirrors *mirror;
	bool beacon;
	bool driversseatfound;
	float totalmass;
	float default_node_friction;
	float default_node_volume;
	float default_node_surface;
	float default_node_loadweight;
	float default_break;
	float default_deform;
	float default_spring;
	float default_damp;
	bool advanced_drag;
	float advanced_node_drag;
	float advanced_total_drag;
	char default_beam_material[256];
	Airfoil *fuseAirfoil;
	node_t *fuseFront;
	node_t *fuseBack;
	float fuseWidth;
	bool hasposlights;
	int mousenode;
	Vector3 mousepos;
	float mousemoveforce;
	int reset_requested;

	int free_airbrake;
	Airbrake *airbrakes[MAX_AIRBRAKES];

	rotator_t rotators[MAX_ROTATORS];
	int free_rotator;
	Buoyance *buoyance;
	float ipy;

	SWInetSocket *sock;
	pthread_t netthread;
	oob_t *oob1;
	oob_t *oob2;
	oob_t *oob3;
	char *netb1;
	char *netb2;
	char *netb3;
	pthread_mutex_t net_mutex;
	Timer *nettimer;
	int net_toffset;
	int netcounter;
	MovableText *netMT; //, *netDist;
	float minimass;

	// network properties
	String networkUsername;
	int networkAuthlevel;

	// skidmark stuff
	Skidmark *skidtrails[MAX_WHEELS*2];
	bool useSkidmarks;

	FlexBody *flexbodies[MAX_FLEXBODIES];
	int free_flexbody;

	int netCustomLightArray[4];
	unsigned char netCustomLightArray_counter;
	bool netBrakeLight, netReverseLight;
	Real mTimeUntilNextToggle;


	void checkBeamMaterial();
	int flaresMode;

	//TruckCommandScheduler *truckScript;

	pthread_t threads[4];

	// cab fading stuff - begin
	void cabFade(float amount);
	void setMeshWireframe(SceneNode *node, bool value);
	void fadeMesh(SceneNode *node, float amount);
	float getAlphaRejection(SceneNode *node);
	void setAlphaRejection(SceneNode *node, float amount);
	float cabFadeTimer;
	float cabFadeTime;
	int cabFadeMode;
	// cab fading stuff - end
	bool lockSkeletonchange;
	int lowestnode;
	bool floating_origin_enable;

	netforcelist_t netforces[MAX_NETFORCE];

	Ogre::ManualObject *simpleSkeletonManualObject;
	bool simpleSkeletonInitiated;
	void initSimpleSkeleton();

	SoundScriptManager *ssm;
	soundsource_t soundsources[MAX_SOUNDSCRIPTS_PER_TRUCK];
	int free_soundsource;

	bool disable_default_sounds;
	/**
	 * Resets the turn signal when the steering wheel is turned back.
	 */
	void autoBlinkReset();
	bool blinktreshpassed;

	CmdKeyInertia *rotaInertia;
	CmdKeyInertia *hydroInertia;
	CmdKeyInertia *cmdInertia;
#ifdef TIMING
	BeamThreadStats *statistics;
#endif


	// overloaded from Streamable:
	Timer netTimer;
	int last_net_time;
	void sendStreamSetup();
	void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);


	// SLIDE NODES /////////////////////////////////////////////////////////////
	//! Stores all the SlideNodes available on this truck
	std::vector< SlideNode > mSlideNodes;
	//! true if SlideNodes are locked, false if not
	bool SlideNodesLocked;

	//! try to connect slidenodes direclty after spawning
	bool slideNodesConnectInstantly;

	/**
	 * @param line line in configuration file
	 * @return true if line was successfully parsed, false if not
	 */
	bool parseSlideNodeLine(const Ogre::String& line);

	/**
	 * calculate and apply Corrective forces
	 * @param dt delta time in seconds
	 */
	void updateSlideNodeForces(const Ogre::Real dt);
	//! Recalculate SlideNode positions
	void resetSlideNodePositions();
	//! Reset all the SlideNodes
	void resetSlideNodes();
	//! incrementally update the position of all SlideNodes
	void updateSlideNodePositions();

	/**
	 *
	 * @param truck which truck to retrieve the closest Rail from
	 * @param node which SlideNode is being checked against
	 * @return a pair containing the rail, and the distant to the SlideNode
	 */
	std::pair<RailGroup*, Ogre::Real> getClosestRailOnTruck( Beam* truck, const SlideNode& node);


	// RAIL GROUPS /////////////////////////////////////////////////////////////
	//! Stores all the available RailGroups for this truck
	std::vector< RailGroup* > mRailGroups;

	/**
	 * @param line line in configuration file
	 * @return true if line was successfully parsed, false if not
	 */
	bool parseRailGroupLine(const Ogre::String& line);

	// utility methods /////////////////////////////////////////////////////////

	/**
	 * searches the RailGRoup array for a rail with the corresponding id value
	 * @param id of rail group to search for
	 * @return NULL if no rail group is found, otherwise the corresponding rail group.
	 */
	RailGroup* getRailFromID(unsigned int id);

	/**
	 * wrapper for getRails, converts the list of strings to a compatible format
	 * @param railStrings list of node id's in string format
	 * @return same as getRails
	 */
	Rail* parseRailString( const std::vector<Ogre::String>& railStrings);

	/**
	 * parses an array of nodes id's to generate a Rail
	 * @param nodeids a list of node id's
	 * @return NULL if nodes do not form a continuous beam structure
	 */
	Rail* getRails(const std::vector<int>& nodeids);

	/**
	 * Finds the beam instance based on the node IDs
	 * @param node1ID node id of one node
	 * @param node2ID node id of the other node
	 * @return pointer to beam instance, NULL if no beam is found
	 */
	beam_t* getBeam(unsigned int node1ID, unsigned int node2ID);

	/**
	 * Finds the beam based on actual node instances
	 * @param node1 first node of the beam
	 * @param node2 second node of the beam
	 * @return pointer to beam instance, NULL if no beam is found
	 */
	beam_t* getBeam(node_t* node1, node_t* node2);

	/**
	 * Find node instance baed on the id
	 * @param id of the node we are looking for
	 * @return pointer to node instance, NULL if no beam is found
	 */
	node_t* getNode(unsigned int id);
};


#endif
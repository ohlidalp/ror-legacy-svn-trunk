/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009,2010 Pierre-Michel Ricordel
Copyright 2007,2008,2009,2010 Thomas Fischer

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

// created on 30th of April 2010 by Thomas Fischer

#ifndef BEAMDATA_H__
#define BEAMDATA_H__

/*

 Rigs of Rods Data Structure (WIP)

 +--------------------------------------------------------+
 | Physics             | Visuals                          |
 +---------------------+----------------------------------+
 | rig_phys_t          | rig_vis_t                        |
 |  node_phys_t        | n/a                              |
 |  beam_phys_t        | n/a                              |
 +---------------------+----------------------------------+

 A word of warning:
 RoR's performance is very sensitive to the ordering of the parameters in this
 structure (due to cache reasons). You can easily destroy RoR's performance if you put
 something in the wrong place. Unless you know what you are doing (do you come armed
 with a cache usage tracker?), add what you wish to the bottom of the structure.
 
 the order of the structs in here is important as well.
*/


// The Ogre required includes
#include "OgrePrerequisites.h"
#include "OgreVector3.h"
// The RoR required includes
#include "RoRPrerequisites.h"

#define THREAD_MONO 0
#define THREAD_HT 1
#define THREAD_HT2 2

#define MAX_TRUCKS 64

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


typedef struct node_t
{
	Ogre::Vector3 RelPosition; //relative to the local physics origin (one origin per truck) (shaky)
	Ogre::Vector3 AbsPosition; //absolute position in the world (shaky)
	Ogre::Vector3 Velocity;
	Ogre::Vector3 Forces;
	Ogre::Real inverted_mass;
	Ogre::Real mass;
	Ogre::Vector3 lastNormal;
	int locked;
	int iswheel; //0=no, 1, 2=wheel1  3,4=wheel2, etc...
	int wheelid;
	int masstype;
	int wetstate;
	int contactless;
	int lockednode;
	Ogre::Vector3 lockedPosition; //absolute
	Ogre::Vector3 lockedForces;
	Ogre::Vector3 lockedVelocity;
	int contacted;
	Ogre::Real friction_coef;
	Ogre::Real buoyancy;
	Ogre::Real volume_coef;
	Ogre::Real surface_coef;
	Ogre::Vector3 lastdrag;
	Ogre::Vector3 gravimass;
	float wettime;
	bool isHot;
	bool overrideMass;
	bool disable_particles;
	Ogre::Vector3 buoyanceForce;
	int id;
	float colltesttimer;
	Ogre::Vector3 iPosition; // initial position, absolute
	Ogre::Real    iDistance; // initial distance from node0 during loading - used to check for loose parts
	Ogre::Vector3 smoothpos; //absolute, per-frame smooth, must be used for visual effects only
	bool iIsSkin;
	bool isSkin;
	bool contacter;
	int pos;
} node_t;


typedef struct shock_t
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

} shock_t;

typedef struct
{
	int rate;
	int distance;
} collcab_rate_t;

typedef struct beam_t
{
	node_t *p1;
	node_t *p2;
	Beam *p2truck; //in case p2 is on another truck
	bool disabled;
	Ogre::Real k; //tensile spring
	Ogre::Real d; //damping factor
	Ogre::Real L; //length
	Ogre::Real minmaxposnegstress;
	int type;
	Ogre::Real maxposstress;
	Ogre::Real maxnegstress;
	Ogre::Real shortbound;
	Ogre::Real longbound;
	Ogre::Real strength;
	Ogre::Real stress;
	int bounded;
	bool broken;
	Ogre::Real plastic_coef;
	Ogre::Real refL; //reference length
	Ogre::Real Lhydro;//hydro reference len
	Ogre::Real hydroRatio;//hydro rotation ratio
	int hydroFlags;
	int animFlags;
	float animOption;
	Ogre::Real commandRatioLong;
	Ogre::Real commandRatioShort;
	Ogre::Real commandShort;
	Ogre::Real commandLong;
	Ogre::Real maxtiestress;
	Ogre::Real diameter;
	Ogre::Vector3 lastforce;
	bool iscentering;
	int isOnePressMode;
	bool isforcerestricted;
	float iStrength; //initial strength
	Ogre::Real default_deform;
	Ogre::Real default_plastic_coef;
	int autoMovingMode;
	bool autoMoveLock;
	bool pressedCenterMode;
	float centerLength;
	float minendmass;
	float scale;
	shock_t *shock;
	SceneNode *mSceneNode; //visual
	Entity *mEntity; //visual
} beam_t;

typedef struct soundsource_t
{
	SoundScriptInstance* ssi;
	int nodenum;
} soundsource_t;

typedef struct contacter_t
{
	int nodeid;
	int contacted;
	int opticontact;
} contacter_t;

typedef struct rigidifier_t
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

typedef struct wheel_t
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

typedef struct vwheel_t
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


typedef struct wing_t
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

typedef struct command_t
{
	float commandValue;
	std::vector<int> beams;
	std::vector<int> rotators;
	Ogre::String description;
} command_t;

typedef struct rotator_t
{
	int nodes1[4];
	int nodes2[4];
	int axis1; //rot axis
	int axis2;
	float angle;
	float rate;
} rotator_t;

typedef struct flare_t
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

typedef struct prop_t
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

typedef struct exhaust_t
{
	int emitterNode;
	int directionNode;
	char material[255];
	float factor;
	bool isOldFormat;
	SceneNode *smokeNode;
	ParticleSystem* smoker;
} exhaust_t;


typedef struct cparticle_t
{
	int emitterNode;
	int directionNode;
	bool active;
	SceneNode *snode;
	ParticleSystem* psys;
} cparticle_t;


typedef struct debugtext_t
{
	int id;
	Ogre::MovableText *txt;
	SceneNode *node;
} debugtext_t;

typedef struct rig_t
{
	node_t nodes[MAX_NODES];
	int free_node;

	beam_t beams[MAX_BEAMS];
	int free_beam;

	contacter_t contacters[MAX_CONTACTERS];
	int free_contacter;

	rigidifier_t rigidifiers[MAX_RIGIDIFIERS];
	int free_rigidifier;

	wheel_t wheels[MAX_WHEELS];
	vwheel_t vwheels[MAX_WHEELS];
	int free_wheel;

	std::vector <rope_t> ropes;
	std::vector <ropable_t> ropables;
	std::vector <tie_t> ties;
	std::vector <hook_t> hooks;

	wing_t wings[MAX_WINGS];
	int free_wing;
		
	command_t commandkey[MAX_COMMANDS + 1];
	
	rotator_t rotators[MAX_ROTATORS];
	int free_rotator;

	std::vector<flare_t> flares;
	int free_flare;

	prop_t props[MAX_PROPS];
	prop_t *driverSeat;
	int free_prop;
	
	std::vector < exhaust_t > exhausts;

	cparticle_t cparticles[MAX_CPARTICLES];
	int free_cparticle;
	
	std::vector<debugtext_t>nodes_debug, beams_debug;
	
	soundsource_t soundsources[MAX_SOUNDSCRIPTS_PER_TRUCK];
	int free_soundsource;
	
} rig_t;

// some non-beam structs

typedef struct _collision_box
{
	//absolute collision box
	float lo_x;
	float hi_x;
	float lo_y;
	float hi_y;
	float lo_z;
	float hi_z;
	bool refined;
	//rotation
	Quaternion rot;
	Quaternion unrot;
	//center of rotation
	Vector3 center;
	//relative collision box
	float relo_x;
	float rehi_x;
	float relo_y;
	float rehi_y;
	float relo_z;
	float rehi_z;
	//self rotation
	bool selfrotated;
	Vector3 selfcenter;
	Quaternion selfrot;
	Quaternion selfunrot;
	int eventsourcenum;
	bool virt;
	bool camforced;
	Vector3 campos;
	int event_filter;
} collision_box_t;

#endif //BEAMDATA_H__

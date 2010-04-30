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
	MovableText *txt;
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


#endif //BEAMDATA_H__

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

#include "OgrePrerequisites.h"
#include "RoRPrerequisites.h"

#include "BeamData.h" // TODO: fix the other headers, so we can remove this line


using namespace Ogre;

#include <pthread.h>
#include <string>

#ifdef SOCKETW
# include "SocketW.h"
#endif //SOCKETW
#include "rornet.h"
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
#include "rormemory.h"
#include <vector>





void *threadstart(void* vid);


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

class Beam : public rig_t, public Streamable, public MemoryAllocatedObject
{
public:
	Beam() {}; // for wrapper, DO NOT USE!

	// destructor
	~Beam();

#ifdef USE_ANGELSCRIPT
	// we have to add this to be able to use the class as reference inside scripts
	void addRef(){};
	void release(){};
#endif

	//constructor
	Beam(int tnum, SceneManager *manager, SceneNode *parent, RenderWindow* win, Network *net, float *mapsizex, float *mapsizez, Real px, Real py, Real pz, Quaternion rot, const char* fname, Collisions *icollisions, HeightFinder *mfinder, Water *w, Camera *pcam, Mirrors *mmirror, bool networked=false, bool networking=false, collision_box_t *spawnbox=NULL, bool ismachine=false, int flareMode=0, std::vector<Ogre::String> *truckconfig=0, SkinPtr skin=SkinPtr(), bool freeposition=false);
	void activate();
	void desactivate();
	void pushNetwork(char* data, int size);
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
	int skeleton;
	float stabratio;
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
	PointColDetector *pointCD;

	pthread_mutex_t work_mutex;
	pthread_cond_t work_cv;

	pthread_mutex_t done_count_mutex;
	pthread_cond_t done_count_cv;
	int done_count;
	int calculateDriverPos(Vector3 &pos, Quaternion &rot);
	float getSteeringAngle() { return hydrodircommand; };
	float default_beam_diameter;
	float default_plastic_coef;
	float skeleton_beam_diameter;

	float elevator;
	float rudder;
	float aileron;
	int flap;

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

	Axle axles[MAX_WHEELS/2];
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

	std::vector<std::string> description;
	int hascommands;
	int forwardcommands;
	RenderWindow* mWindow;
	Real hydrolen;
	Real truckmass;
	Real loadmass;
	int masscount;
	//number of torque points
	//    int torquenum;
	Real lastwspeed;
	float collrange;
	FlexObj *cabMesh;
	SceneNode *cabNode;
	bool disable_smoke;
	bool heathaze;
	SceneNode *smokeNode;
	int smokeRef;
	ParticleSystem* smoker;
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



	bool cparticle_mode;
	Beam** ttrucks;
	int tnumtrucks;
	SceneNode *parentNode;
	SceneNode *beamsRoot;
	int detailLevel;
	Water *water;
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


	Buoyance *buoyance;
	float ipy;

#ifdef SOCEKTW
	SWInetSocket *sock;
#endif //SOCKETW
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

	Ogre::ManualObject *simpleSkeletonManualObject;
	bool simpleSkeletonInitiated;
	void initSimpleSkeleton();

	SoundScriptManager *ssm;
	

	bool disable_default_sounds;
	/**
	 * Resets the turn signal when the steering wheel is turned back.
	 */
	void autoBlinkReset();
	bool blinktreshpassed;

	CmdKeyInertia *rotaInertia;
	CmdKeyInertia *hydroInertia;
	CmdKeyInertia *cmdInertia;
#ifdef FEAT_TIMING
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
	Rail* parseRailString( const Ogre::vector<Ogre::String>::type& railStrings);

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

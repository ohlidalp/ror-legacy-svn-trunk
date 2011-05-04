/*
This source file is part of Rigs of Rods
Copyright 2005-2011 Pierre-Michel Ricordel
Copyright 2007-2011 Thomas Fischer

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
#ifndef BEAM_H__
#define BEAM_H__

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"

// includes to be removed or cleaned up
#include "rornet.h"
#include "Differentials.h"
#include "SlideNode.h"
#include "PointColDetector.h"

// ror includes
#include "Streamable.h"
#include "SerializedRig.h"
#include "BeamData.h"

#include "approxmath.h"
#include "CacheSystem.h"

// system includes
#include <vector>
#include <pthread.h>

class Beam : 
	public SerializedRig, 
	public Streamable
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
	Beam(int tnum
		, SceneManager *manager
		, SceneNode *parent
		, RenderWindow* win
		, Network *net
		, float *mapsizex
		, float *mapsizez
		, Real px
		, Real py
		, Real pz
		, Quaternion rot
		, const char* fname
		, Collisions *icollisions
		, HeightFinder *mfinder
		, Water *w
		, Camera *pcam
		, bool networked=false
		, bool networking=false
		, collision_box_t *spawnbox=NULL
		, bool ismachine=false
		, int flareMode=0
		, std::vector<Ogre::String> *truckconfig=0
		, Skin *skin=0
		, bool freeposition=false);

	//! @{ network related functions
	void pushNetwork(char* data, int size);
	void calcNetwork();
	void updateNetworkInfo();
	//! @}

	//! @{ physic related functions
	void activate();
	void desactivate();
	void addPressure(float v);
	float getPressure();
	void calc_masses2(Real total, bool reCalc=false);
	void calcNodeConnectivityGraph();
	void updateContacterNodes();
	void moveOrigin(Vector3 offset); //move physics origin
	void changeOrigin(Vector3 newOrigin); //change physics origin
	Vector3 getPosition();
	void resetAngle(float rot);
	void resetPosition(float px, float pz, bool setInitPosition, float miny=-9999.0);
	void resetPosition(Ogre::Vector3 translation, bool setInitPosition);
	void reset(bool keepPosition = false); //call this one to reset a truck from any context
	void SyncReset(); //this one should be called only synchronously (without physics running in background)
	//this is called by the threads
	void threadentry(int id);
	//integration loop
	//bool frameStarted(const FrameEvent& evt)
	//this will be called once by frame and is responsible for animation of all the trucks!
	//the instance called is the one of the current ACTIVATED truck
	bool frameStep(Real dt);
	int truckSteps;
	void calcForcesEuler(int doUpdate, Real dt, int step, int maxsteps);
	void truckTruckCollisions(Real dt);
	void calcShocks2(int beam_i, Real difftoBeamL, Real &k, Real &d, Real dt, int update);
	void calcAnimators(int flagstate, float &cstate, int &div, float timer, float opt1, float opt2, float opt3);
	//! @}

	//! @{ audio related functions
	void setupDefaultSoundSources();
	void updateSoundSources();
	//! @}

	//! @{ user interaction functions
	void mouseMove(int node, Vector3 pos, float force);
	void lightsToggle();
	void tieToggle(int group=-1);
	void ropeToggle(int group=-1);
	void hookToggle(int group=-1, int mode = HOOK_TOGGLE);
	void toggleSlideNodeLock();
	void toggleCustomParticles();
	void toggleAxleLock();	//! diff lock on or off
	void parkingbrakeToggle();
	void antilockbrakeToggle();
	void tractioncontrolToggle();
	void beaconsToggle();
	void setReplayMode(bool rm);
	int savePosition(int position);
	int loadPosition(int position);
	void updateTruckPosition();
	//! @}

	//! @{ ground
	ground_model_t *getLastFuzzyGroundModel();
	//! @}

	//! @{ graphical display things */
	void updateSkidmarks();
	void updateAI(float dt);
	String debugText;
	void prepareInside(bool inside);
	void updateFlares(float dt, bool isCurrent=false);
	void updateProps();
	void updateVisual(float dt=0);
	//v=0: full detail
	//v=1: no beams
	void setDetailLevel(int v);
	void showSkeleton(bool meshes=true, bool newMode=false);
	void hideSkeleton(bool newMode=false);
	void resetAutopilot();
	void disconnectAutopilot();
	void scaleTruck(float value);
	float currentScale;
	void updateDebugOverlay();
	int nodedebugstate;
	SceneManager *tsm;
	//! @}

	//! @{ startup / shutdown
	void prepareShutdown();
	//! @}

	//! @{ dynamic physical properties
	Real brake;
	Vector3 affforce;
	Vector3 ffforce;
	Real affhydro;
	Real ffhydro;
	//! @}

	/* functions to be sorted */
	Quaternion specialGetRotationTo(const Vector3& src, const Vector3& dest) const;
	Ogre::String getAxleLockName();	//! get the name of the current differential model
	int getAxleLockCount();
	std::vector< std::vector< int > > nodetonodeconnections;
	std::vector< std::vector< int > > nodebeamconnections;
	int label;

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
	int canwork;
	int sleepcount;
	//can this be drived?
	int previousGear;
	bool requires_wheel_contact;
	int parkingbrake;
	int lights;
	bool reverselight;
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
	float getSteeringAngle();

	float elevator;
	float rudder;
	float aileron;
	int flap;

	Vector3 fusedrag;
	
	bool disableDrag;
	int currentcamera;
	
	int first_wheel_node;
	int netbuffersize;
	int nodebuffersize;
	SceneNode *netLabelNode;

	std::string getTruckName();
	std::vector<authorinfo_t> getAuthors();
	std::vector<std::string> getDescription();

	int getBeamCount();
	beam_t *getBeams();
	float getDefaultDeformation();

	int getNodeCount();
	node_t *getNodes();
	int nodeBeamConnections(int nodeid);


	void changedCamera();

	float getTotalMass(bool withLocked=true);
	void recalc_masses();
	int getWheelNodeCount();
	void setMass(float m);

	beam_t *addBeam(int id1, int id2);
	node_t *addNode(Ogre::Vector3 pos);

	Ogre::String realtruckfilename;



	// this must be in the header as the network stuff is using it...
	bool getBrakeLightVisible();

	bool getReverseLightVisible();

	bool getCustomLightVisible(int number);

	void setCustomLightVisible(int number, bool visible);

	bool getBeaconMode();
	void setBlinkType(blinktype blink);
	blinktype getBlinkType();
	void deleteNetTruck();
	
	
	float getHeadingDirectionAngle();
	bool getCustomParticleMode();
	int getLowestNode();
	void preMapLabelRenderUpdate(bool mode, float cheight=0);
	
	float tdt;
	float ttdt;
	int airbrakeval;
	Vector3 cameranodeacc;
	int cameranodecount;
	bool abs_state;
	float abs_timer;

	void setMeshVisibility(bool visible);
	bool meshesVisible;
	bool inRange(float num, float min, float max);

	int getTruckTime();
	int getNetTruckTimeOffset();
	Real getMinimalCameraRadius();


	Replay *getReplay();

	bool getSlideNodesLockInstant();
	void sendStreamData();
	bool isTied();
	bool isLocked();
	int tsteps;
	float avichatter_timer;

protected:

	void updateSimpleSkeleton();
	SceneNode *simpleSkeletonNode;

	Vector3 position;
	Vector3 lastposition;
	Vector3 lastlastposition;
	Real minCameraRadius;

	Real replayTimer;
	Real replayPrecision;

	Network *net;
	ground_model_t *lastFuzzyGroundModel;

	// this is for managing the blinkers on the truck:
	blinktype blinkingtype;

	bool deleting;

	


	
	RenderWindow* mWindow;
	Real hydrolen;
	
	//number of torque points
	//    int torquenum;
	Real lastwspeed;
	SceneNode *smokeNode;
	ParticleSystem* smoker;
	float stabsleep;
	//	float lastdt;
	Collisions *collisions;
	int fasted;
	int slowed;
	Replay *replay;
	PositionStorage *posStorage;



	bool cparticle_mode;
	Beam** ttrucks;
	int tnumtrucks;
	SceneNode *parentNode;
	int detailLevel;
	bool isInside;
	bool beacon;
	float totalmass;

	int mousenode;
	Vector3 mousepos;
	float mousemoveforce;
	int reset_requested;


	float ipy;

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

	// network properties
	String networkUsername;
	int networkAuthlevel;

	bool netBrakeLight, netReverseLight;
	Real mTimeUntilNextToggle;


	void checkBeamMaterial();

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
	bool floating_origin_enable;

	Ogre::ManualObject *simpleSkeletonManualObject;
	bool simpleSkeletonInitiated;
	void initSimpleSkeleton();

	int loadTruck2(Ogre::String filename, Ogre::SceneManager *manager, Ogre::SceneNode *parent, Ogre::Vector3 pos, Ogre::Quaternion rot, collision_box_t *spawnbox);

	/**
	 * Resets the turn signal when the steering wheel is turned back.
	 */
	void autoBlinkReset();
	bool blinktreshpassed;

#ifdef FEAT_TIMING
	BeamThreadStats *statistics, *statistics_gfx;
#endif


	// overloaded from Streamable:
	Timer netTimer;
	int last_net_time;
	void sendStreamSetup();
	void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);

	// dustpools
	DustPool *dustp;
    DustPool *dripp;
    DustPool *sparksp;
    DustPool *clumpp;
    DustPool *splashp;
    DustPool *ripplep;


	// SLIDE NODES /////////////////////////////////////////////////////////////
	//! true if SlideNodes are locked, false if not
	bool SlideNodesLocked;



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



};


// BEAM Inlined methods ////////////////////////////////////////////////////////

inline ground_model_t* Beam::getLastFuzzyGroundModel()
{
	return lastFuzzyGroundModel;
}

inline float Beam::getSteeringAngle()
{
	return hydrodircommand;
}

inline std::string Beam::getTruckName()
{
	return realtruckname;
}

inline std::vector<authorinfo_t> Beam::getAuthors()
{
	return authors;
}

inline std::vector<std::string> Beam::getDescription()
{
	return description;
}

inline int Beam::getBeamCount()
{
	return free_beam;
}

inline beam_t* Beam::getBeams()
{
	return beams;
}

inline float Beam::getDefaultDeformation()
{
	return default_deform;
}

inline int Beam::getNodeCount()
{
	return free_node;
}

inline node_t* Beam::getNodes()
{
	return nodes;
}

inline void Beam::setMass(float m)
{
	truckmass = m;
}

inline bool Beam::getBrakeLightVisible()
{
	if(state==NETWORKED)
		return netBrakeLight;

//		return (brake > 0.15 && !parkingbrake);
	return (brake > 0.15);
}

inline bool Beam::getCustomLightVisible(int number)
{
	return netCustomLightArray[number] != -1
			&& flares[netCustomLightArray[number]].controltoggle_status;
}

inline void Beam::setCustomLightVisible(int number, bool visible)
{
	if(netCustomLightArray[number] == -1)
		return;
	flares[netCustomLightArray[number]].controltoggle_status = visible;
}


inline bool Beam::getBeaconMode()
{
	return beacon;
}

inline blinktype Beam::getBlinkType()
{
	return blinkingtype;
}

inline bool Beam::getCustomParticleMode()
{
	return cparticle_mode;
}

inline int Beam::getLowestNode()
{
	return lowestnode;
}

inline int Beam::getTruckTime()
{
	return nettimer->getMilliseconds();
}

inline int Beam::getNetTruckTimeOffset()
{
	return net_toffset;
}

inline Real Beam::getMinimalCameraRadius()
{
	return minCameraRadius;
}

inline Replay* Beam::getReplay()
{
	return replay;
}

inline bool Beam::getSlideNodesLockInstant() 
{
	return slideNodesConnectInstantly;
}

inline bool Beam::inRange(float num, float min, float max)
{
	return (num <= max && num >= min);
}
#endif //BEAM_H__

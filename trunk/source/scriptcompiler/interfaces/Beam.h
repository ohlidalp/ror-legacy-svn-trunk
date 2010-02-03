#ifndef __Beam_H__
#define __Beam_H__


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


/**
 *  @brief Beam class that represents a truck
 */
class Beam
{
public:
	/**
	 * scales the truck. Attention, small values will crash your truck!
	 * @param value scaling in percent: 0.1f = 10%, 1.2f = 120%
	 * @see currentScale to get scale of the truck
	 */
	void scaleTruck(float value)
	{ printf("%-30s| %f\n", __FUNCTION__, value); };

	/**
	 * returns the truck name (first line of truck file)
	 * @return name of the truck, can contain spaces
	 */
	std::string getTruckName()
	{ printf("%-30s|\n", __FUNCTION__); return "";};
	
	/**
	 * Resets all beams and nodes to the initialization position that is translated via px and py
	 * @param px x position on the loaded terrain
	 * @param pz z position on the loaded terrain
	 * @param setI should the new resetted coordinates be new initialization coordinates
	 * @param miny minimal Y position: if smaller than -9000, use the terrain height. Otherwise use the value as Y translation
	 */
	void resetPosition(float px, float pz, bool setI, float miny=-9999.0)
	{ printf("%-30s| %f, %f, %d, %f\n", __FUNCTION__, px, pz, setI, miny); };

	/**
	 * Sets the detail Level for the truck visuals
	 * @param v detail level: 0=full details, 1=no beams
	 */
	void setDetailLevel(int v)
	{ printf("%-30s| %d\n", __FUNCTION__, v); };

	/**
	 * shows the skeleton
	 * @param meshes show meshes in shading/wireframe, or hide meshes completely
	 * @param newMode use new line skeleton mode or old beam mesh mode
	 */
	void showSkeleton(bool meshes=true, bool newMode=false)
	{ printf("%-30s| %d, %d\n", __FUNCTION__, meshes, newMode); };

	/**
	 * hides the skeleton
	 * @param newMode if the skeleton was shown with newMode=true, you need to hide it with newMode=true again
	 */
	void hideSkeleton(bool newMode=false)
	{ printf("%-30s| %d\n", __FUNCTION__, newMode); };

	/**
	 * toggles the parking break
	 * @see parkingbrake to get the current value of the paring break
	 */
	void parkingbrakeToggle()
	{ printf("%-30s|\n", __FUNCTION__); };

	/**
	 * toggles the rotating warning beacons
	 * @see getBeaconMode to get the current value
	 */
	void beaconsToggle()
	{ printf("%-30s|\n", __FUNCTION__); };

	/**
	 * enable or disable replay mode
	 * @param rm true to enable, false to disable replay mode
	 */
	void setReplayMode(bool rm)
	{ printf("%-30s| %d\n", __FUNCTION__, rm); };

	/**
	 * resets the auto pilot settings
	 */
	void resetAutopilot()
	{ printf("%-30s|\n", __FUNCTION__); };

	/**
	 * toggles custom particles
	 * @see getCustomParticleMode to get the current value
	 */
	void toggleCustomParticles()
	{ printf("%-30s|\n", __FUNCTION__); };

	/**
	 * returns last used beam's default deformation setting
	 * @return default deformation of beams
	 */
	float getDefaultDeformation()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * get amount of nodes used for the truck
	 * @return amount of used nodes
	 */
	int getNodeCount()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };
	
	/**
	 * returns the total mass of the truck
	 * @param withLocked if true, account locked trucks, if not, ignore.
	 * @return total mass in kilograms
	 */
	float getTotalMass(bool withLocked=true)
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * returns the nodes used for wheels
	 * @return amount of nodes
	 */
	int getWheelNodeCount()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * re-distribute the mass over the truck.
	 */
	void recalc_masses()
	{ printf("%-30s|\n", __FUNCTION__); };

	/**
	 * sets the global mass of the truck
	 * @param m trucks main mass in kilograms
	 */
	void setMass(float m)
	{ printf("%-30s| %f\n", __FUNCTION__, m); };

	/**
	 * returns if the brakelights are visible
	 * @return brakelights visible
	 */
	bool getBrakeLightVisible()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };
	
	/**
	 * get the status of a custom light
	 * @param number number of the custom light
	 * @return true if on, false if off
	 */
	bool getCustomLightVisible(int number)
	{ printf("%-30s| %d\n", __FUNCTION__, number); return false; };

	/**
	 * set the status of a custom light
	 * @param number number of the custom light
	 * @param visible true for visible, false for invisible
	 */
	void setCustomLightVisible(int number, bool visible)
	{ printf("%-30s| %d, %d\n", __FUNCTION__, number, visible); };

	/**
	 * get the status of the beacon light
	 * @return true if on, false if off
	 */
	bool getBeaconMode()
	{ printf("%-30s|\n", __FUNCTION__); return false; };

	/**
	 * set the blink type
	 * @param blink the blink type
	 */
	void setBlinkType(int blink)
	{ printf("%-30s|\n", __FUNCTION__); };

	/**
	 * get the blink type
	 * @return the blink type
	 */
	int getBlinkType()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * get on/off state of the custom particles
	 * @return true for on, false for off
	 */
	bool getCustomParticleMode()
	{ printf("%-30s|\n", __FUNCTION__); return false; };

	/**
	 * get node number with minimal Y coordinate
	 * @return node number
	 */
	int getLowestNode()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * show or hide all meshes on the truck
	 * @param visible true for show meshes, false for hidden meshes
	 */
	void setMeshVisibility(bool visible)
	{ printf("%-30s| %d\n", __FUNCTION__, visible); };

	/**
	 * gets the status of the reverse light
	 * @return true for on, false for off
	 */
	bool getReverseLightVisible()
	{ printf("%-30s|\n", __FUNCTION__); return false; };

	/**
	 * gets heading angle in radiants
	 * @return radiants
	 */
	float getHeadingDirectionAngle()
	{ printf("%-30s|\n", __FUNCTION__); return 0.0f; };

	
	// properties
	float brake;                           //!< current brake force, parkingbrake affects this
	float currentScale;                    //!< current scale, measured by the spawning scale
	int nodedebugstate;                    //!< -1 if node debug is not initialized
	int debugVisuals;                      //!< status of the node debug display
	bool networking;                       //!< if true, we are the players truck in multiplayer
	int label;                             //!< number of the truck
	int trucknum;                          //!< number of the truck
	bool cparticle_mode;                   //!< mode of the custom particles. true if on, false if off
	float WheelSpeed;                      //!< internal wheel speed. This is NOT in m/s. Look in sources for scale or measure yourself
	int skeleton;                          //!< skeleton mode:0=disabled, 1=old skeleton mode, 2=new mode
	bool replaymode;                       //!< replaymode enabled or disabled
	int replaylen;                         //!< lenght of the replay
	int replaypos;                         //!< position of the replay
	int locked;                            //!< lock status: 0=UNLOCK, 1=PRELOCK, 2=LOCKED
	bool cparticle_enabled;                //!< custom particles enabled?
	int hookId;                            //!< node number of the hook node
	Beam *lockTruck;                       //!< pointer to locked truck, 0 if unlocked
	int free_node;                         //!< amount of used nodes
	int dynamicMapMode;                    //!< dynamic overview mode that rotates with the truck?
	int tied;                              //!< tie active? 0=not tied, 1=tied
	int canwork;                           //!< enough pressure to use commands?
	int hashelp;                           //!< has help image?
	float minx,maxx,miny,maxy,minz,maxz;   //!< bounding box of the truck in its initial spawn position
	/**
	 * status of the truck: \n
	 * 0 = ACTIVATED (leading truck)\n
	 * 1 = DESACTIVATED (not leading but active)\n
	 * 2 = MAYSLEEP (active but wanting to sleep)\n
	 * 3 = GOSLEEP (active but ordered to sleep ASAP (synchronously))\n
	 * 4 = SLEEPING (not active)\n
	 * 5 = NETWORKED (truck controlled by remote player)\n
	 * 6 = RECYCLE (truck not in use) (Multiplayer)\n
	 */
	int state;
	int sleepcount;                        //!< internal counter that decides whether a truck may go sleeping or not
	/**
	 * describes the truck type
	 * 0 = NOT_DRIVEABLE (player cannot enter the truck usually)
	 * 1 = TRUCK (normal truck)
	 * 2 = AIRPLANE (plane)
	 * 3 = BOAT (boat)
	 * 4 = MACHINE (simplified GUI, always power for commands, no engine)
	 */
	int driveable;
	int importcommands;                    //!< imports commands?
	bool requires_wheel_contact;           //!< wheel contact internal register
	bool wheel_contact_requested;          //!< wheel contact internal register
	bool rescuer;                          //!< truck is recuer?
	int parkingbrake;                      //!< parking brake status
	int lights;                            //!< front lights status
	int smokeId;                           //!< node id of the smoke node (deprecated)
	int editorId;                          //!< node id of the editor node
	float leftMirrorAngle;                 //!< angle of the left mirror
	float refpressure;                     //!< pressure internals
	int free_pressure_beam;                //!< available pressure for beam commands
	int done_count;                        //!< used for thread synchronization, do not write ...
	int free_prop;                         //!< amount of used props
	float default_beam_diameter;           //!< default thickness of beams
	float skeleton_beam_diameter;          //!< thickness of beams in skeleton mode
	int free_aeroengine;                   //!< amount of used aircraft engines
	float elevator;                        //!< position of the elevator
	float rudder;                          //!< position of the rudder
	float aileron;                         //!< position of the aileron
	int flap;                              //!< flap mode. valid modes:0=none, ..., 5=full flaps
	int free_wing;                         //!< amount of used wing segments
	float fadeDist;                        //!< distance in which the truck will fade away
	bool disableDrag;                      //!< enables / disables air drag forces
	int currentcamera;                     //!< camera mode number of the internal used cinecam
	int freecinecamera;                    //!< amount of used cine cameras
	float brakeforce;                      //!< value used when braking, from the 'brakes' section
	bool ispolice;                         //!< enables police siren instead of horn
	int loading_finished;                  //!< truck loaded completely if 1
	int freecamera;                        //!< amount of used cameras
	int first_wheel_node;                  //!< node of the first wheel
	int netbuffersize;                     //!< buffer size used for receiving/sending Multiplayer data
	int nodebuffersize;                    //!< buffer size that contains the nodes during Multiplayer
	float speedoMax;                       //!< maximum value of the speedometer
	bool useMaxRPMforGUI;                  //!< use maximum RPM for GUI? see guisettings
	std::string realtruckfilename;         //!< the truck filename where the truck was loaded from
	int free_wheel;                        //!< amount of used wheels
	int airbrakeval;                       //!< status of the airbrakes
	int cameranodecount;                   //!< amount of used nodes for cameras
	int free_cab;                          //!< amount of used cab sections
	bool meshesVisible;                    //!< meshes visible or invisible?
	
	// AS utils
	void addRef(){};
	void release(){};
};


#endif

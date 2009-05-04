#ifndef __Beam_H__
#define __Beam_H__

class Beam
{
public:
	void scaleTruck(float value) {};
	std::string getTruckName(){return "";};
	void resetPosition(float px, float pz, bool setI, float miny=-9999.0) {};
	void setDetailLevel(int v) {};
	void showSkeleton(bool meshes=true, bool newMode=false) {};
	void hideSkeleton(bool newMode=false) {};
	void parkingbrakeToggle() {};
	void beaconsToggle() {};
	void setReplayMode(bool rm) {};
	void resetAutopilot() {};
	void toggleCustomParticles() {};
	float getDefaultDeformation(){ return 0;}
	int getNodeCount(){return free_node;};
	float getTotalMass(bool withLocked=true) {return 0;};
	int getWheelNodeCount() {return 0;};
	void recalc_masses() {};
	void setMass(float m) {};
	bool getBrakeLightVisible() {return 0;};
	bool getCustomLightVisible(int number) {return 0;};
	void setCustomLightVisible(int number, bool visible) {};
	bool getBeaconMode() { return false; };
	void setBlinkType(int blink) {};
	int getBlinkType() { return 0; };
	bool getCustomParticleMode() { return false; };
	int getLowestNode() { return 0; };
	void setMeshVisibility(bool visible) {};
	bool getReverseLightVisible() {return 0;};
	float getHeadingDirectionAngle() {return 0;};

	
	// properties
	float brake;
	float currentScale;
	int nodedebugstate;
	int debugVisuals;
	bool networking;
	int label;
	int trucknum;
	float WheelSpeed;
	int skeleton;
	bool replaymode;
	int replaylen;
	int replaypos;
	int locked;
	bool cparticle_enabled;
	int hookId;
	Beam *lockTruck;
	int free_node;
	int dynamicMapMode;
	int tied;
	int canwork;
	int hashelp;
	float minx,maxx,miny,maxy,minz,maxz;
	int state;
	int sleepcount;
	int driveable;
	int importcommands;
	bool requires_wheel_contact;
	bool wheel_contact_requested;
	bool rescuer;
	int parkingbrake;
	int lights;
	int smokeId;
	int editorId;
	float leftMirrorAngle;
	float refpressure;
	int free_pressure_beam;
	int done_count;
	int free_prop;
	float default_beam_diameter;
	float skeleton_beam_diameter;
	int free_aeroengine;
	float elevator;
	float rudder;
	float aileron;
	int flap;
	int free_wing;
	float fadeDist;
	bool disableDrag;
	int currentcamera;
	int freecinecamera;
	float brakeforce;
	bool ispolice;
	int loading_finished;
	int freecamera;
	int first_wheel_node;
	int netbuffersize;
	int nodebuffersize;
	float speedoMax;
	bool useMaxRPMforGUI;
	std::string realtruckfilename;
	int free_wheel;
	int airbrakeval;
	int cameranodecount;
	int free_cab;
	bool meshesVisible;
	
	

	void addRef(){};
	void release(){};
};


#endif

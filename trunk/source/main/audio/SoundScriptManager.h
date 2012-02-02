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
#ifdef USE_OPENAL

#ifndef __SoundScriptManager_H__
#define __SoundScriptManager_H__

#include "RoRPrerequisites.h"
#include "Ogre.h"
#include "Beam.h"
#include "OgreScriptLoader.h"
#include "OgreResourceGroupManager.h"
#include "Sound.h"
#include "SoundManager.h"
#include "Singleton.h"

// TODO: fix this fugly defines into a proper enum
#define MAX_SOUNDS_PER_SCRIPT 16
#define MAX_INSTANCES_PER_GROUP 256
#define MAX_SOUNDLINK_ITEM 64

#define PITCHDOWN_FADE_FACTOR   3.0
#define PITCHDOWN_CUTOFF_FACTOR 5.0

//list of sound triggers
enum {
	SS_TRIG_NONE = -1,
	SS_TRIG_ENGINE = 0,
	SS_TRIG_AEROENGINE1,
	SS_TRIG_AEROENGINE2,
	SS_TRIG_AEROENGINE3,
	SS_TRIG_AEROENGINE4,
	SS_TRIG_HORN,
	SS_TRIG_BRAKE,
	SS_TRIG_PUMP,
	SS_TRIG_STARTER,
	SS_TRIG_ALWAYSON,
	SS_TRIG_REPAIR,
	SS_TRIG_AIR,
	SS_TRIG_GPWS_APDISCONNECT,
	SS_TRIG_GPWS_10,
	SS_TRIG_GPWS_20,
	SS_TRIG_GPWS_30,
	SS_TRIG_GPWS_40,
	SS_TRIG_GPWS_50,
	SS_TRIG_GPWS_100,
	SS_TRIG_GPWS_PULLUP,
	SS_TRIG_GPWS_MINIMUMS,
	SS_TRIG_AIR_PURGE,
	SS_TRIG_SHIFT,
	SS_TRIG_GEARSLIDE,
	SS_TRIG_CREAK,
	SS_TRIG_BREAK,
	SS_TRIG_SCREETCH,
	SS_TRIG_PARK,
	SS_TRIG_AFTERBURNER1,
	SS_TRIG_AFTERBURNER2,
	SS_TRIG_AFTERBURNER3,
	SS_TRIG_AFTERBURNER4,
	SS_TRIG_AFTERBURNER5,
	SS_TRIG_AFTERBURNER6,
	SS_TRIG_AFTERBURNER7,
	SS_TRIG_AFTERBURNER8,
	SS_TRIG_AEROENGINE5,
	SS_TRIG_AEROENGINE6,
	SS_TRIG_AEROENGINE7,
	SS_TRIG_AEROENGINE8,
	SS_TRIG_AOA,
	SS_TRIG_IGNITION,
	SS_TRIG_REVERSE_GEAR,
	SS_TRIG_TURN_SIGNAL,
	SS_TRIG_TURN_SIGNAL_TICK,
	SS_TRIG_TURN_SIGNAL_WARN_TICK,
	SS_TRIG_ALB_ACTIVE,
	SS_TRIG_TC_ACTIVE,
	SS_TRIG_AVICHAT01,
	SS_TRIG_AVICHAT02,
	SS_TRIG_AVICHAT03,
	SS_TRIG_AVICHAT04,
	SS_TRIG_AVICHAT05,
	SS_TRIG_AVICHAT06,
	SS_TRIG_AVICHAT07,
	SS_TRIG_AVICHAT08,
	SS_TRIG_AVICHAT09,
	SS_TRIG_AVICHAT10,
	SS_TRIG_AVICHAT11,
	SS_TRIG_AVICHAT12,
	SS_TRIG_AVICHAT13,
	SS_LINKED_COMMAND,
	SS_MAX_TRIG
};

//list of modulation sources
enum {
	SS_MOD_NONE,
	SS_MOD_ENGINE,
	SS_MOD_TURBO,
	SS_MOD_AEROENGINE1,
	SS_MOD_AEROENGINE2,
	SS_MOD_AEROENGINE3,
	SS_MOD_AEROENGINE4,
	SS_MOD_WHEELSPEED,
	SS_MOD_INJECTOR,
	SS_MOD_TORQUE,
	SS_MOD_GEARBOX,
	SS_MOD_CREAK,
	SS_MOD_BREAK,
	SS_MOD_SCREETCH,
	SS_MOD_PUMP,
	SS_MOD_THROTTLE1,
	SS_MOD_THROTTLE2,
	SS_MOD_THROTTLE3,
	SS_MOD_THROTTLE4,
	SS_MOD_THROTTLE5,
	SS_MOD_THROTTLE6,
	SS_MOD_THROTTLE7,
	SS_MOD_THROTTLE8,
	SS_MOD_AEROENGINE5,
	SS_MOD_AEROENGINE6,
	SS_MOD_AEROENGINE7,
	SS_MOD_AEROENGINE8,
	SS_MOD_AIRSPEED,
	SS_MOD_AOA,
	SS_MOD_LINKED_COMMANDRATE,
	SS_MAX_MOD
};

enum {SL_DEFAULT,
	SL_COMMAND, 
	SL_HYDRO, 
	SL_COLLISION, 
	SL_SHOCKS, 
	SL_BRAKES, 
	SL_ROPES, 
	SL_TIES, 
	SL_PARTICLES, 
	SL_AXLES, 
	SL_FLARES, 
	SL_FLEXBODIES, 
	SL_EXHAUSTS, 
	SL_VIDEOCAMERA, 
	SL_MAX};

class SoundScriptTemplate
{
public:
	SoundScriptTemplate(Ogre::String name, Ogre::String groupname, Ogre::String filename, bool baseTemplate);
	bool setParameter(StringVector vec);
	~SoundScriptTemplate();

//protected:
	int parseModulation(Ogre::String str);
	Ogre::String name;
	Ogre::String groupname;
	Ogre::String filename;
	int          trigger_source;
	int          pitch_source;
	float        pitch_offset;
	float        pitch_multiplier;
	float        pitch_square;
	int          gain_source;
	float        gain_offset;
	float        gain_multiplier;
	float        gain_square;
	bool         has_start_sound;
	float        start_sound_pitch;
	Ogre::String start_sound_name;
	bool         has_stop_sound;
	float        stop_sound_pitch;
	Ogre::String stop_sound_name;
	bool         unpitchable;
	int          free_sound;
	float        sound_pitches[MAX_SOUNDS_PER_SCRIPT];
	Ogre::String sound_names[MAX_SOUNDS_PER_SCRIPT];
	bool         baseTemplate;
};

class SoundScriptInstance
{
public:
	SoundScriptInstance(int truck, SoundScriptTemplate* templ, SoundManager* sm, Ogre::String instancename, int soundLinkType=SL_DEFAULT, int soundLinkItemId=-1);
	void setPitch(float value);
	void setGain(float value);
	void setPosition(Ogre::Vector3 pos, Ogre::Vector3 velocity);
	void runOnce();
	void start();
	void stop();
	
	int truck;  // holds the number of the truck this is for. important
	int soundLinkType; // holds the SL_ type this is bound to
	int soundLinkItemId; // holds the item number this is for

	void setEnabled(bool e);
	SoundScriptTemplate* templ;

protected:
	SoundManager* sm;
	Sound *startSound;
	Sound *stopSound;
	Sound *sounds[MAX_SOUNDS_PER_SCRIPT];
	float startSound_pitchgain;
	float stopSound_pitchgain;
	float sounds_pitchgain[MAX_SOUNDS_PER_SCRIPT];
	float lastgain;
	float pitchgain_cutoff(float sourcepitch, float targetpitch);
};

class SoundScriptManager : public ScriptLoader , public RoRSingleton<SoundScriptManager>
{
public:
	const static int TERRAINSOUND = MAX_TRUCKS+1;

	SoundScriptManager();

	// ScriptLoader interface
    const StringVector& getScriptPatterns(void) const;
    void parseScript(DataStreamPtr& stream, const Ogre::String& groupName);
    Real getLoadingOrder(void) const;

	SoundScriptInstance* createInstance(Ogre::String templatename, int truck, SceneNode *toAttach=NULL, int soundLinkType=SL_DEFAULT, int soundLinkItemId=-1);
	void unloadResourceGroup(Ogre::String groupname);
	void clearNonBaseTemplates();



	// functions
	void trigOnce    (int truck, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	void trigOnce    (Beam *b,   int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	void trigStart   (int truck, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	void trigStart   (Beam *b,   int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	void trigStop    (int truck, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	void trigStop    (Beam *b,   int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	void trigToggle  (int truck, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	void trigToggle  (Beam *b,   int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	bool getTrigState(int truck, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	bool getTrigState(Beam *b,   int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
	void modulate    (int truck, int mod, float value, int linkType = SL_DEFAULT, int linkItemID=-1);
	void modulate    (Beam *b,   int mod, float value, int linkType = SL_DEFAULT, int linkItemID=-1);

	void soundEnable(bool state);

	void setCamera(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity);
	void setLoadingBaseSounds(bool v) { loadingBase = v; };

	inline bool working() { return !soundsDisabled; }

	float maxDistance;
	float rolloffFactor;
	float referenceDistance;
protected:
    StringVector mScriptPatterns;
	int instance_counter;
	bool loadingBase;
	bool soundsDisabled;

	std::map <Ogre::String, SoundScriptTemplate*> templates;

	void skipToNextCloseBrace(DataStreamPtr& chunk);
	void skipToNextOpenBrace(DataStreamPtr& chunk);
	SoundScriptTemplate* createTemplate(Ogre::String name, Ogre::String groupname, Ogre::String filename);

	//instances lookup tables
	int free_trigs[SS_MAX_TRIG];
	SoundScriptInstance *trigs[SS_MAX_TRIG * MAX_INSTANCES_PER_GROUP];

	int free_pitches[SS_MAX_MOD];
	SoundScriptInstance *pitches[SS_MAX_MOD * MAX_INSTANCES_PER_GROUP];
	
	int free_gains[SS_MAX_MOD];
	SoundScriptInstance *gains[SS_MAX_MOD * MAX_INSTANCES_PER_GROUP];

	//state map
	// TODO: replace with STL container to save memory
	// soundLinks, soundItems, trucks, triggers
	std::map <int, std::map <int, std::map <int, std::map <int, bool > > > > statemap;

	SoundManager* sm;
};

#endif // __SoundScriptManager_H__

#endif // USE_OPENAL


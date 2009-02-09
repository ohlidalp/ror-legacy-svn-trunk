/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#ifndef __BeamAudio_H__
#define __BeamAudio_H__

#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif

#include "math.h"

#include"al.h"
#include"alc.h"
#include"alut.h"

#define REF_DIST 7.5

#define NO_PROPS 0
#define TURBOPROPS 1
#define PISTONPROPS 2
#define TURBOJETS 3

#define AUDIO_BUFF_CREAK 0
#define AUDIO_BUFF_IDDLE 1
#define AUDIO_BUFF_LOW   2
#define AUDIO_BUFF_MED   3
#define AUDIO_BUFF_HIGH  4
#define AUDIO_BUFF_FORCE 5
#define AUDIO_BUFF_BREAKS 6
#define AUDIO_BUFF_BRAKE 7
#define AUDIO_BUFF_PARK  8
#define AUDIO_BUFF_PSHI  9
#define AUDIO_BUFF_HYDROSTART 10
#define AUDIO_BUFF_HYDRO 11
#define AUDIO_BUFF_HYDROSTOP 12
#define AUDIO_BUFF_STARTERSTART 13
#define AUDIO_BUFF_STARTER 14
#define AUDIO_BUFF_STARTERSTOP 15
#define AUDIO_BUFF_HORNSTART 16
#define AUDIO_BUFF_HORN  17
#define AUDIO_BUFF_HORNSTOP 18
#define AUDIO_BUFF_TURBO 19
#define AUDIO_BUFF_VALVE 20
#define AUDIO_BUFF_CLACK 21
#define AUDIO_BUFF_SPSHI 22
#define AUDIO_BUFF_REPAIR 23
#define AUDIO_BUFF_DOOR 24
#define AUDIO_BUFF_TPLOPOWER 25
#define AUDIO_BUFF_TPHIPOWER 26
#define AUDIO_BUFF_TPSTART 27
#define AUDIO_BUFF_SCREETCH 28
#define AUDIO_BUFF_CRAA 29
#define AUDIO_BUFF_CAR 30
#define AUDIO_BUFF_POLICE 31
#define AUDIO_BUFF_MARINELARGE 32
#define AUDIO_BUFF_MARINESMALL 33
#define AUDIO_BUFF_CHATTER01 34
#define AUDIO_BUFF_CHATTER02 35
#define AUDIO_BUFF_CHATTER03 36
#define AUDIO_BUFF_CHATTER04 37
#define AUDIO_BUFF_CHATTER05 38
#define AUDIO_BUFF_CHATTER06 39
#define AUDIO_BUFF_CHATTER07 40
#define AUDIO_BUFF_CHATTER08 41
#define AUDIO_BUFF_CHATTER09 42
#define AUDIO_BUFF_CHATTER10 43
#define AUDIO_BUFF_CHATTER11 44
#define AUDIO_BUFF_CHATTER12 45
#define AUDIO_BUFF_CHATTER13 46
#define AUDIO_BUFF_GPWS_10 47
#define AUDIO_BUFF_GPWS_20 48
#define AUDIO_BUFF_GPWS_30 49
#define AUDIO_BUFF_GPWS_40 50
#define AUDIO_BUFF_GPWS_50 51
#define AUDIO_BUFF_GPWS_100 52
#define AUDIO_BUFF_GPWS_MINIMUMS 53
#define AUDIO_BUFF_GPWS_PULLUP 54
#define AUDIO_BUFF_GPWS_APDISCONNECT 55
#define AUDIO_BUFF_PPLOPOWER 56
#define AUDIO_BUFF_PPHIPOWER 57
#define AUDIO_BUFF_PPSTART 58
#define AUDIO_BUFF_TJSTART 59
#define AUDIO_BUFF_TJENGINE 60
#define AUDIO_BUFF_TJENGINE2 61
#define AUDIO_BUFF_TJAB 62

#define AUDIO_NUMBUFF 63

#define AUDIO_GLOBAL_BEAMS 0
#define AUDIO_GLOBAL_FORCE 1
#define AUDIO_GLOBAL_BRAKE 2
#define AUDIO_GLOBAL_PSHI 3
#define AUDIO_GLOBAL_HYDRO 4
#define AUDIO_GLOBAL_STARTER 5
#define AUDIO_GLOBAL_HORN 6
#define AUDIO_GLOBAL_TURBO 7

#define AUDIO_NUMGLOBAL 8

#define AUDIO_LOCAL_ENGINE1 0
#define AUDIO_LOCAL_ENGINE2 1

#define AUDIO_NUMLOCAL 2

#define AUDIO_LOCALTP_ENGINE1A 0
#define AUDIO_LOCALTP_ENGINE1B 1
#define AUDIO_LOCALTP_ENGINE2A 2
#define AUDIO_LOCALTP_ENGINE2B 3
#define AUDIO_LOCALTP_ENGINE3A 4
#define AUDIO_LOCALTP_ENGINE3B 5
#define AUDIO_LOCALTP_ENGINE4A 6
#define AUDIO_LOCALTP_ENGINE4B 7
#define AUDIO_NUMLOCALTP 8

class BeamAudio
{
protected:
	int brakstate;
	int pshistate;
	int hydrostate;
	int starterstate;
	int parkstate;
	int enginestate;
	int hasengine;
	int hasaeroengines;
	int hasboat;
	ALuint localsources[AUDIO_NUMLOCAL];
	ALuint localtpsources[AUDIO_NUMLOCALTP];
	ALuint *buffers;
	ALuint *globalsources;
	ALuint hydrolist[256];
	ALuint starterlist[256];
	ALuint hornlist[256];
	char enginetype;
	bool policestate;
	int boatsource;
	float chattertimer;
	float next_chatter;
	int chatnum;

public:
	int hornstate;


	BeamAudio(ALuint *gbuffers, ALuint *gsources);
	void setEngineType(char type);
	void setupEngine();
	void mySourcePlay(ALuint source);
	void mySetBuffer(ALuint source, ALuint buffer);
	void setPosition(int iscurrent, float px, float py, float pz, float vx, float vy, float vz);
	void playCreak(float factor);
	void playScreetch(float factor);
	void playBreak(float factor);
	void playStart();
	void playStop();
	void setAcc(float v);
	void brakon();
	void brakoff();
	void playValve();
	void playClack();
	void playCraa();
	void playDoor();
	void playRepair();
	void playSPshi();
	void parkon();
	void parkoff();
	void pshion();
	void pshioff();
	void hydroon(float stress);
	void hydrooff();
	void starteron();
	void starteroff();
	void hornon();
	void hornoff();
	void policeToggle(int state=-1);
	void setSpeed(float rpm);
	void setTurbo(float rpm);
	//AIRPLANE
	void setupAeroengines(int type);
	void playAEStart(int num);
	void stopAEStart(int num);
	void setAERPM(int freetp, float *rpms, float *cs, bool *warmup);
	void setAEPosition(int iscurrent, int freetp,
		float *px, float *py, float *pz,
		float *vx, float *vy, float *vz
		);

	void setupBoat(float mass);
	void setBoatThrotle(float val);
	void playChatter(float dt);
	void playGPWS(int id);
	
	bool getPoliceState() { return policestate; };
};




class BeamAudioManager
{
protected:
	ALCdevice *device;
	ALCcontext *context;
	ALuint buffers[AUDIO_NUMBUFF];
	ALuint sources[AUDIO_NUMGLOBAL];
public:
	BeamAudioManager(Ogre::String cfgpath);
	bool LoadWave(char *sfname, ALuint buffer);
	void setListenerPosition(float px, float py, float pz, float vx, float vy, float vz, float dx, float dy, float dz, float ux, float uy, float uz);
	BeamAudio* createBeamAudio();
	void soundEnable(bool enable);
	~BeamAudioManager();
};


#endif

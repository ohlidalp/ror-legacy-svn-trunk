/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#include"audio.h"
#include "ExampleFrameListener.h" // for cfg file path

BeamAudio::BeamAudio(ALuint *gbuffers, ALuint *gsources)
{
	brakstate=0;
	pshistate=0;
	hydrostate=0;
	starterstate=0;
	parkstate=0;
	hornstate=0;
	enginestate=0;
	hasengine=0;
	hasaeroengines=NO_PROPS;
	hasboat=0;
	buffers=gbuffers;
	globalsources=gsources;
	chattertimer=0;
	next_chatter=10;	
	chatnum=0;
	enginetype='t';
	policestate=false;

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		alListenerf(AL_GAIN, 1.0);
		alDopplerFactor(1.0); // don't exaggerate doppler shift
		alDopplerVelocity(343.0); // using meters/second
#endif

	int i;
	//prepare the lists
	hydrolist[0]=buffers[AUDIO_BUFF_HYDROSTART];
	for (i=1; i<256; i++) hydrolist[i]=buffers[AUDIO_BUFF_HYDRO];

	starterlist[0]=buffers[AUDIO_BUFF_STARTERSTART];
	for (i=1; i<256; i++) starterlist[i]=buffers[AUDIO_BUFF_STARTER];

	hornlist[0]=buffers[AUDIO_BUFF_HORNSTART];
	for (i=1; i<256; i++) hornlist[i]=buffers[AUDIO_BUFF_HORN];
}

void BeamAudio::setEngineType(char type)
{
	enginetype=type;
}

void BeamAudio::setupEngine()
{
	int i;
	ALint error;
	alGetError();
	alGenSources(AUDIO_NUMLOCAL, localsources);
	if((error = alGetError()) != AL_NO_ERROR )
		Ogre::LogManager::getSingleton().logMessage("AUDIO LOCAL SOURCE ERROR "+Ogre::StringConverter::toString(error));

	//no binding yet
	for (i=0; i<AUDIO_NUMLOCAL; i++)
	{
		//special sauce
		alSourcef(localsources[i], AL_REFERENCE_DISTANCE, REF_DIST);
		alSourcei(localsources[i], AL_LOOPING, AL_TRUE);
		alSourcef(localsources[i], AL_GAIN, 0.0f);
	}
	hasengine=1;

}

void BeamAudio::mySourcePlay(ALuint source)
{
	int value;
	alGetSourcei(source, AL_SOURCE_STATE, &value);
	if (value!=AL_PLAYING) alSourcePlay(source);
}

void BeamAudio::mySetBuffer(ALuint source, ALuint buffer)
{
	ALint value;
	alGetSourcei(source, AL_BUFFER, &value);
	if ((ALuint)value!=buffer) 
	{
		alSourceStop(source);
		alSourcei(source, AL_BUFFER, buffer);
	}
}

void BeamAudio::setPosition(int iscurrent, float px, float py, float pz, float vx, float vy, float vz)
{
	int i;
	
/*Ogre::LogManager::getSingleton().logMessage("AUDIO SOURCEPOS "
										+Ogre::StringConverter::toString(iscurrent)+", "
										+Ogre::StringConverter::toString(px)+", "
										+Ogre::StringConverter::toString(py)+", "
										+Ogre::StringConverter::toString(pz));
*/		for (i=0; i<AUDIO_NUMLOCAL; i++)
	{
		alSource3f(localsources[i], AL_POSITION, px,py,pz);
		alSource3f(localsources[i], AL_VELOCITY, vx,vy,vz);
	}
	if (iscurrent)
	{
		for (i=0; i<AUDIO_NUMGLOBAL; i++)
		{
			alSource3f(globalsources[i], AL_POSITION, px,py,pz);
			alSource3f(globalsources[i], AL_VELOCITY, vx,vy,vz);
		}
	}

/*		float kx, ky, kz;
alGetSource3f(localsources[0], AL_POSITION, &kx, &ky, &kz);
Ogre::LogManager::getSingleton().logMessage("AUDIO SOURCEPOS "
										+Ogre::StringConverter::toString(kx)+", "
										+Ogre::StringConverter::toString(ky)+", "
										+Ogre::StringConverter::toString(kz));
*/	}

//factor 0..1
void BeamAudio::playCreak(float factor)
{
	int value;
	if (factor<0.0f || factor>1.0f) return;
	alGetSourcei(globalsources[AUDIO_GLOBAL_BEAMS], AL_SOURCE_STATE, &value);
	if (value==AL_PLAYING) return;
	alSourcei(globalsources[AUDIO_GLOBAL_BEAMS], AL_BUFFER, buffers[AUDIO_BUFF_CREAK]);
	alSourcef(globalsources[AUDIO_GLOBAL_BEAMS], AL_PITCH, 1.0f+factor/5.0f);
	alSourcef(globalsources[AUDIO_GLOBAL_BEAMS], AL_GAIN, 0.3f+factor);
	mySourcePlay(globalsources[AUDIO_GLOBAL_BEAMS]);
}

//factor 0..1
void BeamAudio::playScreetch(float factor)
{
	int value;
	alSourcef(globalsources[AUDIO_GLOBAL_HYDRO], AL_GAIN, factor);
	alGetSourcei(globalsources[AUDIO_GLOBAL_HYDRO], AL_SOURCE_STATE, &value);
	if (value==AL_PLAYING) return;
	alSourcei(globalsources[AUDIO_GLOBAL_HYDRO], AL_BUFFER, buffers[AUDIO_BUFF_SCREETCH]);
	alSourcef(globalsources[AUDIO_GLOBAL_HYDRO], AL_PITCH, 1.0f);
	mySourcePlay(globalsources[AUDIO_GLOBAL_HYDRO]);
}


//factor 0..1
void BeamAudio::playBreak(float factor)
{
//		int value;
	//alGetSourcei(globalsources[AUDIO_GLOBAL_BEAMS], AL_SOURCE_STATE, &value);
	//if (value==AL_PLAYING) return;
	if (factor<0.0f || factor>1.0f) return;
	alSourceStop(globalsources[AUDIO_GLOBAL_BEAMS]);
	alSourcei(globalsources[AUDIO_GLOBAL_BEAMS], AL_BUFFER, buffers[AUDIO_BUFF_BREAKS]);
	alSourcef(globalsources[AUDIO_GLOBAL_BEAMS], AL_PITCH, 1.0f+factor);
	alSourcef(globalsources[AUDIO_GLOBAL_BEAMS], AL_GAIN, 1.0);
	mySourcePlay(globalsources[AUDIO_GLOBAL_BEAMS]);
}

void BeamAudio::playStart()
{
	if (!hasengine) return;
	if (enginetype=='t')
	{
		mySourcePlay(globalsources[AUDIO_GLOBAL_TURBO]);
		mySourcePlay(globalsources[AUDIO_GLOBAL_FORCE]);
	}
	enginestate=1;
}

void BeamAudio::playStop()
{
	if (!hasengine) return;
	enginestate=0;
	alSourceStop(localsources[AUDIO_LOCAL_ENGINE1]);
	alSourceStop(localsources[AUDIO_LOCAL_ENGINE2]);
	alSourcef(globalsources[AUDIO_GLOBAL_FORCE], AL_GAIN, 0);
	alSourcef(globalsources[AUDIO_GLOBAL_TURBO], AL_GAIN, 0);
//		alSourceStop(globalsources[AUDIO_GLOBAL_TURBO]);
//		alSourceStop(globalsources[AUDIO_GLOBAL_FORCE]);
}

//factor 0..1
/*	void BeamAudio::playGrind(float factor)
{
	int value;
	alGetSourcei(sources[AUDIO_GRIND], AL_SOURCE_STATE, &value);
	if (value==AL_PLAYING) return;
	alSourcef(sources[AUDIO_GRIND], AL_PITCH, 1.0f+factor/10.0);
	mySourcePlay(sources[AUDIO_GRIND]);
}
*/
void BeamAudio::setAcc(float v)
{
	if (!enginestate) return;
	if (enginetype=='t') alSourcef(globalsources[AUDIO_GLOBAL_FORCE], AL_GAIN, v/2.0f);
	if (enginetype=='c') alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_GAIN, 0.5+v/2.0);
}

void BeamAudio::brakon()
{
	alSourceStop(globalsources[AUDIO_GLOBAL_BRAKE]);
	brakstate=1;
}

void BeamAudio::brakoff()
{
	if (enginetype=='c') return;
	if (brakstate)
	{
		alSourceStop(globalsources[AUDIO_GLOBAL_BRAKE]);
		alSourcei(globalsources[AUDIO_GLOBAL_BRAKE], AL_BUFFER, buffers[AUDIO_BUFF_BRAKE]);
		//alSourceRewind(sources[AUDIO_BRAKE]);
		mySourcePlay(globalsources[AUDIO_GLOBAL_BRAKE]);
		brakstate=0;
	}
}

void BeamAudio::playValve()
{
	alSourceStop(globalsources[AUDIO_GLOBAL_BRAKE]);
	alSourcei(globalsources[AUDIO_GLOBAL_BRAKE], AL_BUFFER, buffers[AUDIO_BUFF_VALVE]);
	mySourcePlay(globalsources[AUDIO_GLOBAL_BRAKE]);
}

void BeamAudio::playClack()
{
	alSourceStop(globalsources[AUDIO_GLOBAL_BRAKE]);
	alSourcei(globalsources[AUDIO_GLOBAL_BRAKE], AL_BUFFER, buffers[AUDIO_BUFF_CLACK]);
	mySourcePlay(globalsources[AUDIO_GLOBAL_BRAKE]);
}

void BeamAudio::playCraa()
{
	alSourceStop(globalsources[AUDIO_GLOBAL_BRAKE]);
	alSourcei(globalsources[AUDIO_GLOBAL_BRAKE], AL_BUFFER, buffers[AUDIO_BUFF_CRAA]);
	mySourcePlay(globalsources[AUDIO_GLOBAL_BRAKE]);
}

void BeamAudio::playDoor()
{
	alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_DOOR]);
	mySourcePlay(globalsources[AUDIO_GLOBAL_STARTER]);
}

void BeamAudio::playRepair()
{
	alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_REPAIR]);
	mySourcePlay(globalsources[AUDIO_GLOBAL_STARTER]);
}

void BeamAudio::playSPshi()
{
	return;
	int value;
	alGetSourcei(globalsources[AUDIO_GLOBAL_BRAKE], AL_SOURCE_STATE, &value);
	if (value==AL_PLAYING) return;
	alSourceStop(globalsources[AUDIO_GLOBAL_BRAKE]);
	alSourcei(globalsources[AUDIO_GLOBAL_BRAKE], AL_BUFFER, buffers[AUDIO_BUFF_SPSHI]);
	mySourcePlay(globalsources[AUDIO_GLOBAL_BRAKE]);
}

void BeamAudio::parkon()
{
	if (enginetype=='c') return;
	if (!parkstate)
	{
		alSourceStop(globalsources[AUDIO_GLOBAL_BRAKE]);
		alSourcei(globalsources[AUDIO_GLOBAL_BRAKE], AL_BUFFER, buffers[AUDIO_BUFF_PARK]);
		//alSourceRewind(sources[AUDIO_BRAKE]);
		mySourcePlay(globalsources[AUDIO_GLOBAL_BRAKE]);
		parkstate=1;
	}
}

void BeamAudio::parkoff()
{
	alSourceStop(globalsources[AUDIO_GLOBAL_BRAKE]);
	parkstate=0;
}


void BeamAudio::pshion()
{
	if (!pshistate)
	{
		mySourcePlay(globalsources[AUDIO_GLOBAL_PSHI]);
		pshistate=1;
	}
}

void BeamAudio::pshioff()
{
	if (pshistate)
	{
		alSourceStop(globalsources[AUDIO_GLOBAL_PSHI]);
		pshistate=0;
	};
}

void BeamAudio::hydroon(float stress)
{
	alSourcef(globalsources[AUDIO_GLOBAL_HYDRO], AL_GAIN, 1.0);
	alSourcef(globalsources[AUDIO_GLOBAL_HYDRO], AL_PITCH, 1.0f-stress/100.0f);
	if (hydrostate==0)
	{
		alSourceStop(globalsources[AUDIO_GLOBAL_HYDRO]);
		alSourcei(globalsources[AUDIO_GLOBAL_HYDRO], AL_BUFFER, 0);

		alSourceQueueBuffers(globalsources[AUDIO_GLOBAL_HYDRO], 256, hydrolist);
		mySourcePlay(globalsources[AUDIO_GLOBAL_HYDRO]);

		hydrostate=1;
	}
}

void BeamAudio::hydrooff()
{
	if (hydrostate) 
	{
		ALuint list[256];
		alSourceStop(globalsources[AUDIO_GLOBAL_HYDRO]);
		alSourceUnqueueBuffers(globalsources[AUDIO_GLOBAL_HYDRO], 256, list);
		alSourcei(globalsources[AUDIO_GLOBAL_HYDRO], AL_BUFFER, buffers[AUDIO_BUFF_HYDROSTOP]);
		mySourcePlay(globalsources[AUDIO_GLOBAL_HYDRO]);
		hydrostate=0;
	}
}

void BeamAudio::starteron()
{
	if (starterstate==0)
	{
		alSourceStop(globalsources[AUDIO_GLOBAL_STARTER]);
		alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, 0);

		alSourceQueueBuffers(globalsources[AUDIO_GLOBAL_STARTER], 256, starterlist);
		mySourcePlay(globalsources[AUDIO_GLOBAL_STARTER]);
		starterstate=1;
	}
}

void BeamAudio::starteroff()
{
	if (starterstate) 
	{
		ALuint list[256];
		alSourceStop(globalsources[AUDIO_GLOBAL_STARTER]);
		alSourceUnqueueBuffers(globalsources[AUDIO_GLOBAL_STARTER], 256, list);
		alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_STARTERSTOP]);
		mySourcePlay(globalsources[AUDIO_GLOBAL_STARTER]);

		starterstate=0;
	}
}

void BeamAudio::hornon()
{
	if (hornstate==0)
	{
		alSourceStop(globalsources[AUDIO_GLOBAL_HORN]);
		alSourcei(globalsources[AUDIO_GLOBAL_HORN], AL_BUFFER, 0);

		alSourceQueueBuffers(globalsources[AUDIO_GLOBAL_HORN], 256, hornlist);
		mySourcePlay(globalsources[AUDIO_GLOBAL_HORN]);

		hornstate=1;
	}

}

void BeamAudio::hornoff()
{
	if (hornstate) 
	{
		ALuint list[256];
		alSourceStop(globalsources[AUDIO_GLOBAL_HORN]);
		alSourceUnqueueBuffers(globalsources[AUDIO_GLOBAL_HORN], 256, list);
		alSourcei(globalsources[AUDIO_GLOBAL_HORN], AL_BUFFER, buffers[AUDIO_BUFF_HORNSTOP]);
		mySourcePlay(globalsources[AUDIO_GLOBAL_HORN]);
		hornstate=0;
	}
}

void BeamAudio::policeToggle(int state)
{
	if(state==-1)
		policestate=!policestate;
	else if(state>0 && !policestate)
		policestate = true;
	else if(state>0 && policestate)
		// already on
		return;
	else if(state==0 && policestate)
		policestate = false;
	else if(state==0 && !policestate)
		// already off
		return;

	if (policestate)
	{
		alSourcei(globalsources[AUDIO_GLOBAL_HORN], AL_BUFFER, buffers[AUDIO_BUFF_POLICE]);
		alSourcei(globalsources[AUDIO_GLOBAL_HORN], AL_LOOPING, AL_TRUE);
		mySourcePlay(globalsources[AUDIO_GLOBAL_HORN]);
	}
	else
	{
		alSourceStop(globalsources[AUDIO_GLOBAL_HORN]);
		alSourcei(globalsources[AUDIO_GLOBAL_HORN], AL_LOOPING, AL_FALSE);
	}
}
/*
void setTrans(float rpm, float torque)
{
	if (!trans) return;
	trans->setPitchShift(rpm/200.0f);
//		float vol=rpm/200.0f*torque/100000.0f;
	float vol=torque/200000.0f;
	if (vol>1.0f) vol=1.0f;
	trans->setVolume(vol);
}
*/
void BeamAudio::setSpeed(float rpm)
{
	if (!hasengine || rpm<0 || !enginestate) return;
	if (enginetype=='t')
	{
		if (rpm<750)
		{
			mySetBuffer(localsources[AUDIO_LOCAL_ENGINE1], buffers[AUDIO_BUFF_IDDLE]);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_PITCH, rpm/750.0f);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_GAIN, 1.0f);

			mySourcePlay(localsources[AUDIO_LOCAL_ENGINE1]);
			alSourceStop(localsources[AUDIO_LOCAL_ENGINE2]);
		}
		else if (rpm<849)
		{
			mySetBuffer(localsources[AUDIO_LOCAL_ENGINE1], buffers[AUDIO_BUFF_IDDLE]);
			mySetBuffer(localsources[AUDIO_LOCAL_ENGINE2], buffers[AUDIO_BUFF_LOW]);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_PITCH, rpm/750.0f);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_GAIN, 1.0f-(rpm-750.0f)/(849.0f-750.0f));
			alSourcef(localsources[AUDIO_LOCAL_ENGINE2], AL_PITCH, rpm/849.0f);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE2], AL_GAIN, (rpm-750.0f)/(849.0f-750.0f));

			mySourcePlay(localsources[AUDIO_LOCAL_ENGINE1]);
			mySourcePlay(localsources[AUDIO_LOCAL_ENGINE2]);
		}
		else if (rpm<1035)
		{
			mySetBuffer(localsources[AUDIO_LOCAL_ENGINE1], buffers[AUDIO_BUFF_LOW]);
			mySetBuffer(localsources[AUDIO_LOCAL_ENGINE2], buffers[AUDIO_BUFF_MED]);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_PITCH, rpm/849.0f);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_GAIN, 1.0f-(rpm-849.0f)/(1035.0f-849.0f));
			alSourcef(localsources[AUDIO_LOCAL_ENGINE2], AL_PITCH, rpm/1035.0f);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE2], AL_GAIN, (rpm-849.0f)/(1035.0f-849.0f));

			mySourcePlay(localsources[AUDIO_LOCAL_ENGINE1]);
			mySourcePlay(localsources[AUDIO_LOCAL_ENGINE2]);
		}
		else if (rpm<1455)
		{
			mySetBuffer(localsources[AUDIO_LOCAL_ENGINE1], buffers[AUDIO_BUFF_MED]);
			mySetBuffer(localsources[AUDIO_LOCAL_ENGINE2], buffers[AUDIO_BUFF_HIGH]);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_PITCH, rpm/1035.0f);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_GAIN, 1.0f-(rpm-1035.0f)/(1455.0f-1035.0f));
			alSourcef(localsources[AUDIO_LOCAL_ENGINE2], AL_PITCH, rpm/1455.0f);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE2], AL_GAIN, (rpm-1035.0f)/(1455.0f-1035.0f));

			mySourcePlay(localsources[AUDIO_LOCAL_ENGINE1]);
			mySourcePlay(localsources[AUDIO_LOCAL_ENGINE2]);
		} else
		{
			mySetBuffer(localsources[AUDIO_LOCAL_ENGINE1], buffers[AUDIO_BUFF_HIGH]);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_PITCH, rpm/1455.0f);
			alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_GAIN, 1.0);

			mySourcePlay(localsources[AUDIO_LOCAL_ENGINE1]);
			alSourceStop(localsources[AUDIO_LOCAL_ENGINE2]);
		}
	}
	if (enginetype=='c')
	{
		mySetBuffer(localsources[AUDIO_LOCAL_ENGINE1], buffers[AUDIO_BUFF_CAR]);
		alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_PITCH, rpm/1500.0f);


		mySourcePlay(localsources[AUDIO_LOCAL_ENGINE1]);
		alSourceStop(localsources[AUDIO_LOCAL_ENGINE2]);
	}

}

void BeamAudio::setTurbo(float rpm)
{
	alSourcef(globalsources[AUDIO_GLOBAL_TURBO], AL_PITCH, rpm/100000.0f);
	alSourcef(globalsources[AUDIO_GLOBAL_TURBO], AL_GAIN, 0.06f*rpm/180000.0f*rpm/180000.0f);
}

//AIRPLANE
void BeamAudio::setupAeroengines(int type)
{
	if (hasaeroengines) return;
	int i;
	ALint error;
	alGetError();
	alGenSources(AUDIO_NUMLOCALTP, localtpsources);
	if((error = alGetError()) != AL_NO_ERROR )
		Ogre::LogManager::getSingleton().logMessage("AUDIO LOCAL SOURCE ERROR "+Ogre::StringConverter::toString(error));

	//no binding yet
	for (i=0; i<AUDIO_NUMLOCALTP; i++)
	{
		//special sauce
		alSourcef(localtpsources[i], AL_REFERENCE_DISTANCE, REF_DIST);
		alSourcei(localtpsources[i], AL_LOOPING, AL_TRUE);
		alSourcef(localtpsources[i], AL_GAIN, 0.0f);
	}
	hasaeroengines=type;
}

void BeamAudio::playAEStart(int num)
{
	if (hasaeroengines==TURBOPROPS)	mySetBuffer(localtpsources[AUDIO_LOCALTP_ENGINE1A+num*2+1], buffers[AUDIO_BUFF_TPSTART]);
	else if (hasaeroengines==PISTONPROPS) mySetBuffer(localtpsources[AUDIO_LOCALTP_ENGINE1A+num*2+1], buffers[AUDIO_BUFF_PPSTART]);
	else if (hasaeroengines==TURBOJETS) mySetBuffer(localtpsources[AUDIO_LOCALTP_ENGINE1A+num*2+1], buffers[AUDIO_BUFF_TJSTART]);
	alSourcef(localtpsources[AUDIO_LOCALTP_ENGINE1A+num*2+1], AL_PITCH, 1.0);
	alSourcef(localtpsources[AUDIO_LOCALTP_ENGINE1A+num*2+1], AL_GAIN, 0.4);
	alSourcei(localtpsources[AUDIO_LOCALTP_ENGINE1A+num*2+1], AL_LOOPING, AL_FALSE);
	mySourcePlay(localtpsources[AUDIO_LOCALTP_ENGINE1A+num*2+1]);
}
void BeamAudio::stopAEStart(int num)
{
	alSourceStop(localtpsources[AUDIO_LOCALTP_ENGINE1A+num*2+1]);
}

void BeamAudio::setAERPM(int freetp, float *rpms, float *cs, bool *warmup)
{
	if (!hasaeroengines) return;
	int i;
	for (i=0; i<freetp; i++)
	{
		int enginea=AUDIO_LOCALTP_ENGINE1A+i*2;
		int engineb=AUDIO_LOCALTP_ENGINE1A+i*2+1;
		float mrpm=rpms[i];
		if (hasaeroengines==TURBOJETS)
		{
			if (mrpm<10.0f)
			{
				alSourceStop(localtpsources[enginea]);
				if (!warmup[i]) alSourceStop(localtpsources[engineb]);
			}
			else
			{
				mySetBuffer(localtpsources[enginea], buffers[AUDIO_BUFF_TJENGINE]);
				alSourcef(localtpsources[enginea], AL_PITCH, mrpm/25.0f);
				float gain=(mrpm-10.0f)/20.0f; if (gain>1.0f) gain=1.0f;
				if (mrpm>50.0f) gain=(75.0f-mrpm)/25.0f;
				if (gain<0.0f) gain=0.0f;
				alSourcef(localtpsources[enginea], AL_GAIN, gain);
				mySourcePlay(localtpsources[enginea]);
				if (!warmup[i]) 
				{
					if (mrpm>40.0f)
					{
						if (cs[i]>0.5)
						{
							//afterburner
							mySetBuffer(localtpsources[engineb], buffers[AUDIO_BUFF_TJAB]);
							alSourcef(localtpsources[engineb], AL_PITCH, 1.0f);
							alSourcef(localtpsources[engineb], AL_GAIN, 1.0f);
							alSourcei(localtpsources[engineb], AL_LOOPING, AL_TRUE);
							mySourcePlay(localtpsources[engineb]);
						}
						else
						{
							gain=(mrpm-30.0f)/70.0f;
							mySetBuffer(localtpsources[engineb], buffers[AUDIO_BUFF_TJENGINE2]);
							alSourcef(localtpsources[engineb], AL_PITCH, 1.0f);
							alSourcef(localtpsources[engineb], AL_GAIN, gain);
							alSourcei(localtpsources[engineb], AL_LOOPING, AL_TRUE);
							mySourcePlay(localtpsources[engineb]);
						}
					}
					else
					{
						alSourceStop(localtpsources[engineb]);
					}
				}
			}
		}
		else
		{
			if (mrpm<100.0f)
			{
				alSourceStop(localtpsources[enginea]);
				if (!warmup[i]) alSourceStop(localtpsources[engineb]);
			}
			else
			{
				if (cs[i]<0.5f)
				{
					if (hasaeroengines==TURBOPROPS)
						mySetBuffer(localtpsources[enginea], buffers[AUDIO_BUFF_TPLOPOWER]);
					else
						mySetBuffer(localtpsources[enginea], buffers[AUDIO_BUFF_PPLOPOWER]);
					alSourcef(localtpsources[enginea], AL_PITCH, mrpm/1000.0f);
					alSourcef(localtpsources[enginea], AL_GAIN, 0.5f+cs[i]/2.0f);

					mySourcePlay(localtpsources[enginea]);
					if (!warmup[i]) alSourceStop(localtpsources[engineb]);
				}
				else
				{
					if (hasaeroengines==TURBOPROPS)
						mySetBuffer(localtpsources[enginea], buffers[AUDIO_BUFF_TPLOPOWER]);
					else
						mySetBuffer(localtpsources[enginea], buffers[AUDIO_BUFF_PPLOPOWER]);
					if (!warmup[i]) 
					{
						if (hasaeroengines==TURBOPROPS)
							mySetBuffer(localtpsources[engineb], buffers[AUDIO_BUFF_TPHIPOWER]);
						else
							mySetBuffer(localtpsources[engineb], buffers[AUDIO_BUFF_PPHIPOWER]);
					}
					alSourcef(localtpsources[enginea], AL_PITCH, mrpm/1000.0f);
					if (!warmup[i]) alSourcef(localtpsources[engineb], AL_PITCH, mrpm/1000.0f);
					if (!warmup[i]) alSourcei(localtpsources[engineb], AL_LOOPING, AL_TRUE);
					alSourcef(localtpsources[enginea], AL_GAIN, 0.5f+cs[i]/2.0f);
					if (!warmup[i]) alSourcef(localtpsources[engineb], AL_GAIN, (cs[i]-0.5f));

					mySourcePlay(localtpsources[enginea]);
					if (!warmup[i]) mySourcePlay(localtpsources[engineb]);
				}
			}
		}
	}
}

void BeamAudio::setAEPosition(int iscurrent, int freetp,
	float *px, float *py, float *pz,
	float *vx, float *vy, float *vz
	)
{
	int i,j;
	for (i=0; i<2; i++)
	{
		for (j=0; j<freetp; j++)
		{
			alSource3f(localtpsources[i+j*2], AL_POSITION, px[j],py[j],pz[j]);
			alSource3f(localtpsources[i+j*2], AL_VELOCITY, vx[j],vy[j],vz[j]);
		}
	}

	if (iscurrent)
	{
		for (i=0; i<AUDIO_NUMGLOBAL; i++)
		{
			alSource3f(globalsources[i], AL_POSITION, px[0],py[0],pz[0]);
			alSource3f(globalsources[i], AL_VELOCITY, vx[0],vy[0],vz[0]);
		}
	}
}

void BeamAudio::playGPWS(int id)
{
	alSourcei(globalsources[AUDIO_GLOBAL_BRAKE], AL_BUFFER, buffers[id]);
	mySourcePlay(globalsources[AUDIO_GLOBAL_BRAKE]);
}


void BeamAudio::playChatter(float dt)
{
	chattertimer+=dt;
	if (chattertimer>=next_chatter)
	{
		switch (chatnum)
		{
		case 0: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER01]);break;
		case 1: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER02]);break;
		case 2: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER03]);break;
		case 3: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER04]);break;
		case 4: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER05]);break;
		case 5: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER06]);break;
		case 6: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER07]);break;
		case 7: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER08]);break;
		case 8: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER09]);break;
		case 9: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER10]);break;
		case 10: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER11]);break;
		case 11: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER12]);break;
		case 12: alSourcei(globalsources[AUDIO_GLOBAL_STARTER], AL_BUFFER, buffers[AUDIO_BUFF_CHATTER13]);break;
		}
		chatnum++;
		mySourcePlay(globalsources[AUDIO_GLOBAL_STARTER]);
		next_chatter=chattertimer+60.0*((float)rand()/(float)RAND_MAX)+10.0;
	}
}

void BeamAudio::setupBoat(float mass)
{
	if (hasboat) return;
	int i;
	ALint error;
	if (mass<50000.0) boatsource=AUDIO_BUFF_MARINESMALL; else boatsource=AUDIO_BUFF_MARINELARGE;
	alGetError();
	alGenSources(AUDIO_NUMLOCAL, localsources);
	if((error = alGetError()) != AL_NO_ERROR )
		Ogre::LogManager::getSingleton().logMessage("AUDIO LOCAL SOURCE ERROR "+Ogre::StringConverter::toString(error));

	//no binding yet
	for (i=0; i<AUDIO_NUMLOCAL; i++)
	{
		//special sauce
		alSourcef(localsources[i], AL_REFERENCE_DISTANCE, REF_DIST);
		alSourcei(localsources[i], AL_LOOPING, AL_TRUE);
		alSourcef(localsources[i], AL_GAIN, 0.0f);
	}
	hasboat=1;
}

void BeamAudio::setBoatThrotle(float val)
{
mySetBuffer(localsources[AUDIO_LOCAL_ENGINE1], buffers[boatsource]);
alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_PITCH, 0.5f+val/2.0f);
alSourcef(localsources[AUDIO_LOCAL_ENGINE1], AL_GAIN, 1.0f+val/2.0f);


mySourcePlay(localsources[AUDIO_LOCAL_ENGINE1]);
alSourceStop(localsources[AUDIO_LOCAL_ENGINE2]);
}





BeamAudioManager::BeamAudioManager(Ogre::String cfgpath)
{
	//open device
	Ogre::ConfigFile cfg;
	// Don't trim whitespace
	cfg.load(cfgpath, "\t:=", false);
//return;
	if (cfg.getSetting("3D Sound renderer")=="Hardware")
		device=alcOpenDevice("Generic Hardware");
	else
		device=alcOpenDevice("Generic Software");

	//device=alcOpenDevice(NULL);
	if (device)
	{
		context=alcCreateContext(device, NULL);
		alcMakeContextCurrent(context);
	} else return;
	//create buffers
	alGenBuffers(AUDIO_NUMBUFF, buffers);
	LoadWave("data\\sounds\\creak.wav", buffers[AUDIO_BUFF_CREAK]);
	LoadWave("data\\sounds\\engine\\iddle.wav", buffers[AUDIO_BUFF_IDDLE]);
	LoadWave("data\\sounds\\engine\\low.wav", buffers[AUDIO_BUFF_LOW]);
	LoadWave("data\\sounds\\engine\\medium.wav", buffers[AUDIO_BUFF_MED]);
	LoadWave("data\\sounds\\engine\\high.wav", buffers[AUDIO_BUFF_HIGH]);
	LoadWave("data\\sounds\\engine\\force.wav", buffers[AUDIO_BUFF_FORCE]);
	LoadWave("data\\sounds\\break.wav", buffers[AUDIO_BUFF_BREAKS]);
	LoadWave("data\\sounds\\brake.wav", buffers[AUDIO_BUFF_BRAKE]);
	LoadWave("data\\sounds\\park.wav", buffers[AUDIO_BUFF_PARK]);
	LoadWave("data\\sounds\\pshi.wav", buffers[AUDIO_BUFF_PSHI]);
	LoadWave("data\\sounds\\hydrostart.wav", buffers[AUDIO_BUFF_HYDROSTART]);
	LoadWave("data\\sounds\\hydro.wav", buffers[AUDIO_BUFF_HYDRO]);
	LoadWave("data\\sounds\\hydrostop.wav", buffers[AUDIO_BUFF_HYDROSTOP]);
	LoadWave("data\\sounds\\starterstart.wav", buffers[AUDIO_BUFF_STARTERSTART]);
	LoadWave("data\\sounds\\starter.wav", buffers[AUDIO_BUFF_STARTER]);
	LoadWave("data\\sounds\\starterstop.wav", buffers[AUDIO_BUFF_STARTERSTOP]);
	LoadWave("data\\sounds\\hornstart.wav", buffers[AUDIO_BUFF_HORNSTART]);
	LoadWave("data\\sounds\\horn.wav", buffers[AUDIO_BUFF_HORN]);
	LoadWave("data\\sounds\\hornstop.wav", buffers[AUDIO_BUFF_HORNSTOP]);
	LoadWave("data\\sounds\\turbo.wav", buffers[AUDIO_BUFF_TURBO]);
	LoadWave("data\\sounds\\valve.wav", buffers[AUDIO_BUFF_VALVE]);
	LoadWave("data\\sounds\\clack.wav", buffers[AUDIO_BUFF_CLACK]);
	LoadWave("data\\sounds\\spshi.wav", buffers[AUDIO_BUFF_SPSHI]);
	LoadWave("data\\sounds\\repair.wav", buffers[AUDIO_BUFF_REPAIR]);
	LoadWave("data\\sounds\\door.wav", buffers[AUDIO_BUFF_DOOR]);
	LoadWave("data\\sounds\\turboprop\\lopower.wav", buffers[AUDIO_BUFF_TPLOPOWER]);
	LoadWave("data\\sounds\\turboprop\\hipower.wav", buffers[AUDIO_BUFF_TPHIPOWER]);
	LoadWave("data\\sounds\\turboprop\\start.wav", buffers[AUDIO_BUFF_TPSTART]);
	LoadWave("data\\sounds\\screetch.wav", buffers[AUDIO_BUFF_SCREETCH]);
	LoadWave("data\\sounds\\craa.wav", buffers[AUDIO_BUFF_CRAA]);
	LoadWave("data\\sounds\\car\\idle.wav", buffers[AUDIO_BUFF_CAR]);
	LoadWave("data\\sounds\\police.wav", buffers[AUDIO_BUFF_POLICE]);
	LoadWave("data\\sounds\\marine\\largediesel.wav", buffers[AUDIO_BUFF_MARINELARGE]);
	LoadWave("data\\sounds\\marine\\smalldiesel.wav", buffers[AUDIO_BUFF_MARINESMALL]);
	LoadWave("data\\sounds\\cockpit\\chatter01.wav", buffers[AUDIO_BUFF_CHATTER01]);
	LoadWave("data\\sounds\\cockpit\\chatter02.wav", buffers[AUDIO_BUFF_CHATTER02]);
	LoadWave("data\\sounds\\cockpit\\chatter03.wav", buffers[AUDIO_BUFF_CHATTER03]);
	LoadWave("data\\sounds\\cockpit\\chatter04.wav", buffers[AUDIO_BUFF_CHATTER04]);
	LoadWave("data\\sounds\\cockpit\\chatter05.wav", buffers[AUDIO_BUFF_CHATTER05]);
	LoadWave("data\\sounds\\cockpit\\chatter06.wav", buffers[AUDIO_BUFF_CHATTER06]);
	LoadWave("data\\sounds\\cockpit\\chatter07.wav", buffers[AUDIO_BUFF_CHATTER07]);
	LoadWave("data\\sounds\\cockpit\\chatter08.wav", buffers[AUDIO_BUFF_CHATTER08]);
	LoadWave("data\\sounds\\cockpit\\chatter09.wav", buffers[AUDIO_BUFF_CHATTER09]);
	LoadWave("data\\sounds\\cockpit\\chatter10.wav", buffers[AUDIO_BUFF_CHATTER10]);
	LoadWave("data\\sounds\\cockpit\\chatter11.wav", buffers[AUDIO_BUFF_CHATTER11]);
	LoadWave("data\\sounds\\cockpit\\chatter12.wav", buffers[AUDIO_BUFF_CHATTER12]);
	LoadWave("data\\sounds\\cockpit\\chatter13.wav", buffers[AUDIO_BUFF_CHATTER13]);
	LoadWave("data\\sounds\\cockpit\\GPWS10.WAV", buffers[AUDIO_BUFF_GPWS_10]);
	LoadWave("data\\sounds\\cockpit\\GPWS20.WAV", buffers[AUDIO_BUFF_GPWS_20]);
	LoadWave("data\\sounds\\cockpit\\GPWS30.WAV", buffers[AUDIO_BUFF_GPWS_30]);
	LoadWave("data\\sounds\\cockpit\\GPWS40.WAV", buffers[AUDIO_BUFF_GPWS_40]);
	LoadWave("data\\sounds\\cockpit\\GPWS50.WAV", buffers[AUDIO_BUFF_GPWS_50]);
	LoadWave("data\\sounds\\cockpit\\GPWS100.WAV", buffers[AUDIO_BUFF_GPWS_100]);
	LoadWave("data\\sounds\\cockpit\\GPWSminimums.wav", buffers[AUDIO_BUFF_GPWS_MINIMUMS]);
	LoadWave("data\\sounds\\cockpit\\GPWSpullup.wav", buffers[AUDIO_BUFF_GPWS_PULLUP]);
	LoadWave("data\\sounds\\cockpit\\GPWSapdisconnect.wav", buffers[AUDIO_BUFF_GPWS_APDISCONNECT]);
	LoadWave("data\\sounds\\pistonprop\\lopower.wav", buffers[AUDIO_BUFF_PPLOPOWER]);
	LoadWave("data\\sounds\\pistonprop\\hipower.wav", buffers[AUDIO_BUFF_PPHIPOWER]);
	LoadWave("data\\sounds\\pistonprop\\start.wav", buffers[AUDIO_BUFF_PPSTART]);
	LoadWave("data\\sounds\\turbojet\\start.wav", buffers[AUDIO_BUFF_TJSTART]);
	LoadWave("data\\sounds\\turbojet\\engine.wav", buffers[AUDIO_BUFF_TJENGINE]);
	LoadWave("data\\sounds\\turbojet\\engine2.wav", buffers[AUDIO_BUFF_TJENGINE2]);
	LoadWave("data\\sounds\\turbojet\\ab.wav", buffers[AUDIO_BUFF_TJAB]);
	
	//take care of global sources
	int i;
	ALint error;
	alGetError();
	alGenSources(AUDIO_NUMGLOBAL, sources);
	if((error = alGetError()) != AL_NO_ERROR )
		Ogre::LogManager::getSingleton().logMessage("AUDIO GLOBAL SOURCE ERROR "+Ogre::StringConverter::toString(error));

	//no binding yet
	for (i=0; i<AUDIO_NUMGLOBAL; i++)
	{
		//special sauce
		alSourcef(sources[i], AL_REFERENCE_DISTANCE, REF_DIST);
	}
	
	//some bindings
	alSourcei(sources[AUDIO_GLOBAL_FORCE], AL_BUFFER, buffers[AUDIO_BUFF_FORCE]);
	alSourcei(sources[AUDIO_GLOBAL_FORCE], AL_LOOPING, AL_TRUE);
	alSourcef(sources[AUDIO_GLOBAL_FORCE], AL_GAIN, 0.0f);

	alSourcei(sources[AUDIO_GLOBAL_TURBO], AL_BUFFER, buffers[AUDIO_BUFF_TURBO]);
	alSourcei(sources[AUDIO_GLOBAL_TURBO], AL_LOOPING, AL_TRUE);
	alSourcef(sources[AUDIO_GLOBAL_TURBO], AL_GAIN, 0.0f);

	alSourcei(sources[AUDIO_GLOBAL_PSHI], AL_BUFFER, buffers[AUDIO_BUFF_PSHI]);
	alSourcei(sources[AUDIO_GLOBAL_PSHI], AL_LOOPING, AL_TRUE);


}

bool BeamAudioManager::LoadWave(char *sfname, ALuint buffer)
{
	ALint	    error;
	ALsizei     size, freq;
	ALenum	    format;
	ALvoid	    *data;
	ALboolean   loop;

	char fname[256];
	strcpy(fname, sfname);
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	char* fp;
	fp=fname;
	while (*fp!=0) {if (*fp=='\\') *fp='/'; fp++;};
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	char* fp;
	fp=fname;
	while (*fp!=0) {if (*fp=='\\') *fp='/'; fp++;};
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	alutLoadWAVFile( fname,&format,&data,&size,&freq );
#else
	alutLoadWAVFile( fname,&format,&data,&size,&freq,&loop );
#endif
	if( (error = alGetError()) != AL_NO_ERROR ) return false;


	// Copy data into ALBuffer
	alBufferData( buffer, format, data, size, freq );
	if( (error = alGetError()) != AL_NO_ERROR ) return false;


	// Unload wave file
	alutUnloadWAV(format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR) return false;


	return true;
}

void BeamAudioManager::setListenerPosition(float px, float py, float pz, float vx, float vy, float vz, float dx, float dy, float dz, float ux, float uy, float uz)
{
	if (!device) return;
	float dir[6];
	//at
	dir[0]=dx;
	dir[1]=dy;
	dir[2]=dz;
	//up
	dir[3]=ux;
	dir[4]=uy;
	dir[5]=uz;
	alListener3f(AL_POSITION, px,py,pz);
	alListener3f(AL_VELOCITY, vx,vy,vz);
	alListenerfv(AL_ORIENTATION, dir);

/*		float kx, ky, kz;
	alGetListener3f(AL_POSITION, &kx, &ky, &kz);
Ogre::LogManager::getSingleton().logMessage("AUDIO LISTENPOS "
										+Ogre::StringConverter::toString(kx)+", "
										+Ogre::StringConverter::toString(ky)+", "
										+Ogre::StringConverter::toString(kz));
*/
}

BeamAudio* BeamAudioManager::createBeamAudio()
{
	if (!device) return 0;
	BeamAudio* ba=new BeamAudio(buffers, sources);
	return ba;
}

void BeamAudioManager::soundEnable(bool enable)
{
	if (!device) return;
	if (enable)
		alListenerf(AL_GAIN, 1.0);
	else
		alListenerf(AL_GAIN, 0.0);
	//alcMakeContextCurrent(NULL);
}

BeamAudioManager::~BeamAudioManager()
{
	if (!device) return;
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

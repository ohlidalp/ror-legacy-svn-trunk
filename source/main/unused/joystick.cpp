/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#include "joystick.h"

BeamJoystick::BeamJoystick(OIS::InputManager *im, float deadzone, bool feedback, ConfigFile *cfg)
	{
		int i;
		usefeedback=feedback;
		noforce=true;
		for (i=0; i<NUM_ROLES; i++) roles[i]=-1;
		free_axe=0;
		sticknum=-1;
		povjoy=-1;
		pov=-1;
		dz=(int)(deadzone*1000.0);

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		numjoy=0;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		numjoy=im->numJoysticks();
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
//		numjoy=im->numJoySticks();
#endif
		LogManager::getSingleton().logMessage("Joy: "+StringConverter::toString(numjoy)+" joystick(s) found");
		int j,a;
		for (j=0; j<numjoy; j++)
		{
			joys[j] = static_cast<OIS::JoyStick*>(im->createInputObject(OIS::OISJoyStick, false));
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE
//			if (joys[j]->hats()>0) povjoy=j;
//			for (a=0; a<joys[j]->axes(); a++)
#endif
			{
				//we have a new axis
				char name[256];
				sprintf(name, "joy%i/axis%i", j,a);
				LogManager::getSingleton().logMessage("Joy: Axis: "+String(name));
				axes[free_axe].jnum=j;
				axes[free_axe].axis=a;
				axes[free_axe].linear=false;
				axes[free_axe].half=false;
				axes[free_axe].reverse=false;
				axes[free_axe].used=false;
				int i;
				for (i=0; i<NUM_ROLES; i++)
				{
					char line[256];
					sprintf(line, "Joy %s Func %s", axisprefix[i], axisname[i]);
					if (cfg->getSetting(line)==name)
					{
	LogManager::getSingleton().logMessage("Joy: Function "+String(line)+" OK");
						axes[free_axe].used=true;
						sprintf(line, "Joy %s Lin %s", axisprefix[i], axisname[i]);
						axes[free_axe].linear=(cfg->getSetting(line)=="Yes");
						sprintf(line, "Joy %s Rev %s", axisprefix[i], axisname[i]);
						axes[free_axe].reverse=(cfg->getSetting(line)=="Yes");
						sprintf(line, "Joy %s Hlf %s", axisprefix[i], axisname[i]);
						axes[free_axe].half=(cfg->getSetting(line)=="Yes");
						roles[i]=free_axe;
						if (i==AXIS_VEHICLE_STICK) sticknum=j;
					}
				}
				free_axe++;
			}
		}

	}

	BeamJoystick::~BeamJoystick()
	{
		//we should destroy joysticks
	}


void BeamJoystick::UpdateInputState()
{
	//read all entries
	int j,k;
	OIS::JoyStickState jstate[64];
	for (j=0; j<numjoy; j++)
	{
		joys[j]->capture();
		jstate[j]=joys[j]->getJoyStickState();
	}
	for (k=0; k<free_axe; k++)
	{
		if (axes[k].used)
		{
			axes[k].value=1000*jstate[axes[k].jnum].mAxes[axes[k].axis].abs/32768;
		}
	}
	if (povjoy!=-1) pov=jstate[povjoy].mPOV[0].direction;
}

 float BeamJoystick::dead(int axis)
{
	if (axis<-dz) return (float)(axis+dz)/(1000.0-(float)dz);
	if (axis<dz) return 0.0; //dead zone
	return (float)(axis-dz)/(1000.0-(float)dz);
}

 float BeamJoystick::logval(float val)
{
	if (val>0) return log10(1.0/(1.1-val))/1.0;
	if (val==0) return 0;
	return -log10(1.0/(1.1+val))/1.0;
}

 float BeamJoystick::getAxis(int type)
{
	axis_t *ax;
	if ((type==AXIS_VEHICLE_THROTLE || type==AXIS_VEHICLE_BRAKE) && roles[AXIS_VEHICLE_COMBO]!=-1) 
		ax=&axes[roles[AXIS_VEHICLE_COMBO]];
	else
		ax=&axes[roles[type]];
	if (ax->half)
	{
		//no dead zone in half axis
		float val=(1000.0+ax->value)/2000.0;
		if (ax->reverse) val=1.0-val;
		if (!ax->linear) val=log10(1.0/(1.1-val));
		return val;
	}
	else
	{
		float val=dead(ax->value);
		if (ax->reverse) val=-val;
		if (!ax->linear) val=logval(val);
//if (type==AXIS_VEHICLE_STEERING) LogManager::getSingleton().logMessage("Joytest "+StringConverter::toString(val)+" src "+StringConverter::toString(ax->value));
		return val;

	}
};

 int BeamJoystick::getHPov()
{
	if (pov==-1) return 0;
	if (pov&OIS::Pov::East) return 1;
	if (pov&OIS::Pov::West) return -1;
	return 0;
}

 int BeamJoystick::getVPov()
{
	if (pov==-1) return 0;
	if (pov&OIS::Pov::North) return 1;
	if (pov&OIS::Pov::South) return -1;
	return 0;
}

 bool BeamJoystick::povLeft() {return getHPov()==-1;}
 bool BeamJoystick::povRight() {return getHPov()==1;}
 bool BeamJoystick::povUp() {return getVPov()==1;}
 bool BeamJoystick::povDown() {return getVPov()==-1;}

bool BeamJoystick::hasJoy(int type) {
	//special case for the combo
	if ((type==AXIS_VEHICLE_THROTLE || type==AXIS_VEHICLE_BRAKE) && roles[AXIS_VEHICLE_COMBO]!=-1) return true;
	return roles[type]!=-1;
};
bool BeamJoystick::hasForce() {return !noforce;};

//stick
int BeamJoystick::updateStickShift(bool enabled, float clutch)
{
/*
	if (sticknum==-1) return -1;
	if( NULL == g_pJoystick[sticknum] ) return -1;
	if (!enabled) {SetDeviceForcesXY(0, 0, stickEffect, g_dwNumStickAxis);return -1;}
	bool shiftenabled=clutch<0.1;
	// Poll the device to read the current state
	hr = g_pJoystick[sticknum]->Poll(); 
	if( FAILED(hr) )  
	{
		// DInput is telling us that the input stream has been
		// interrupted. We aren't tracking any state between polls, so
		// we don't have any special reset that needs to be done. We
		// just re-acquire and try again.
		hr = g_pJoystick[sticknum]->Acquire();
		while( hr == DIERR_INPUTLOST ) 
			hr = g_pJoystick[sticknum]->Acquire();

		// hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
		// may occur when the app is minimized or in the process of 
		// switching, so just try again later 
		return -1; 
	}

	// Get the input's device state
	if( FAILED( hr = g_pJoystick[sticknum]->GetDeviceState( sizeof(DIJOYSTATE2), &js ) ) )
		return -1; // The device should have been acquired during the Poll()

	//we got axis
	float posx=js.lX/1000.0;
	float posy=js.lY/1000.0;
	//we'll have force
	float fx=0;
	float fy=0;
	if (shiftenabled && posy>-0.33 && posy<0.33)
	{
		//this is neutral
		stickstate=0;
		fx=0;
		fy=0;
		//center centering
		if (posy<0.5 && posy>-0.5) {fx=posx/4.0;fy=posy/4.0;};
	}
	else
	{
		//take care of the "grip"
//		if (posy>0.66) fy=-(posy-0.66)*3.0;
//		if (posy<-0.66) fy=-(posy+0.66)*3.0;
		//center centering
//		if (posy<0.5 && posy>-0.5) {fx=posx/4.0;fy=posy/4.0;};
		//now the grid
		if (posy>0.5 && posx<-0.33 && stickstate==0 && shiftenabled) stickstate=2;
		if (posy>0.5 && posx>-0.33 && posx<0.33 && stickstate==0 && shiftenabled) stickstate=4;
		if (posy>0.5 && posx>0.33 && stickstate==0 && shiftenabled) stickstate=6;
		if (posy<-0.5 && posx<-0.33 && stickstate==0 && shiftenabled) stickstate=1;
		if (posy<-0.5 && posx>-0.33 && posx<0.33 && stickstate==0 && shiftenabled) stickstate=3;
		if (posy<-0.5 && posx>0.33 && stickstate==0 && shiftenabled) stickstate=5;
		//gridforce
		if (stickstate==1 || stickstate==2) fx=(posx+1.0)*3.0;
		if (stickstate==3 || stickstate==4) fx=posx*3.0;
		if (stickstate==5 || stickstate==6) fx=(posx-1.0)*3.0;

		if (stickstate==2 || stickstate==4 || stickstate==6) fy=-(posy-0.66)*3.0;
		if (stickstate==1 || stickstate==3 || stickstate==5) fy=-(posy+0.66)*3.0;
		//center centering
		if (stickstate==0) {fx=posx/4.0;fy=posy/4.0;};


		if (fx>1.0) fx=1.0;
		if (fx<-1.0) fx=-1.0;
	}



	//set force
	SetDeviceForcesXY(fx*DI_FFNOMINALMAX, fy*DI_FFNOMINALMAX, stickEffect, g_dwNumStickAxis);
	return stickstate;
*/
	return 0;
}


//FORCE FEEDBACK STUFF

void BeamJoystick::setForceFeedback(float fx, float fy)
{
/*	if (noforce) return;
	if (fx>1.0)  fx=1.0;
	if (fx<-1.0) fx=-1.0;
	if (fy>1.0)  fy=1.0;
	if (fy<-1.0) fy=-1.0;
	SetDeviceForcesXY(fx*DI_FFNOMINALMAX, fy*DI_FFNOMINALMAX, g_pEffect, g_dwNumFFAxis);
	*/
}



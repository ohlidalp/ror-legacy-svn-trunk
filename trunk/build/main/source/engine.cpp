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
#include "engine.h"
#include "SoundScriptManager.h"
#include "TorqueCurve.h"

BeamEngine::BeamEngine(float iddle, float max, float torque, float rear, int numgears, float *gears, float diff, int trucknum)
{
	int i;
	this->trucknum=trucknum;
	ssm=SoundScriptManager::getSingleton();
	torqueCurve = new TorqueCurve();
	automode=AUTOMATIC;
	iddleRPM=iddle;
	maxRPM=max;
	engineTorque=torque;
	brakingTorque=-engineTorque/5.0f;
	engineTorque-=brakingTorque;
	reverseRatio=rear*diff;
	numGears=numgears;
	gearsRatio = (float*)malloc(sizeof(float)*numGears+2);
	for (i=1; i<numGears+1; i++) gearsRatio[i]=gears[i]*diff;
	curGear=0;
	curEngineRPM=0;
	curGearboxRPM=0;
	curTurboRPM=0;
	curClutch=0;
	running=0;
	contact=0;
	curAcc=0;
	autocurAcc=0;
	curClutchTorque=0.0f;
	inertia=10.0;
	clutchForce=10000.0f;
	stallRPM=300.0f;
	shifting=0;
	postshifting=0;
	starter=0;
	prime=0;
	hydropump=0.0;
	apressure=0;
	autoselect=DRIVE;
	hasturbo=true;
	hasair=true;
	type='t';
}

void BeamEngine::setOptions(float einertia, char etype)
{
	inertia=einertia;
	type=etype;
	if (etype=='c')
	{
		//its a car!
		hasturbo=false;
		hasair=false;
		clutchForce=5000.0f;
	}

}

void BeamEngine::update(float dt, int doUpdate)
{
	if (hasair)
	{
		//air pressure
		apressure+=dt*curEngineRPM;
		if (apressure>50000.0) {ssm->trigOnce(trucknum, SS_TRIG_AIR_PURGE); apressure=0;};
	}
	if (hasturbo)
	{
		//update turbo speed (lag)
		float turbotorque=0.0f;
		float turboInertia=0.000003f;
		//braking (compression)
		turbotorque-=curTurboRPM/200000.0f;
		//powering (exhaust) with limiter
		if (curTurboRPM<200000 && running && contact) turbotorque+=curAcc*(curEngineRPM/maxRPM);//*(curEngineRPM/maxRPM);
		//integration
		curTurboRPM+=dt*turbotorque/turboInertia;
	}

	//update engine speed
	float totaltorque=0.0f;
	//engine braking
	if (contact) totaltorque+=brakingTorque*curEngineRPM/maxRPM;
	else totaltorque+=10.0*brakingTorque*curEngineRPM/maxRPM;
	//braking by hydropump
	if (curEngineRPM>100.0) totaltorque-=8.0f*hydropump/(curEngineRPM*0.105f*dt);
	//engine power
	//with limiter
	float tqValue = 1.0f;
	float rpmRatio = curEngineRPM / maxRPM;
	if (rpmRatio > 1.0f) rpmRatio = 1.0f;
	if (torqueCurve) tqValue = torqueCurve->getEngineTorque(rpmRatio);
	if (contact && running && curEngineRPM<(maxRPM*1.25) && curEngineRPM>stallRPM)
		totaltorque += engineTorque * curAcc * tqValue;
	if (running && curEngineRPM<stallRPM) stop();
	//starter
	if (contact && starter && curEngineRPM<stallRPM*1.5) totaltorque+=-brakingTorque;//1000.0f;
	//restart
	if (!running && curEngineRPM>stallRPM && starter)
	{
		running=1; 
		ssm->trigStart(trucknum, SS_TRIG_ENGINE);
	}
	//clutch
	float retorque=0.0f;
	if (curGear<0) 
		retorque=-curClutchTorque/reverseRatio;
	else if (curGear>0)
		retorque=curClutchTorque/gearsRatio[curGear];
	totaltorque-=retorque;
	//integration
	curEngineRPM+=dt*totaltorque/inertia;

	//update clutch torque
	float gearboxspinner=0.0f;
	if (curGear<0) 
		gearboxspinner=-curEngineRPM/reverseRatio;
	else if (curGear>0)
		gearboxspinner=curEngineRPM/gearsRatio[curGear];
	if (curGear!=0) curClutchTorque=(gearboxspinner-curGearboxRPM)*curClutch*clutchForce;
	else curClutchTorque=0.0f;

	if (curEngineRPM<0.0f) curEngineRPM=0.0f;

	if (automode!=MANUAL)
	{
		//autoshift
		if (shifting)
		{
			shiftclock+=dt;
			//clutch
			if (shiftclock<CLUTCH_TIME) curClutch=1.0f-(shiftclock/CLUTCH_TIME);
			else if (shiftclock>(SHIFT_TIME-CLUTCH_TIME)) curClutch=1.0f-(SHIFT_TIME-shiftclock)/CLUTCH_TIME;
			else curClutch=0.0f;
			//shift
			if (shiftval && shiftclock>CLUTCH_TIME/2)
			{
				ssm->trigStart(trucknum, SS_TRIG_SHIFT);
				curGear+=shiftval;
				if (automode==AUTOMATIC) while (curGear>2 && curGearboxRPM*gearsRatio[curGear]<iddleRPM) curGear-=2;
				if (curGear<-1) curGear=-1;
				if (curGear>numGears) curGear=numGears;
				shiftval=0;
			};
			//end of shifting
			if (shiftclock>SHIFT_TIME)
			{
				ssm->trigStop(trucknum, SS_TRIG_SHIFT);
				setAcc(autocurAcc);
				shifting=0;
				curClutch=1.0;
				postshifting=1;
				postshiftclock=0.0;
			}
		}
		if (postshifting)
		{
			postshiftclock+=dt;
			if (postshiftclock>POST_SHIFT_TIME) postshifting=0;
		}

		//auto-declutch
		if (shifting)
		{
			//we are shifting, just avoid stalling in worst case
			if (curEngineRPM<stallRPM*1.2) curClutch=0;
		} else if (postshifting)
		{
			//we are postshifting, no gear change
			if (curEngineRPM<stallRPM*1.2 && curAcc<0.5) curClutch=0; else curClutch=1.0;
		} else
		{
			//we are normal
			if (curEngineRPM<stallRPM*1.2 && curAcc<0.5) {curClutch=0; /*curGear=0;*/} else
			{
				//if (curGear==0 && curEngineRPM>iddleRPM && curAcc>0.5) curGear=1;
				if (curGear==1 || curGear==-1)
				{
					//1st gear : special
					if (curEngineRPM>iddleRPM) {curClutch=((curEngineRPM-iddleRPM)/(maxRPM-iddleRPM));if (curClutch>1.0) curClutch=1.0;}
					else curClutch=0.0;
				}
				else curClutch=1.0;
			}
		}
	}
	//audio stuff
	if (hasturbo) ssm->modulate(trucknum, SS_MOD_TURBO, curTurboRPM);
	if (doUpdate)
	{
		ssm->modulate(trucknum, SS_MOD_ENGINE, curEngineRPM);
		ssm->modulate(trucknum, SS_MOD_TORQUE, curClutchTorque);
		ssm->modulate(trucknum, SS_MOD_GEARBOX, curGearboxRPM);
		//gear hack
		if (curGear<0 || automode!=AUTOMATIC) return;
		if (!shifting && !postshifting && curEngineRPM>maxRPM-100.0 && curGear<numGears && autoselect==DRIVE) shift(1);
		if (!shifting && !postshifting && curEngineRPM<iddleRPM && curGear>1 && autoselect==DRIVE) shift(-1);
	}
	// reverse gear beep
	if (curGear==-1 && running) 
	{
		ssm->trigStart(trucknum, SS_TRIG_REVERSE_GEAR);
	}
	else
	{
		ssm->trigStop(trucknum, SS_TRIG_REVERSE_GEAR);
	}

}

float BeamEngine::getRPM() {return curEngineRPM;}

void BeamEngine::toggleAutoMode()
{
	automode++;
	if (automode>MANUAL) automode=AUTOMATIC;

	// this switches off all automatic symbols when in manual mode
	if(automode!=AUTOMATIC)
		autoselect = MANUALMODE;
	if(automode==AUTOMATIC)
		autoselect = NEUTRAL;
}

int BeamEngine::getAutoMode()
{
	return automode;
}

void BeamEngine::setAcc(float val)
{
	if (prime)
	{
		if (curEngineRPM<850) val=1.0;
		else if (curEngineRPM<900) {float t=(900.0f-curEngineRPM)/50.0f;if (t>val) val=t;};
	}
	ssm->modulate(trucknum, SS_MOD_INJECTOR, val);
	curAcc=val*0.94f+0.06f;
}

float BeamEngine::getTurboPSI() {return curTurboRPM/10000.0f;}

float BeamEngine::getAcc()
{
	return curAcc;
}

//this is mainly for smoke...
void BeamEngine::netForceSettings(float rpm, float force)
{
	curEngineRPM=rpm;
	curAcc=force;
	running=(fabs(rpm)>10.0);
}

float BeamEngine::getSmoke()
{
	if (running) return curAcc*(1.0f-curEngineRPM/maxRPM);//*engineTorque/5000.0;
	else return -1;
}

float BeamEngine::getTorque()
{
	if (curClutchTorque>1000000.0) return 1000000.0;
	if (curClutchTorque<-1000000.0) return -1000000.0;
	return curClutchTorque;
}

void BeamEngine::setSpin(float rpm)
{
	curGearboxRPM=rpm;
}

//for hydros acceleration
float BeamEngine::getCrankFactor()
{
	return 1.0f+4.0f*(curEngineRPM-800.0f)/(maxRPM-800.0f);
}

void BeamEngine::setClutch(float clutch)
{
	curClutch=clutch;
}

float BeamEngine::getClutch()
{
	return curClutch;
}

void BeamEngine::toggleContact()
{
	contact=!contact;
	if (contact) ssm->trigStart(trucknum, SS_TRIG_IGNITION);
	else ssm->trigStop(trucknum, SS_TRIG_IGNITION);
}

//quick start
void BeamEngine::start()
{
	curGear=1;
	curClutch=0;
	autoselect=DRIVE;
	curEngineRPM=750.0f;
	curGearboxRPM=750.0f;
	curClutchTorque=0.0f;
	curTurboRPM=0.0f;
	apressure=0.0f;
	running=1;
	contact=1;
	ssm->trigStart(trucknum, SS_TRIG_IGNITION);
	setAcc(0);
	ssm->trigStart(trucknum, SS_TRIG_ENGINE);
}

void BeamEngine::offstart()
{
	curGear=0;
	curClutch=0;
	autoselect=NEUTRAL;
	curEngineRPM=0.0f;
	running=0;
	contact=0;
	ssm->trigStop(trucknum, SS_TRIG_IGNITION);
//		setAcc(0);
	ssm->trigStop(trucknum, SS_TRIG_ENGINE);
}

void BeamEngine::setstarter(int v)
{
	starter=v;
	if (v && curEngineRPM<750.0) setAcc(1.0f);
}

//low level gear changing
int BeamEngine::getGear() {return curGear;}
void BeamEngine::setGear(int v) {curGear=v;}

//stalling engine
void BeamEngine::stop()
{
	if (!running) return;
//		curGear=0;
//		curClutch=0;
	running=0;
	ssm->trigStop(trucknum, SS_TRIG_ENGINE);
}


//high level controls
void BeamEngine::autoSetAcc(float val)
{
	autocurAcc=val;
	if (!shifting) setAcc(val);
}


void BeamEngine::shift(int val)
{
	if (automode!=MANUAL)
	{
		ssm->trigStart(trucknum, SS_TRIG_SHIFT);
		shiftval=val;
		shifting=1;
		shiftclock=0.0;
		setAcc(0.0);
	}
	else
	{
		if (curClutch>0.25)
			{ssm->trigOnce(trucknum, SS_TRIG_GEARSLIDE);}
		else
		{
			ssm->trigOnce(trucknum, SS_TRIG_SHIFT);
			curGear+=val;
			if (curGear<-1) curGear=-1;
			if (curGear>numGears) curGear=numGears;
		}
	}
}

void BeamEngine::shiftTo(int val)
{
	if (automode!=MANUAL)
	{
		shiftval=val-curGear;
		shifting=1;
		shiftclock=0.0;
		setAcc(0.0);
	}
	else
	{
		if (curClutch>0.25)
			{ssm->trigOnce(trucknum, SS_TRIG_GEARSLIDE);}
		else
		{
			ssm->trigOnce(trucknum, SS_TRIG_SHIFT);
			curGear=val;
			if (curGear<-1) curGear=-1;
			if (curGear>numGears) curGear=numGears;
		}
	}
}

void BeamEngine::updateShifts()
{
/*		if (autoselect==REAR) shiftTo(-1);
	if (autoselect==NEUTRAL) shiftTo(0);
	if (autoselect==DRIVE) shiftTo(1);
	if (autoselect==TWO) shiftTo(2);
	if (autoselect==ONE) shiftTo(1);
*/
	ssm->trigOnce(trucknum, SS_TRIG_SHIFT);
	if (autoselect==REAR) curGear=-1;
	if (autoselect==NEUTRAL) curGear=0;
	if (autoselect==DRIVE && curGear <= 0) curGear=1;
	if (autoselect==TWO) curGear=2;
	if (autoselect==ONE) curGear=1;
}

void BeamEngine::autoShiftUp()
{
	if (autoselect!=REAR) 
	{
		autoselect=(autoswitch)(autoselect-1);
		updateShifts();
	}
}

void BeamEngine::autoShiftDown()
{
	if (autoselect!=ONE) 
	{
		autoselect=(autoswitch)(autoselect+1);
		updateShifts();
	}
}

int BeamEngine::getAutoShift()
{
	return (int)autoselect;
}

void BeamEngine::setManualClutch(float val)
{
	if (automode==MANUAL)
	{
		if (val<0) val=0;
		curClutch=1.0-val;
	}
}

BeamEngine::~BeamEngine()
{
	// TODO: fix crash upon delete:
	//if(gearsRatio) free(gearsRatio); gearsRatio=0;
	
	if(torqueCurve) delete torqueCurve; torqueCurve=0;
}

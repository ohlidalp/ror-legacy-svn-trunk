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

#include "Character.h"

#include "water.h"
#include "collisions.h"
#include "heightfinder.h"
#include "MapControl.h"
#include "InputEngine.h"

using namespace Ogre;

unsigned int Character::characterCounter=0;

Character::Character(Collisions *c, HeightFinder *h, Water *w, MapControl *m, Ogre::SceneManager *scm)
{
	this->collisions=c;
	this->hfinder=h;
	this->water=w;
	this->map=m;
	this->scm=scm;
	
	myNumber = characterCounter++;
	myName = "character"+Ogre::StringConverter::toString(myNumber);
	persoangle=0;
	persospeed=2;
	persovspeed=0;
	perso_canjump=false;
	lastpersopos=Vector3::ZERO;
	personode=0;
	persoanim=0;

	Entity *ent = scm->createEntity(myName+"_mesh", "character.mesh");
	ent->setNormaliseNormals(true);
	// Add entity to the scene node
	personode=scm->getRootSceneNode()->createChildSceneNode();
	personode->attachObject(ent);
	personode->setScale(0.02,0.02,0.02);
	persoanim=ent->getAllAnimationStates();
	
	if(map)
	{
		MapEntity *e = map->createNamedMapEntity(myName, "person");
		e->setState(0);
		e->setVisibility(true);
		e->setPosition(personode->getPosition());
		e->setRotation(personode->getOrientation());
	}
	
}

void Character::setPosition(Ogre::Vector3 pos)
{
	personode->setPosition(pos);
	if(map)
		map->getEntityByName(myName)->setPosition(pos);
}

Vector3 Character::getPosition()
{
	return personode->getPosition();
}

Quaternion Character::getOrientation()
{
	return personode->getOrientation();
}
void Character::setVisible(bool v)
{
	personode->setVisible(v);
	if(map)
		map->getEntityByName(myName)->setVisibility(v);
}

float Character::getAngle()
{
	return persoangle;
}


void Character::setAnimationMode(Ogre::String mode, float time)
{
	if(!persoanim)
		return;
	static String lastmode = "";
	if(lastmode != mode)
	{
		AnimationStateIterator it = persoanim->getAnimationStateIterator();
		while(it.hasMoreElements())
		{
			AnimationState *as = it.getNext();
			if(as->getAnimationName() == mode)
			{
				as->setEnabled(true);
				as->setWeight(1);
				as->addTime(time);
			}
			else
			{
				as->setEnabled(false);
				as->setWeight(0);
			}
		}
		lastmode = mode;
	} else
	{
		persoanim->getAnimationState(mode)->addTime(time);
	}
}

void Character::update(float dt)
{
	//mode perso
	Vector3 position=personode->getPosition();
	//gravity force is always on
	position.y+=persovspeed*dt;
	persovspeed+=dt*-9.8;
	bool idleanim=true;
	//object contact
	Vector3 position2=position+Vector3(0, 0.075, 0);
	Vector3 position3=position+Vector3(0, 0.25, 0);
	if (collisions->collisionCorrect(&position))
	{
		if (persovspeed<0) persovspeed=0.0;
		if (collisions->collisionCorrect(&position2) && !collisions->collisionCorrect(&position3)) persovspeed=2.0; //autojump
		else perso_canjump=true;
	}
	else
	{
		//double check
		if ((position-lastpersopos).length()<5.0)
		{
			int numstep=100;
			Vector3 dvec=(position-lastpersopos);
			Vector3 iposition=lastpersopos;
			Vector3 cposition;
			for (int i=0; i<numstep; i++)
			{
				cposition=iposition+dvec*((float)(i+1)/numstep);
				//Vector3 cposition2=cposition+Vector3(0, 0.075, 0);
				if (collisions->collisionCorrect(&cposition))
				{
					position=cposition;
					if (persovspeed<0) persovspeed=0.0;
					perso_canjump=true;
					/*if (collisions->collisionCorrect(&cposition2))
					{
					position=cposition2-Vector3(0, 0.075, 0);
					}*/
					break;
				}
			}
		}

	}
	lastpersopos=position;

	//ground contact
	float pheight = hfinder->getHeightAt(position.x,position.z);
	float wheight = -99999;
	if (position.y<pheight)
	{
		position.y=pheight;
		persovspeed=0.0;
		perso_canjump=true;
	}
	//water stuff
	bool isswimming=false;
	if (water)
	{
		wheight=water->getHeightWaves(position);
		if (position.y<wheight-1.8) 
		{
			position.y=wheight-1.8;
			persovspeed=0.0;
		};
	}

	// 0.1 due to 'jumping' from waves -> not nice looking
	if(water && wheight - pheight > 1.8 && position.y + 0.1 <= wheight)
		isswimming=true;


	float tmpJoy = 0;
	if(perso_canjump)
	{
		if (INPUTENGINE.getEventBoolValue(EV_CHARACTER_JUMP))
		{
			persovspeed = 2.0;
			perso_canjump=false;
		}
	}

	tmpJoy = INPUTENGINE.getEventValue(EV_CHARACTER_RIGHT);
	if (tmpJoy > 0.0)
	{
		persoangle += dt * 2.0 * tmpJoy;
		personode->resetOrientation();
		personode->yaw(-Radian(persoangle));
		if(!isswimming)
		{
			setAnimationMode("Turn", -dt);
			idleanim=false;
		}
	}

	tmpJoy = INPUTENGINE.getEventValue(EV_CHARACTER_LEFT);
	if (tmpJoy > 0.0)
	{
		persoangle -= dt * 2.0 * tmpJoy;
		personode->resetOrientation();
		personode->yaw(-Radian(persoangle));
		if(!isswimming)
		{
			setAnimationMode("Turn", dt);
			idleanim=false;
		}
	}

	tmpJoy = INPUTENGINE.getEventValue(EV_CHARACTER_SIDESTEP_LEFT);
	if (tmpJoy > 0.0)
	{
		// animation missing for that
		position+=dt*persospeed*1.5*tmpJoy*Vector3(cos(persoangle-Math::PI/2), 0.0, sin(persoangle-Math::PI/2));
	}

	tmpJoy = INPUTENGINE.getEventValue(EV_CHARACTER_SIDESTEP_RIGHT);
	if (tmpJoy > 0.0)
	{
		// animation missing for that
		position+=dt*persospeed*1.5*tmpJoy*Vector3(cos(persoangle+Math::PI/2), 0.0, sin(persoangle+Math::PI/2));
	}

	tmpJoy = INPUTENGINE.getEventValue(EV_CHARACTER_FORWARD) + INPUTENGINE.getEventValue(EV_CHARACTER_ROT_UP);
	if(tmpJoy>1) tmpJoy = 1;
	float tmpRun = INPUTENGINE.getEventValue(EV_CHARACTER_RUN);
	float tmpBack  = INPUTENGINE.getEventValue(EV_CHARACTER_BACKWARDS) + INPUTENGINE.getEventValue(EV_CHARACTER_ROT_DOWN);
	if(tmpBack>1) tmpBack = 1;
	if (tmpJoy > 0.0 || tmpRun > 0.0)
	{
		float accel = 1.0 * tmpJoy;
		float time = dt*accel*persospeed;
		if (tmpRun > 0) 
			accel = 3.0 * tmpRun;
		if(isswimming)
		{
			setAnimationMode("Swim_loop", time);
			idleanim=false;
		} else
		{
			if (tmpRun > 0) 
			{
				setAnimationMode("Run", time);
				idleanim=false;
			} else
			{
				setAnimationMode("Walk", time);
				idleanim=false;
			}
		}
		// 0.005f fixes character getting stuck on meshes
		position+=dt*persospeed*1.5*accel*Vector3(cos(persoangle), 0.01f, sin(persoangle));
	} else if (tmpBack > 0.0)
	{	
		float time = -dt*persospeed;
		if(isswimming)
		{
			setAnimationMode("Spot_swim", time);
			idleanim=false;
		} else
		{
			setAnimationMode("Walk", time);
			idleanim=false;
		}
		// 0.005f fixes character getting stuck on meshes
		position-=dt*persospeed*tmpBack*Vector3(cos(persoangle), 0.01f, sin(persoangle));
	}

	if(idleanim)
	{
		if(isswimming)
			setAnimationMode("Spot_swim", dt * 0.5);
		else
			setAnimationMode("Idle_sway", dt * 0.05);
	}
	
	/*
	//object contact
	int numstep=100;
	Vector3 dvec=(position-personode->getPosition());
	Vector3 iposition=personode->getPosition();
	for (int i=0; i<numstep; i++)
	{
	position=iposition+dvec*((float)(i+1)/numstep);
	Vector3 position2=position+Vector3(0, 0.01, 0);
	Vector3 position3=position+Vector3(0, 0.25, 0);
	Vector3 rposition=position;
	if (collisions->collisionCorrect(&position) || collisions->collisionCorrect(&position2))
	{
	if (persovspeed<0) persovspeed=0.0;
	Vector3 corr=rposition-position; corr.y=0;
	if (corr.squaredLength()>0 && !collisions->collisionCorrect(&position3)) persovspeed=2.0; //autojump
	perso_canjump=true;
	break;
	}
	}
	*/
	personode->setPosition(position);
	updateMapIcon();
}

void Character::updateMapIcon()
{
	if(map)
	{
		map->getEntityByName(myName)->setPosition(personode->getPosition());
		map->getEntityByName(myName)->setRotation(personode->getOrientation());
	}
}

void Character::move(Ogre::Vector3 v)
{
	personode->translate(v);
}

Ogre::SceneNode *Character::getSceneNode()
{
	return personode;
}

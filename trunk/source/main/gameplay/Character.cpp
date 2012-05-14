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

#include "Character.h"

#include "water.h"
#include "collisions.h"
#include "heightfinder.h"
#include "MapControl.h"
#include "MapEntity.h"
#include "InputEngine.h"
#include "network.h"
#include "NetworkStreamManager.h"
#include "BeamFactory.h"
#include "RoRFrameListener.h"
#include "ColoredTextAreaOverlayElement.h"
#include "PlayerColours.h"

#include "utils.h"

using namespace Ogre;

unsigned int Character::characterCounter=0;


Character::Character(Camera *cam,Collisions *c, Network *net, HeightFinder *h, Water *w, MapControl *m, Ogre::SceneManager *scm, int source, unsigned int streamid, int colourNumber, bool remote) : personode(0)
{
	this->mCamera=cam;
	this->net=net;
	this->collisions=c;
	this->hfinder=h;
	this->water=w;
	this->map=m;
	this->scm=scm;
	this->source=source;
	this->streamid=streamid;
	this->colourNumber=colourNumber;
	this->remote=remote;
	this->physicsEnabled=true;
	beamCoupling=0;
	//remote = true;
	last_net_time=0;
	netMT=0;
	networkUsername = String();
	networkAuthLevel = 0;


	myNumber = characterCounter++;
	myName = "character"+TOSTRING(myNumber);
	persoangle=0;
	persospeed=2;
	persovspeed=0;
	perso_canjump=false;
	lastpersopos=Vector3::ZERO;
	personode=0;
	persoanim=0;

	Entity *ent = scm->createEntity(myName+"_mesh", "character.mesh");
#if OGRE_VERSION<0x010602
	ent->setNormaliseNormals(true);
#endif //OGRE_VERSION

	// fix disappearing mesh
	Ogre::AxisAlignedBox aabb;
	aabb.setInfinite();
	ent->getMesh()->_setBounds(aabb);

	// Add entity to the scene node
	personode=scm->getRootSceneNode()->createChildSceneNode();
	personode->attachObject(ent);
	personode->setScale(0.02,0.02,0.02);
	persoanim=ent->getAllAnimationStates();

#ifdef USE_MYGUI
	if(map)
	{
		mapEnt = map->createNamedMapEntity(myName, "person");
		mapEnt->setState(0);
		mapEnt->setVisibility(true);
		mapEnt->setPosition(personode->getPosition());
		mapEnt->setRotation(personode->getOrientation());
	}
#endif //MYGUI

	if(net)
		sendStreamSetup();

	if(remote)
		setVisible(true);

	// setup colour
	MaterialPtr mat = MaterialManager::getSingleton().getByName("tracks/character");
	MaterialPtr mat2 = mat->clone("tracks/"+myName);
	ent->setMaterialName("tracks/"+myName);

	updateNetLabel();
}

Character::~Character()
{
	setVisible(false);
	if(netMT) delete netMT;
	if(personode) personode->detachAllObjects();
#ifdef USE_MYGUI
	if(map && mapEnt) map->deleteMapEntity(mapEnt);
#endif //MYGUI
	// try to unload some materials
	try
	{
		MaterialManager::getSingleton().unload("tracks/"+myName);
	} catch(...)
	{
	}
}

void Character::updateCharacterColour()
{
	String matName = "tracks/" + myName;
	PlayerColours::getSingleton().updateMaterial(colourNumber, matName, 2);
}

void Character::setUID(int uid)
{
	this->source = uid;
}

void Character::updateNetLabel()
{
#ifdef USE_SOCKETW
	// label above head
	if(!net) return;
	if(remote)
	{
		client_t *info = net->getClientInfo(this->source);
		if(!info) return;
		if(tryConvertUTF(info->user.username).empty()) return;
		this->colourNumber = info->user.colournum;
		networkUsername = tryConvertUTF(info->user.username);
		networkAuthLevel = info->user.authstatus;
	} else
	{
		user_info_t *info = net->getLocalUserData();
		if(!info) return;
		if(String(info->username).empty()) return;
		this->colourNumber = info->colournum;
		networkUsername = tryConvertUTF(info->username);
		networkAuthLevel = info->authstatus;
	}

	//LOG(" * updateNetLabel : " + TOSTRING(this->source));
	if(!netMT)
	{
		netMT = new MovableText("netlabel-"+myName, networkUsername);
		personode->attachObject(netMT);
		netMT->setFontName("CyberbitEnglish");
		netMT->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
		netMT->setAdditionalHeight(2);
		netMT->showOnTop(false);
		netMT->setCharacterHeight(8);
		netMT->setColor(ColourValue::Black);
	}

	//LOG(" *label caption: " + String(networkUsername));
	try
	{
		netMT->setCaption(networkUsername);
	}
	catch (...)
	{
	}
	/*
	if(networkAuthLevel & AUTH_ADMIN)
	{
		netMT->setFontName("highcontrast_red");
	} else if(networkAuthLevel & AUTH_RANKED)
	{
		netMT->setFontName("highcontrast_green");
	} else
	{
		netMT->setFontName("highcontrast_black");
	}
	*/

	// update character colour
	updateCharacterColour();
#endif //SOCKETW
}

void Character::setPosition(Ogre::Vector3 pos)
{
	personode->setPosition(pos);
#ifdef USE_MYGUI
	if(map)
		map->getEntityByName(myName)->setPosition(pos);
#endif //MYGUI
}

Vector3 Character::getPosition()
{
	return personode->getPosition();
}

Quaternion Character::getOrientation()
{
	return personode->getOrientation();
}

void Character::setOrientation(Quaternion rot)
{
	return personode->setOrientation(rot);
}

void Character::setVisible(bool visible)
{
	personode->setVisible(visible);
#ifdef USE_MYGUI
	// CRASH BELOW
	//if(map)
	//	map->getEntityByName(myName)->setVisibility(visible);
#endif //MYGUI
}

bool Character::getVisible()
{
	return personode->getAttachedObject(0)->getVisible();
}

void Character::setAngle(float angle)
{
	persoangle = angle;
	personode->resetOrientation();
	personode->yaw(-Radian(persoangle));
}

void Character::setCollisions(Collisions *c)
{
	this->collisions = c;
}

void Character::setHFinder(HeightFinder *h)
{
	this->hfinder = h;
}

void Character::setWater(Water *w)
{
	this->water = w;
}

void Character::setAnimationMode(Ogre::String mode, float time)
{
	if(!persoanim)
		return;
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
	if(physicsEnabled && !remote)
	{
		// disable character movement when using the free camera mode or when chatting
		// OLD!
		//if(RoRFrameListener::eflsingleton && (RoRFrameListener::eflsingleton->cameramode == CAMERA_FREE || RoRFrameListener::eflsingleton->isUserChatting())) return;

		// small hack: if not visible do not apply physics
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
			if (persovspeed<0) persovspeed=0.0f;
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
						if (persovspeed<0) persovspeed=0.0f;
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
			persovspeed=0.0f;
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
				persovspeed=0.0f;
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
		if (tmpJoy > 0.0f)
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
		if (tmpJoy > 0.0f)
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
				
		float tmpRun = INPUTENGINE.getEventValue(EV_CHARACTER_RUN);
		float accel = 1.0f;

		tmpJoy = accel = INPUTENGINE.getEventValue(EV_CHARACTER_SIDESTEP_LEFT);
		if (tmpJoy > 0.0f)
		{
			if (tmpRun > 0.0f) accel = 3.0 * tmpRun;
			// animation missing for that
			position+=dt*persospeed*1.5*accel*Vector3(cos(persoangle-Math::PI/2), 0.0f, sin(persoangle-Math::PI/2));
		}

		tmpJoy = accel = INPUTENGINE.getEventValue(EV_CHARACTER_SIDESTEP_RIGHT);
		if (tmpJoy > 0.0f)
		{
			if (tmpRun > 0.0f) accel = 3.0 * tmpRun;
			// animation missing for that
			position+=dt*persospeed*1.5*accel*Vector3(cos(persoangle+Math::PI/2), 0.0f, sin(persoangle+Math::PI/2));
		}

		tmpJoy = accel = INPUTENGINE.getEventValue(EV_CHARACTER_FORWARD) + INPUTENGINE.getEventValue(EV_CHARACTER_ROT_UP);
		float tmpBack  = INPUTENGINE.getEventValue(EV_CHARACTER_BACKWARDS) + INPUTENGINE.getEventValue(EV_CHARACTER_ROT_DOWN);
		
		tmpJoy  = std::min(tmpJoy, 1.0f);
		tmpBack = std::min(tmpBack, 1.0f);

		if (tmpJoy > 0.0f || tmpRun > 0.0f)
		{
			if (tmpRun > 0.0f) accel = 3.0 * tmpRun;
			
			float time = dt*tmpJoy*persospeed;

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
		} else if (tmpBack > 0.0f)
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
		if (persovspeed<0) persovspeed=0.0f;
		Vector3 corr=rposition-position; corr.y=0;
		if (corr.squaredLength()>0 && !collisions->collisionCorrect(&position3)) persovspeed=2.0; //autojump
		perso_canjump=true;
		break;
		}
		}
		*/
		personode->setPosition(position);
		updateMapIcon();
	} else if(beamCoupling)
	{
		// if physics is disabled, its attached to a beam truck maybe?
		Vector3 pos;
		Quaternion rot;
		float angle = beamCoupling->hydrodirwheeldisplay * -1.0f; // not getSteeringAngle(), but this, as its smoothed
		int res = beamCoupling->calculateDriverPos(pos, rot);
		if(!res)
		{
			setPosition(pos + rot * Vector3(0.0f,-0.6f,-0.1f)); // hack to position the character right perfect on the default seat
			setOrientation(rot);
			setAnimationMode("driving");
			Real lenght = persoanim->getAnimationState("driving")->getLength();
			float timePos = ((angle + 1.0f) * 0.5f) * lenght;
			//LOG("angle: " + TOSTRING(angle) + " / " + TOSTRING(timePos));
			//prevent animation flickering on the borders:
			if(timePos < 0.01f) timePos = 0.01f;
			if(timePos > lenght - 0.01f) timePos = lenght - 0.01f;
			persoanim->getAnimationState("driving")->setTimePosition(timePos);
		}

	}

#ifdef USE_SOCKETW
	if(net && !remote)
		sendStreamData();
#endif // USE_SOCKETW
}

void Character::updateMapIcon()
{
#ifdef USE_MYGUI
	if(map)
	{
		map->getEntityByName(myName)->setPosition(personode->getPosition());
		map->getEntityByName(myName)->setRotation(personode->getOrientation());
	}
#endif //MYGUI
}

void Character::move(Ogre::Vector3 v)
{
	personode->translate(v);
}

void Character::sendStreamSetup()
{
	if(remote) return;
	// new local stream
	stream_register_t reg;
	memset(&reg, 0, sizeof(reg));
	reg.status = 1;
	strcpy(reg.name, "default");
	reg.type = 1;
	reg.data[0] = 2;

	NetworkStreamManager::getSingleton().addLocalStream(this, &reg);
}

void Character::sendStreamData()
{
	int t = netTimer.getMilliseconds();
	if (t-last_net_time < 100)
		return;

	// do not send position data if coupled to a truck already
	if(beamCoupling) return;

	last_net_time = t;

	pos_netdata_t data;
	data.command = CHARCMD_POSITION;
	data.posx = personode->getPosition().x;
	data.posy = personode->getPosition().y;
	data.posz = personode->getPosition().z;
	data.rotx = personode->getOrientation().x;
	data.roty = personode->getOrientation().y;
	data.rotz = personode->getOrientation().z;
	data.rotw = personode->getOrientation().w;
	strncpy(data.animationMode, lastmode.c_str(), 254);
	data.animationTime = persoanim->getAnimationState(lastmode)->getTimePosition();

	//LOG("sending character stream data: " + TOSTRING(net->getUserID()) + ":"+ TOSTRING(streamid));
	this->addPacket(MSG2_STREAM_DATA, sizeof(pos_netdata_t), (char*)&data);
}

void Character::receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len)
{
	if(type == MSG2_STREAM_DATA && this->source == source && this->streamid == streamid)
	{
		header_netdata_t *header = (header_netdata_t *)buffer;
		if(header->command == CHARCMD_POSITION)
		{
			// position
			pos_netdata_t *data = (pos_netdata_t *)buffer;
			Vector3 pos(data->posx, data->posy, data->posz);
			this->setPosition(pos);
			Quaternion rot(data->rotw, data->rotx, data->roty, data->rotz);
			this->setOrientation(rot);
			setAnimationMode(getASCIIFromCharString(data->animationMode, 255), data->animationTime);
		} else if (header->command == CHARCMD_ATTACH)
		{
			// attach
			attach_netdata_t *data = (attach_netdata_t *)buffer;
			if(data->enabled)
			{
				Beam *beam = BeamFactory::getSingleton().getBeam(data->source_id, data->stream_id);
				setBeamCoupling(true, beam);
			} else
			{
				setBeamCoupling(false);
			}
		}
	}
	//else
	//	LOG("character stream data wrong: " + TOSTRING(source) + ":"+ TOSTRING(streamid));
}

void Character::updateNetLabelSize()
{
	if (!this || !net || !netMT) return;

	netMT->setVisible(getVisible());

	if(!netMT->isVisible()) return;

	Vector3 vdir = personode->getPosition() - mCamera->getPosition();
	float vlen=vdir.length();
	float h = vlen * 1.2f;
	if(h<9)
		h=9;
	netMT->setCharacterHeight(h);
	if(vlen>1000)
		netMT->setCaption(networkUsername + "  (" + TOSTRING( (float)(ceil(vlen/100)/10.0) )+ " km)");
	else if (vlen>20 && vlen <= 1000)
		netMT->setCaption(networkUsername + "  (" + TOSTRING((int)vlen)+ " m)");
	else
		netMT->setCaption(networkUsername);

	//netMT->setAdditionalHeight((maxy-miny)+h+0.1);
	//netMT->setVisible(true);
}

int Character::setBeamCoupling(bool enabled, Beam *truck)
{
	if(enabled)
	{
		if(!truck) return 1;
		beamCoupling = truck;
		setPhysicsEnabled(false);
		if(netMT && netMT->isVisible()) netMT->setVisible(false);
		if(net && !remote)
		{
			attach_netdata_t data;
			data.command = CHARCMD_ATTACH;
			data.enabled = true;
			data.source_id = beamCoupling->getSourceID();
			data.stream_id = beamCoupling->getStreamID();
			this->addPacket(MSG2_STREAM_DATA, sizeof(attach_netdata_t), (char*)&data);
		}

		// do not cast shadows inside of a truck
		personode->getAttachedObject(0)->setCastShadows(false);

		// check if there is a seat, if not, hide our character
		Vector3 pos;
		Quaternion rot;
		int res = beamCoupling->calculateDriverPos(pos, rot);
		if(res)
		{
			// driver seat not found
			setVisible(false);
		}

	} else
	{
		setPhysicsEnabled(true);
		beamCoupling=0;
		if(netMT && !netMT->isVisible()) netMT->setVisible(true);
		if(net && !remote)
		{
			attach_netdata_t data;
			data.command = CHARCMD_ATTACH;
			data.enabled = false;
			this->addPacket(MSG2_STREAM_DATA, sizeof(attach_netdata_t), (char*)&data);
		}

		// show character
		setVisible(true);

		personode->resetOrientation();

		// cast shadows when using it on the outside
		personode->getAttachedObject(0)->setCastShadows(true);
	}
	return 0;
}


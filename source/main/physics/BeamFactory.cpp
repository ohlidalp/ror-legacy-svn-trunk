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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 24th of August 2009

#include "BeamFactory.h"

#include <Ogre.h>
#include "network.h"
#include "utils.h"
#include "sha1.h"
#include "pthread.h"
#include "RoRFrameListener.h"
#include "collisions.h"
#include "Settings.h"
#include "engine.h"

#ifdef USE_MYGUI
#include "gui_mp.h"
#endif  // USE_MYGUI

using namespace Ogre;


template<> BeamFactory *StreamableFactory < BeamFactory, Beam >::ms_Singleton = 0;

BeamFactory::BeamFactory(SceneManager *manager, SceneNode *parent, RenderWindow* win, Network *net, float *mapsizex, float *mapsizez, Collisions *icollisions, HeightFinder *mfinder, Water *_w, Camera *pcam) : 
	  manager(manager)
	, parent(parent)
	, win(win)
	, net(net)
	, mapsizex(mapsizex)
	, mapsizez(mapsizez)
	, icollisions(icollisions)
	, mfinder(mfinder)
	, w(_w)
	, pcam(pcam)
	, thread_mode(THREAD_MONO)
	//, trucks(0)
	, free_truck(0)
	, current_truck(-1)
	, done_count(0)
{
	pthread_mutex_init(&done_count_mutex, NULL);
	pthread_cond_init(&done_count_cv, NULL);

	for (int i = 0; i < MAX_TRUCKS; i++)
		trucks[i] = 0;

	if (SSETTING("Threads")=="1 (Standard CPU)")                        thread_mode = THREAD_MONO;
	if (SSETTING("Threads")=="2 (Hyper-Threading or Dual core CPU)")    thread_mode = THREAD_HT;
	if (SSETTING("Threads")=="3 (multi core CPU, one thread per beam)") thread_mode = THREAD_HT2;

}

Beam *BeamFactory::createLocal(int slotid)
{
	// do not use this ...
	return 0;
}

int BeamFactory::removeBeam(Beam *b)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			if(it2->second  == b)
			{
				NetworkStreamManager::getSingleton().removeStream(it1->first, it2->first);
				_deleteTruck(it2->second);
				it1->second.erase(it2);
				unlockStreams();
				return 0;
			}
		}
	}
	unlockStreams();
	return 1;
}

Beam *BeamFactory::createLocal(Ogre::Vector3 pos, Ogre::Quaternion rot, Ogre::String fname, collision_box_t *spawnbox, bool ismachine, int flareMode, std::vector<Ogre::String> *truckconfig, Skin *skin, bool freePosition)
{
	bool networked=false, networking=false;
	if(net) networking = true;

	int truck_num = getFreeTruckSlot();
	if(truck_num == -1)
	{
		LOG("ERROR: could not add beam to main list");
		return 0;
	}

	Beam *b = new Beam(truck_num,
		manager,
		manager->getRootSceneNode()->createChildSceneNode(),
		win,
		net,
		mapsizex,
		mapsizez,
		pos.x,
		pos.y,
		pos.z,
		rot,
		fname.c_str(),
		icollisions,
		mfinder,
		w,
		pcam,
		networked,
		networking,
		spawnbox,
		ismachine,
		flareMode,
		truckconfig,
		skin,
		freePosition);

	trucks[truck_num] = b;

	// lock slidenodes after spawning the truck?
	if(b->getSlideNodesLockInstant())
		b->toggleSlideNodeLock();

	lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	streamables[-1][10 + truck_num] = b; // 10 streams offset for beam constructions
	unlockStreams();

	return b;
}

Beam *BeamFactory::createRemoteInstance(stream_reg_t *reg)
{
	// NO LOCKS IN HERE, already locked

	stream_register_trucks_t *treg = (stream_register_trucks_t *)&reg->reg;

	LOG(" new beam truck for " + TOSTRING(reg->sourceid) + ":" + TOSTRING(reg->streamid));

	bool networked=true, networking=false;
	if(net) networking = true;

	// check if we got this truck installed
	String filename = String(treg->name);
	String group = "";
	if(!CACHE.checkResourceLoaded(filename, group))
	{
		LOG("wont add remote stream (truck not existing): '"+filename+"'");

		// add 0 to the map so we know its stream is existing but not usable for us
		// already locked
		//lockStreams();
		std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
		streamables[reg->sourceid][reg->streamid] = 0;
		//unlockStreams();

		return 0;
	}

	// fill truckconfig
	std::vector<Ogre::String> truckconfig;
	for(int i = 0; i < 10; i++)
	{
		if(!strnlen(treg->truckconfig[i], 60))
			break;
		truckconfig.push_back(String(treg->truckconfig[i]));
	}


	// DO NOT spawn the truck far off anywhere
	// the truck parsing will break flexbodies initialization when using huge numbers here
	Vector3 pos = Vector3::ZERO;

	int truck_num = getFreeTruckSlot();
	if(truck_num == -1)
	{
		LOG("ERROR: could not add beam to main list");
		return 0;
	}

	Beam *b = new Beam(truck_num,
		manager,
		manager->getRootSceneNode(),
		win,
		net,
		mapsizex,
		mapsizez,
		pos.x,
		pos.y,
		pos.z,
		Quaternion::ZERO,
		reg->reg.name,
		icollisions,
		mfinder,
		w,
		pcam,
		networked,
		networking,
		0,
		false,
		0,
		&truckconfig,
		0);

	trucks[truck_num] = b;

	b->setSourceID(reg->sourceid);
	b->setStreamID(reg->streamid);

	// already locked
	//lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	streamables[reg->sourceid][reg->streamid] = b;
	//unlockStreams();

	b->updateNetworkInfo();

	return b;
}

void BeamFactory::localUserAttributesChanged(int newid)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	if(streamables.find(-1) == streamables.end())
	{
		unlockStreams();
		return;
	}

	Beam *b = streamables[-1][0];
	streamables[newid][0] = streamables[-1][0]; // add alias :)
	//b->setUID(newid);
	//b->updateNetLabel();
	unlockStreams();
}

void BeamFactory::netUserAttributesChanged(int source, int streamid)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			if(it1->first == source)
			{
				Beam *b = it2->second;
				if(b) b->updateNetworkInfo();
				break;
			}
		}
	}
	unlockStreams();
}

Beam *BeamFactory::getBeam(int source, int streamid)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			if(it1->first == source)
			{
				unlockStreams();
				return it2->second;
			}
		}
	}
	unlockStreams();
	return 0;
}

bool BeamFactory::syncRemoteStreams()
{
	//block until all threads done
	if (thread_mode==THREAD_HT)
	{
		MUTEX_LOCK(&done_count_mutex);
		while (done_count>0)
			pthread_cond_wait(&done_count_cv, &done_count_mutex);
		MUTEX_UNLOCK(&done_count_mutex);
	}

	// we override this here, so we know if something changed and could update the player list
	// we delete and add trucks in there, so be sure that nothing runs as we delete them ...
	bool changes = StreamableFactory <BeamFactory, Beam>::syncRemoteStreams();
	
	if(changes)
		updateGUI();

	return changes;
}

void BeamFactory::updateGUI()
{
#ifdef USE_MYGUI
#ifdef USE_SOCKETW
	GUI_Multiplayer::getSingleton().update();
#endif // USE_SOCKETW
#endif // USE_MYGUI	
}

//j is the index of a MAYSLEEP truck, returns true if one active was found in the set
bool BeamFactory::checkForActive(int j, bool *sleepyList)
{
	int i;
	sleepyList[j]=true;
	for (i=0; i<free_truck; i++)
	{
		if(!trucks[i]) continue;
		if ( !sleepyList[i] &&
			((trucks[j]->minx<trucks[i]->minx && trucks[i]->minx<trucks[j]->maxx) || (trucks[j]->minx<trucks[i]->maxx && trucks[i]->maxx<trucks[j]->maxx) || (trucks[i]->minx<trucks[j]->maxx && trucks[j]->maxx<trucks[i]->maxx)) &&
			((trucks[j]->miny<trucks[i]->miny && trucks[i]->miny<trucks[j]->maxy) || (trucks[j]->miny<trucks[i]->maxy && trucks[i]->maxy<trucks[j]->maxy) || (trucks[i]->miny<trucks[j]->maxy && trucks[j]->maxy<trucks[i]->maxy)) &&
			((trucks[j]->minz<trucks[i]->minz && trucks[i]->minz<trucks[j]->maxz) || (trucks[j]->minz<trucks[i]->maxz && trucks[i]->maxz<trucks[j]->maxz) || (trucks[i]->minz<trucks[j]->maxz && trucks[j]->maxz<trucks[i]->maxz))
			)
		{
			if (trucks[i]->state==MAYSLEEP || (trucks[i]->state==DESACTIVATED && trucks[i]->sleepcount>=5))
			{
				if (checkForActive(i, sleepyList)) return true;
			}
			else return true;
		};
	}
	return false;
}


void BeamFactory::recursiveActivation(int j)
{
	int i;
	for (i=0; i<free_truck; i++)
	{
		if(!trucks[i]) continue;
		if ((trucks[i]->state==SLEEPING || trucks[i]->state==MAYSLEEP || trucks[i]->state==GOSLEEP ||(trucks[i]->state==DESACTIVATED && trucks[i]->sleepcount>=5)) &&
			((trucks[j]->minx<trucks[i]->minx && trucks[i]->minx<trucks[j]->maxx) || (trucks[j]->minx<trucks[i]->maxx && trucks[i]->maxx<trucks[j]->maxx) || (trucks[i]->minx<trucks[j]->maxx && trucks[j]->maxx<trucks[i]->maxx)) &&
			((trucks[j]->miny<trucks[i]->miny && trucks[i]->miny<trucks[j]->maxy) || (trucks[j]->miny<trucks[i]->maxy && trucks[i]->maxy<trucks[j]->maxy) || (trucks[i]->miny<trucks[j]->maxy && trucks[j]->maxy<trucks[i]->maxy)) &&
			((trucks[j]->minz<trucks[i]->minz && trucks[i]->minz<trucks[j]->maxz) || (trucks[j]->minz<trucks[i]->maxz && trucks[i]->maxz<trucks[j]->maxz) || (trucks[i]->minz<trucks[j]->maxz && trucks[j]->maxz<trucks[i]->maxz))
			)
		{
			trucks[i]->desactivate();//paradoxically, this activates the truck!
			trucks[i]->disableDrag = trucks[current_truck]->driveable==AIRPLANE;
			recursiveActivation(i);
		};
	}
}

void BeamFactory::checkSleepingState()
{
	if (current_truck!=-1)
	{
		trucks[current_truck]->disableDrag=false;
		recursiveActivation(current_truck);
		//if its grabbed, its moving
		//if (isnodegrabbed && trucks[truckgrabbed]->state==SLEEPING) trucks[truckgrabbed]->desactivate();
		//put to sleep
		for (int t=0; t<free_truck; t++)
		{
			if(!trucks[t]) continue;
			if (trucks[t]->state==MAYSLEEP)
			{
				bool sleepyList[MAX_TRUCKS];
				for (int i=0; i<MAX_TRUCKS; i++) sleepyList[i]=false;
				if (!checkForActive(t, sleepyList))
				{
					//no active truck in the set, put everybody to sleep
					for (int i=0; i<free_truck; i++)
					{
						if(!trucks[i]) continue;
						if (sleepyList[i])
							trucks[i]->state=GOSLEEP;
					}
				}
			}
		}

		//special stuff for rollable gear
		int t;
		bool rollmode=false;
		for (t=0; t<free_truck; t++)
		{
			if(!trucks[t]) continue;
			if (trucks[t]->state != SLEEPING)
				rollmode = rollmode || trucks[t]->wheel_contact_requested;
			
			trucks[t]->requires_wheel_contact = rollmode;// && !trucks[t]->wheel_contact_requested;
		}

	}
}

int BeamFactory::getFreeTruckSlot()
{
	// find a free slot for the truck
	for (int i=0; i<MAX_TRUCKS; i++)
	{
		if(trucks[i] == 0 && i >= free_truck) // XXX: TODO: remove this hack
		{
			// reuse slots
			if(i >= free_truck)
				free_truck = i + 1;
			return i;
		}
	}
	return -1;
}


void BeamFactory::activateAllTrucks()
{
	for (int i=0; i<free_truck; i++)
	{
		if(!trucks[i]) continue;
		if(trucks[i]->state==SLEEPING || trucks[i]->state==MAYSLEEP || trucks[i]->state==GOSLEEP || trucks[i]->state==DESACTIVATED)
		{
			trucks[i]->desactivate();//paradoxically, this activates the truck!
			trucks[i]->disableDrag = trucks[current_truck]->driveable==AIRPLANE;
			recursiveActivation(i);
		};
	}
}

void BeamFactory::sendAllTrucksSleeping()
{
	for (int i=0; i<free_truck; i++)
	{
		if(!trucks[i]) continue;
		if(trucks[i]->state==ACTIVATED)
		{
			trucks[i]->state=GOSLEEP;
		};
	}
}


void BeamFactory::recalcGravityMasses()
{
	// update the mass of all trucks
	for (int t=0; t < free_truck; t++)
	{
		if(!trucks[t]) continue;
		trucks[t]->recalc_masses();
	}
}

void BeamFactory::repairTruck(SoundScriptManager *ssm, Collisions *collisions, char* inst, char* box, bool keepPosition)
{
	//find the right truck (the one in the box)
	int t;
	int rtruck=-1;
	for (t=0; t<free_truck; t++)
	{
		if(!trucks[t]) continue;
		if (collisions->isInside(trucks[t]->nodes[0].AbsPosition, inst, box))
		{
			//we found one
			if (rtruck==-1) rtruck=t;
			else rtruck=-2; //too much trucks!
		}
	}
	if (rtruck>=0)
	{
		//take a position reference
#ifdef USE_OPENAL
		if(ssm) ssm->trigOnce(rtruck, SS_TRIG_REPAIR);
#endif //OPENAL
		Vector3 ipos=trucks[rtruck]->nodes[0].AbsPosition;
		trucks[rtruck]->reset();
		trucks[rtruck]->resetPosition(ipos.x, ipos.z, false);
		trucks[rtruck]->updateVisual();
	}
}

void BeamFactory::removeTruck(Collisions *collisions, char* inst, char* box)
{
	//find the right truck (the one in the box)
	int t;
	int rtruck=-1;
	for (t=0; t<free_truck; t++)
	{
		if(!trucks[t]) continue;
		if (collisions->isInside(trucks[t]->nodes[0].AbsPosition, inst, box))
		{
			//we found one
			if (rtruck==-1) rtruck=t;
			else rtruck=-2; //too much trucks!
		}
	}
	if (rtruck>=0)
	{
		// remove it
		removeTruck(rtruck);
	}
}

void BeamFactory::removeTruck(int truck)
{
	if(truck == -1 || truck > free_truck)
		// invalid number
		return;
	if(current_truck == truck)
		setCurrentTruck(-1);

	if(removeBeam(trucks[truck]))
	{
		// deletion over beamfactory failed, delete by hand
		// then delete the class
		_deleteTruck(trucks[truck], truck);
	}
}

void BeamFactory::_deleteTruck(Beam *b, int num)
{
	if(num == -1 && b)
		num = b->trucknum;

	//block until all threads done
	if (thread_mode==THREAD_HT)
	{
		MUTEX_LOCK(&done_count_mutex);
		while (done_count>0)
			pthread_cond_wait(&done_count_cv, &done_count_mutex);
		MUTEX_UNLOCK(&done_count_mutex);
	}

	// synced delete
	trucks[num] = 0;
	delete b;
	b = 0;
}


void BeamFactory::removeCurrentTruck()
{
	removeTruck(current_truck);
}

void BeamFactory::setCurrentTruck(int v)
{
	if (trucks[current_truck] && current_truck!=-1 && current_truck<free_truck)
		trucks[current_truck]->desactivate();
	int previous_truck = current_truck;
	current_truck=v;

	// fire update
	// TODO: remove this hack
	if(RoRFrameListener::eflsingleton)
		RoRFrameListener::eflsingleton->changedCurrentTruck(previous_truck<0?0:trucks[previous_truck], current_truck<0?0:trucks[current_truck]);
}

bool BeamFactory::enterRescueTruck()
{
	//rescue!
	//if (current_truck!=-1) BeamFactory::getSingleton().setCurrentTruck(-1);
	int rtruck=-1;
	// search a rescue truck
	for (int i=0; i<free_truck; i++)
	{
		if(!trucks[i]) continue;
		if (trucks[i]->rescuer)
			rtruck=i;
	}
	if(rtruck == -1)
	{
		return false;
	} else
	{
		// go to person mode first
		setCurrentTruck(-1);
		// then to the rescue truck, this fixes overlapping interfaces
		setCurrentTruck(rtruck);
	}
	return true;
}

void BeamFactory::updateVisual(float dt)
{
	for (int t=0; t<free_truck; t++)
	{
		if(!trucks[t]) continue;
		if (trucks[t]->state!=SLEEPING && trucks[t]->loading_finished)
		{
			trucks[t]->updateSkidmarks();
			trucks[t]->updateVisual(dt);
			trucks[t]->updateFlares(dt, (t==current_truck) );
		}
	}
}

void BeamFactory::updateAI(float dt)
{
	for (int t=0; t<free_truck; t++)
	{
		if(!trucks[t]) continue;
		
		trucks[t]->updateAI(dt);
	}
}


void BeamFactory::calcPhysics(float dt)
{
	int t=0;
	//this is the big "shaker"
	if (current_truck!=-1)
	{
		// now handle inter truck coll in different HT modes
		if (thread_mode == THREAD_HT2)
		{
			// wait for all threads to finish
			for (t=0; t<free_truck; t++)
			{
				if(!trucks[t]) continue;
				MUTEX_LOCK(&trucks[t]->done_count_mutex);
				while(trucks[t]->done_count > 0)
					pthread_cond_wait(&trucks[t]->done_count_cv, &trucks[t]->done_count_mutex);
				MUTEX_UNLOCK(&trucks[t]->done_count_mutex);
			}

			// smooth the stuff
			for (t=0; t<free_truck; t++)
			{
				if(!trucks[t]) continue;
				trucks[t]->frameStep(dt);
			}


			// inter truck coll.
			float dtperstep=dt/(Real)trucks[current_truck]->tsteps;
			trucks[current_truck]->truckTruckCollisions(dtperstep);

			// unlock all threads
			for (t=0; t<free_truck; t++)
			{
				if(!trucks[t]) continue;
				trucks[t]->done_count=1;
				MUTEX_LOCK(&trucks[t]->work_mutex);
				pthread_cond_broadcast(&trucks[t]->work_cv);
				MUTEX_UNLOCK(&trucks[t]->work_mutex);
			}
		} else
		{
			// classic mode
			trucks[current_truck]->frameStep(dt);
		}

	}

	//things always on
	for (t=0; t<free_truck; t++)
	{
		if(!trucks[t]) continue;
		//networked trucks must be taken care of

		switch(trucks[t]->state)
		{

			case NETWORKED:
			{
				trucks[t]->calcNetwork();
				break;
			}
			case RECYCLE:
			{
				break;
			}
			default:
			{
				if (t!=current_truck && trucks[t]->engine)
					trucks[t]->engine->update(dt, 1);
				if(trucks[t]->networking)
					trucks[t]->sendStreamData();
			}
		}
	}
}


void BeamFactory::removeInstance(stream_del_t *del)
{
	// we override this here so we can also delete the truck array content
	//already locked
	//lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		if(it1->first != del->sourceid) continue;

		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			if(del->streamid == -1 || del->streamid == (int)it2->first)
			{
				// clear the trucks array
				if(it2->second)
				{
					trucks[it2->second->trucknum] = 0;
					// deletes the stream
					
					
					// TODO: fix deletion
					//delete it2->second;
					it2->second = 0;
				}
			}
		}
		break;
	}
	//unlockStreams();
}
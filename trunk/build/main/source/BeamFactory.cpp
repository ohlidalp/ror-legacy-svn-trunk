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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 24th of August 2009

#include "BeamFactory.h"

#include "Ogre.h"
#include "network.h"
#include "utils.h"
#include "sha1.h"
#include "pthread.h"
#include "ExampleFrameListener.h"
#include "mirrors.h"

using namespace Ogre;


template<> BeamFactory *StreamableFactory < BeamFactory, Beam >::ms_Singleton = 0;

BeamFactory::BeamFactory(ExampleFrameListener *efl, Beam **trucks, SceneManager *manager, SceneNode *parent, RenderWindow* win, Network *net, float *mapsizex, float *mapsizez, Collisions *icollisions, HeightFinder *mfinder, Water *w, Camera *pcam, Mirrors *mmirror0) : efl(efl), trucks(trucks), manager(manager), parent(parent), win(win), net(net), mapsizex(mapsizex), mapsizez(mapsizez), icollisions(icollisions), mfinder(mfinder), pcam(pcam), mmirror0(mmirror0)
{
}

Beam *BeamFactory::createLocal()
{
	// do not use this ...
	return 0;
}

Beam *BeamFactory::createLocal(Ogre::Vector3 pos, Ogre::Quaternion rot, Ogre::String fname, collision_box_t *spawnbox, bool ismachine, int flareMode, std::vector<Ogre::String> *truckconfig, SkinPtr skin, bool freePosition)
{
	bool networked=false, networking=false;
	if(net) networking = true;

	Beam *b = new Beam(efl->free_truck,
		manager,
		manager->getRootSceneNode(),
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
		mmirror0,
		networked,
		networking,
		spawnbox,
		ismachine,
		flareMode,
		truckconfig,
		skin,
		freePosition);

	// lock slidenodes after spawning the truck?
	if(b->getSlideNodesLockInstant())
		b->toggleSlideNodeLock(efl->trucks, efl->free_truck, efl->free_truck);

	efl->trucks[efl->free_truck] = b;

	streamables[-1][10 + efl->free_truck] = b; // 10 streams offset
	//efl->free_truck++;
	return b;
}

Beam *BeamFactory::createRemote(int sourceid, stream_register_t *reg, int slotid)
{
	LogManager::getSingleton().logMessage(" new beam truck for " + StringConverter::toString(sourceid) + ":" + StringConverter::toString(reg->sid));

	bool networked=true, networking=false;
	if(net) networking = true;

	// spawn the truck far off anywhere
	Vector3 pos = Vector3(99999,99999,99999);

	Beam *b = new Beam(efl->free_truck,
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
		reg->name,
		icollisions,
		mfinder,
		w,
		pcam,
		mmirror0,
		networked,
		networking,
		0,
		false,
		0,
		0,
		SkinPtr());

	efl->trucks[efl->free_truck] = b;
	efl->free_truck++;

	streamables[sourceid][reg->sid] = b;

	return b;
}

void BeamFactory::remove(Beam *stream)
{
	delete stream;
	stream = 0;
	// TODO: find stream in streamables and remove it there ...
}

void BeamFactory::removeUser(int userid)
{
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		if(it1->first != userid) continue;

		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			delete it2->second;
			it2->second = 0;
		}
		break;
	}
}

void BeamFactory::localUserAttributesChanged(int newid)
{
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	if(streamables.find(-1) == streamables.end()) return;

	Beam *b = streamables[-1][0];
	streamables[newid][0] = streamables[-1][0]; // add alias :)
	//b->setUID(newid);
	//b->updateNetLabel();
}

void BeamFactory::netUserAttributesChanged(int source, int streamid)
{
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			Beam *b = it2->second;
			//b->updateNetLabel();
		}
	}
}
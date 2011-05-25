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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of August 2009

#include "CharacterFactory.h"
#include "Streamable.h"
#include "Character.h"

#include "Ogre.h"
#include "network.h"
#include "utils.h"
#include "sha1.h"
#include "pthread.h"

using namespace Ogre;


template<> CharacterFactory *StreamableFactory < CharacterFactory, Character >::ms_Singleton = 0;

CharacterFactory::CharacterFactory(Camera *cam, Network *net, Collisions *c, HeightFinder *h, Water *w, MapControl *m, Ogre::SceneManager *scm) : cam(cam), c(c), net(net), h(h), w(w), m(m), scm(scm)
{
}

Character *CharacterFactory::createLocal(int playerColour)
{
	Character *ch = new Character(cam, c, net, h, w, m, scm, -1, 0, playerColour, false);

	lockStreams();
	std::map < int, std::map < unsigned int, Character *> > &streamables = getStreams();
	streamables[-1][0] = ch;
	unlockStreams();
	return ch;
}

Character *CharacterFactory::createRemoteInstance(stream_reg_t *reg)
{
	// NO LOCKS IN HERE, already locked

	LOG(" new character for " + TOSTRING(reg->sourceid) + ":" + TOSTRING(reg->streamid) + ", colour: " + TOSTRING(reg->colour));
	Character *ch = new Character(cam, c, net, h, w, m, scm, reg->sourceid, reg->streamid, reg->colour, true);

	lockStreams();
	std::map < int, std::map < unsigned int, Character *> > &streamables = getStreams();
	streamables[reg->sourceid][reg->streamid] = ch;
	unlockStreams();
	return ch;
}

void CharacterFactory::localUserAttributesChanged(int newid)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Character *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Character *> >::iterator it1;
	std::map < unsigned int, Character *>::iterator it2;

	if(streamables.find(-1) == streamables.end())
	{
		unlockStreams();
		return;
	}

	Character *c = streamables[-1][0];
	streamables[newid][0] = streamables[-1][0]; // add alias :)
	c->setUID(newid);
	c->updateNetLabel();
	unlockStreams();
}

void CharacterFactory::netUserAttributesChanged(int source, int streamid)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Character *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Character *> >::iterator it1;
	std::map < unsigned int, Character *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			Character *c = dynamic_cast<Character*>(it2->second);
			if(c) c->updateNetLabel();
		}
	}
	unlockStreams();
}

void CharacterFactory::updateCharacters(float dt)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Character *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Character *> >::iterator it1;
	std::map < unsigned int, Character *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			Character *c = dynamic_cast<Character*>(it2->second);
			if(c) c->update(dt);
		}
	}
	unlockStreams();
}

void CharacterFactory::updateLabels()
{
	lockStreams();
	std::map < int, std::map < unsigned int, Character *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Character *> >::iterator it1;
	std::map < unsigned int, Character *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			Character *c = dynamic_cast<Character*>(it2->second);
			if(c) c->updateNetLabelSize();
		}
	}
	unlockStreams();
}

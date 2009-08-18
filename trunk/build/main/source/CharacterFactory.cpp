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

CharacterFactory& CharacterFactory::getSingleton(void)
{
	assert( ms_Singleton );  return ( *(dynamic_cast<CharacterFactory*>(ms_Singleton)) );
}


CharacterFactory::CharacterFactory(Collisions *c, Network *net, HeightFinder *h, Water *w, MapControl *m, Ogre::SceneManager *scm) : c(c), net(net), h(h), w(w), m(m), scm(scm), StreamableFactory()
{
}

Character *CharacterFactory::createLocal()
{
	Character *ch = new Character(c, net, h, w, m, scm, -1);
	streamables[-1][0] = ch;
	return ch;
}

Character *CharacterFactory::createRemote(int sourceid, stream_register_t *reg, int slotid)
{
	LogManager::getSingleton().logMessage(" new character for " + StringConverter::toString(sourceid) + ":" + StringConverter::toString(reg->sid));
	Character *ch = new Character(c, net, h, w, m, scm, sourceid, reg->sid, slotid);
	streamables[sourceid][reg->sid] = ch;
	return ch;
}

void CharacterFactory::remove(Character *stream)
{
}

void CharacterFactory::removeUser(int userid)
{
	std::map < int, std::map < unsigned int, Streamable *> >::iterator it1;
	std::map < unsigned int, Streamable *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		if(it1->first != userid) continue;

		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			Character *c = dynamic_cast<Character*>(it2->second);
			delete c;
		}
		break;
	}
}

void CharacterFactory::localUserAttributesChanged(int newid)
{
	std::map < int, std::map < unsigned int, Streamable *> >::iterator it1;
	std::map < unsigned int, Streamable *>::iterator it2;

	if(streamables.find(-1) == streamables.end()) return;

	Character *c = dynamic_cast<Character*>(streamables[-1][0]);
	streamables[newid][0] = streamables[-1][0]; // add alias :)
	c->setUID(newid);
	c->updateNetLabel();
}

void CharacterFactory::netUserAttributesChanged(int source, int streamid)
{
	std::map < int, std::map < unsigned int, Streamable *> >::iterator it1;
	std::map < unsigned int, Streamable *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			Character *c = dynamic_cast<Character*>(it2->second);
			c->updateNetLabel();
		}
	}
}
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

#include "ChatSystem.h"

#include "water.h"
#include "collisions.h"
#include "heightfinder.h"
#include "MapControl.h"
#include "InputEngine.h"
#include "network.h"
#include "NetworkStreamManager.h"
#include "ColoredTextAreaOverlayElement.h"
#include "language.h"

using namespace Ogre;

///////////////////////////////////
// ChatSystemFactory 

template<> ChatSystemFactory *StreamableFactory < ChatSystemFactory, ChatSystem >::ms_Singleton = 0;

ChatSystemFactory::ChatSystemFactory(Network *net) : net(net)
{
}

ChatSystemFactory::~ChatSystemFactory()
{
}

ChatSystem *ChatSystemFactory::createLocal(int playerColour)
{
	ChatSystem *ch = new ChatSystem(net, -1, 0, playerColour, false);
	streamables[-1][0] = ch;
	return ch;
}

ChatSystem *ChatSystemFactory::createRemote(int sourceid, int streamid, stream_register_t *reg, int playerColour)
{
	LogManager::getSingleton().logMessage(" new chat system for " + StringConverter::toString(sourceid) + ":" + StringConverter::toString(streamid) + ", colour: " + StringConverter::toString(playerColour));
	ChatSystem *ch = new ChatSystem(net, sourceid, streamid, playerColour, true);
	streamables[sourceid][streamid] = ch;
	return ch;
}

void ChatSystemFactory::remove(ChatSystem *stream)
{
}

void ChatSystemFactory::removeUser(int userid)
{
	std::map < int, std::map < unsigned int, ChatSystem *> >::iterator it1;
	std::map < unsigned int, ChatSystem *>::iterator it2;

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

#if 0
void ChatSystemFactory::updatePlayerList()
{
	for (int i=0; i<MAX_PEERS; i++)
	{
		if (!clients[i].used)
			continue;
		if(i < MAX_PLAYLIST_ENTRIES)
		{
			try
			{
				String plstr = StringConverter::toString(i) + ": " + ColoredTextAreaOverlayElement::StripColors(String(clients[i].user_name));
				if(clients[i].invisible)
					plstr += " (i)";
				mefl->playerlistOverlay[i]->setCaption(plstr);
			} catch(...)
			{
			}
		}
	}
}
#endif //0

///////////////////////////////////
// ChatSystem

ChatSystem::ChatSystem(Network *net, int source, unsigned int streamid, int colourNumber, bool remote) : 
	net(net), 
	source(source),
	streamid(streamid),
	colourNumber(colourNumber),
	remote(remote),
	username("unkown")
{
	sendStreamSetup();
	if(remote)
	{
		client_t *c = net->getClientInfo(source);
		if(c) username = String(c->user_name);
		//NETCHAT.addText(username + _L(" joined the chat with language ") + String(c->user_language));
		NETCHAT.addText(username + _L(" joined the chat"));
	}
}

ChatSystem::~ChatSystem()
{
	if(remote)
	{
		NETCHAT.addText(username + _L(" left the chat"));
	}
}

void ChatSystem::sendStreamSetup()
{
	if(remote)
	{
		LogManager::getSingleton().logMessage("new remote ChatSystem: " + StringConverter::toString(source) + ":"+ StringConverter::toString(streamid));
		NetworkStreamManager::getSingleton().addRemoteStream(this, source, streamid);
	} else
	{
		stream_register_t reg;
		reg.status = 1;
		strcpy(reg.name, "chat");
		reg.type = 3;
		NetworkStreamManager::getSingleton().addLocalStream(this, &reg);
	}
}

void ChatSystem::sendStreamData()
{
    // we send data syncronously to prevent lag
}

void ChatSystem::receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len)
{
	if(type == MSG2_CHAT)
	{
		// some chat code
		if(source == -1)
		{
			// server said something
			NETCHAT.addText(String(buffer));
		} else if(this->source == source && this->streamid == streamid)
		{
			NETCHAT.addText(username + ": " + String(buffer));
		}
	}
}

void ChatSystem::sendChat(Ogre::UTFString chatline)
{
	this->addPacket(MSG2_CHAT, chatline.size(), const_cast<char *>(chatline.asUTF8_c_str()));
}


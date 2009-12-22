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

void ChatSystemFactory::createRemoteInstance(stream_reg_t *reg)
{
	LogManager::getSingleton().logMessage(" new chat system for " + StringConverter::toString(reg->sourceid) + ":" + StringConverter::toString(reg->streamid) + ", colour: " + StringConverter::toString(reg->colour));
	ChatSystem *ch = new ChatSystem(net, reg->sourceid, reg->streamid, reg->colour, true);
	streamables[reg->sourceid][reg->streamid] = ch;
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


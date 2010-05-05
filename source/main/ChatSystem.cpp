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
#include "utils.h"
#include "PlayerColours.h"

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
	LOCKSTREAMS();
	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	ChatSystem *ch = new ChatSystem(net, -1, 0, playerColour, false);
	streamables[-1][0] = ch;
	UNLOCKSTREAMS();
	return ch;
}

ChatSystem *ChatSystemFactory::createRemoteInstance(stream_reg_t *reg)
{
	// NO LOCKS IN HERE, already locked

	LogManager::getSingleton().logMessage(" new chat system for " + StringConverter::toString(reg->sourceid) + ":" + StringConverter::toString(reg->streamid) + ", colour: " + StringConverter::toString(reg->colour));
	ChatSystem *ch = new ChatSystem(net, reg->sourceid, reg->streamid, reg->colour, true);

	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	streamables[reg->sourceid][reg->streamid] = ch;
	return ch;
}

void ChatSystemFactory::syncRemoteStreams()
{
	StreamableFactory <ChatSystemFactory, ChatSystem>::syncRemoteStreams();

	// now add the update for the player list here
	updatePlayerList();
}

void ChatSystemFactory::updatePlayerList()
{
	if(!net) return;

	LOCKSTREAMS();

	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, ChatSystem *> >::iterator it1;

	// the player itself
	String oname = "tracks/MPPlayerList/Player0";
	String colname = "tracks/MPPlayerList/Player0/Colour";
	TextAreaOverlayElement *te = (TextAreaOverlayElement*)OverlayManager::getSingleton().getOverlayElement(oname);
	TextAreaOverlayElement *tec = (TextAreaOverlayElement*)OverlayManager::getSingleton().getOverlayElement(colname);
#ifdef USE_SOCKETW
	if(te && tec)
	{
		client_info_on_join *c = net->getLocalUserData();
		if(c)
		{
			try
			{
				UTFString username = tryConvertUTF(c->nickname);
				te->setCaption(username);

				String matName = PlayerColours::getSingleton().getColourMaterial(c->colournum);
				tec->setMaterialName(matName);
				if(c->authstatus & AUTH_ADMIN)
					te->setColour(ColourValue(1,0,0));
				else if(c->authstatus & AUTH_RANKED)
					te->setColour(ColourValue(0,1,0));
				else
					te->setColour(ColourValue::Black);


			} catch(...)
			{
			}
		}
	}

	int num=1;
	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		if(it1->second.size() == 0) continue;

		client_t *c = net->getClientInfo(it1->first);
		if(!c) continue;

		try
		{
			UTFString username = tryConvertUTF(c->user_name);

			String plstr = StringConverter::toString(num) + ": " + ColoredTextAreaOverlayElement::StripColors(username);

			String oname = "tracks/MPPlayerList/Player" + StringConverter::toString(num);
			String colname = "tracks/MPPlayerList/Player"+ StringConverter::toString(num)+"/Colour";
			te = (TextAreaOverlayElement*)OverlayManager::getSingleton().getOverlayElement(oname);
			tec = (TextAreaOverlayElement*)OverlayManager::getSingleton().getOverlayElement(colname);
			if(!te || !tec) break;
			te->setCaption(plstr);
			String matName = PlayerColours::getSingleton().getColourMaterial(c->colournum);
			tec->setMaterialName(matName);
			if(c->user_authlevel & AUTH_ADMIN)
				te->setColour(ColourValue(1,0,0));
			else if(c->user_authlevel & AUTH_RANKED)
				te->setColour(ColourValue(0,1,0));
			else
				te->setColour(ColourValue::Black);

		} catch(...)
		{
		}

		num++;
	}
#endif //SOCKETW

	UNLOCKSTREAMS();
}

ChatSystem *ChatSystemFactory::getFirstChatSystem()
{
	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	if(streamables.empty() || streamables.begin()->second.empty()) return 0;
	return streamables.begin()->second.begin()->second;
}

///////////////////////////////////
// ChatSystem

ChatSystem::ChatSystem(Network *net, int source, unsigned int streamid, int colourNumber, bool remote) :
	net(net),
	source(source),
	streamid(streamid),
	colourNumber(colourNumber),
	remote(remote),
	username("unkown"),
	mNickColour("^0")
{
	sendStreamSetup();
#ifdef USE_SOCKETW
	if(remote)
	{
		client_t *c = net->getClientInfo(source);
		if(c)
		{
			username = tryConvertUTF(c->user_name);

			int nickColour = 8;
			if(c->user_authlevel & AUTH_NONE)   nickColour = 8; // grey
			if(c->user_authlevel & AUTH_BOT )   nickColour = 4; // blue
			if(c->user_authlevel & AUTH_RANKED) nickColour = 2; // green
			if(c->user_authlevel & AUTH_MOD)    nickColour = 1; // red
			if(c->user_authlevel & AUTH_ADMIN)  nickColour = 1; // red

			mNickColour = String("^") + StringConverter::toString(nickColour);
			username = mNickColour + ColoredTextAreaOverlayElement::StripColors(username);
		}

		NETCHAT.addText(username + _L(" ^9joined the chat"));
	}
#endif //SOCKETW
}

ChatSystem::~ChatSystem()
{
	if(remote)
	{
		NETCHAT.addText(username + _L(" ^9left the chat"));
	}
}

void ChatSystem::sendStreamSetup()
{
	if(remote) return;

	stream_register_t reg;
	reg.status = 1;
	strcpy(reg.name, "chat");
	reg.type = 3;
	NetworkStreamManager::getSingleton().addLocalStream(this, &reg);
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
		} else if(source == (int)this->source && (int)streamid == this->streamid)
		{
			UTFString text = tryConvertUTF(buffer);
			NETCHAT.addText(username + "^7: " + text);
		}
	}
}

void ChatSystem::sendChat(Ogre::UTFString chatline)
{
	this->addPacket(MSG2_CHAT, chatline.size(), const_cast<char *>(chatline.asUTF8_c_str()));
}


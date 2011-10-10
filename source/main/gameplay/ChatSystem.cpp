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

#ifdef USE_MYGUI
#include "gui_mp.h"
#endif  // USE_MYGUI

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
	lockStreams();
	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	ChatSystem *ch = new ChatSystem(net, -1, 0, playerColour, false);
	streamables[-1][0] = ch;
	unlockStreams();
	return ch;
}

ChatSystem *ChatSystemFactory::createRemoteInstance(stream_reg_t *reg)
{
	// NO LOCKS IN HERE, already locked

	LOG(" new chat system for " + TOSTRING(reg->sourceid) + ":" + TOSTRING(reg->streamid) + ", colour: " + TOSTRING(reg->colour));
	ChatSystem *ch = new ChatSystem(net, reg->sourceid, reg->streamid, reg->colour, true);

	// already locked
	//lockStreams();
	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	streamables[reg->sourceid][reg->streamid] = ch;
	//unlockStreams();
	return ch;
}

bool ChatSystemFactory::syncRemoteStreams()
{
	// we override this here, so we know if something changed and could update the player list
	bool changes = StreamableFactory <ChatSystemFactory, ChatSystem>::syncRemoteStreams();

#ifdef USE_MYGUI
#ifdef USE_SOCKETW
	if(changes)
		GUI_Multiplayer::getSingleton().update();
#endif // USE_SOCKETW
#endif // USE_MYGUI	
	return changes;
}

ChatSystem *ChatSystemFactory::getFirstChatSystem()
{
	lockStreams();
	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	if(streamables.empty() || streamables.begin()->second.empty()) return 0;
	ChatSystem *r = streamables.begin()->second.begin()->second;
	unlockStreams();
	return r;
}

///////////////////////////////////
// ChatSystem

const Ogre::String ChatSystem::commandColour = "#e72edc";
const Ogre::String ChatSystem::normalColour  = "#000000";

ChatSystem::ChatSystem(Network *net, int source, unsigned int streamid, int colourNumber, bool remote) :
	net(net),
	source(source),
	streamid(streamid),
	colourNumber(colourNumber),
	remote(remote),
	username("unknown"),
	mNickColour("")
{
	sendStreamSetup();
#ifdef USE_SOCKETW
	if(remote)
	{
		client_t *c = net->getClientInfo(source);
		if(c)
		{
			username = getColouredName(tryConvertUTF(c->user.username), c->user.authstatus, c->user.colournum);
		}

		String msg = username + commandColour + _L(" joined the game");
		Console::getInstance().putMessage(Console::CONSOLE_MSGTYPE_NETWORK, msg, "user_add.png");
	}
#endif //SOCKETW
}

ChatSystem::~ChatSystem()
{
	if(remote)
	{
		String msg = username + commandColour + _L(" left the game");
		Console::getInstance().putMessage(Console::CONSOLE_MSGTYPE_NETWORK, msg, "user_delete.png");
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
    // we send data synchronously to prevent lag
}

void ChatSystem::receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len)
{
	if(type == MSG2_CHAT)
	{
		// some chat code
		if(source == -1)
		{
			// server said something
			String msg = getASCIIFromCharString(buffer, 255);
			Console::getInstance().putMessage(Console::CONSOLE_MSGTYPE_NETWORK, msg, "user_gray.png");
		} else if(source == (int)this->source && (int)streamid == this->streamid)
		{
			UTFString msg = username + normalColour + ": " + tryConvertUTF(buffer);
			Console::getInstance().putMessage(Console::CONSOLE_MSGTYPE_NETWORK, msg, "user_comment.png");
		}
	}
}

void ChatSystem::sendChat(Ogre::UTFString chatline)
{
	this->addPacket(MSG2_CHAT, chatline.size(), const_cast<char *>(chatline.asUTF8_c_str()));
}

String ChatSystem::getColouredName(String nick, int auth, int colourNumber)
{
	Ogre::ColourValue col_val = PlayerColours::getSingleton().getColour(colourNumber);
	char tmp[255] = "";
	sprintf(tmp, "#%02X%02X%02X", (unsigned int)(col_val.r * 255.0f), (unsigned int)(col_val.g * 255.0f), (unsigned int)(col_val.b * 255.0f));
	return String(tmp) + nick;

#if 0
	// old code: colour not depending on auth status anymore ...
	// replace # with X so the user cannot fake colour
	for(unsigned int i=0; i<nick.size(); i++)
		if(nick[i] == '#') nick[i] = 'X';

	if(auth == AUTH_NONE)  col = "#c9c9c9"; // grey
	if(auth & AUTH_BOT )   col = "#0000c9"; // blue
	if(auth & AUTH_RANKED) col = "#00c900"; // green
	if(auth & AUTH_MOD)    col = "#c90000"; // red
	if(auth & AUTH_ADMIN)  col = "#c97100"; // orange

	return col + nick;
#endif //0
}
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

const Ogre::String ChatSystem::commandColour       = "#941e8d";
const Ogre::String ChatSystem::normalColour        = "#000000";
const Ogre::String ChatSystem::whisperColour       = "#967417";
const Ogre::String ChatSystem::scriptCommandColour = "#32436f";




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
			username = getColouredName(*c);
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
	else if(type == MSG2_PRIVCHAT)
	{
		// some private chat message
		if(source == -1)
		{
			// server said something
			String msg = whisperColour + _L(" [whispered] ") + normalColour +  getASCIIFromCharString(buffer, 255);
			Console::getInstance().putMessage(Console::CONSOLE_MSGTYPE_NETWORK, msg, "script_key.png");
		} else if(source == (int)this->source && (int)streamid == this->streamid)
		{
			UTFString msg = username + _L(" [whispered] ") + normalColour + ": " + tryConvertUTF(buffer);
			Console::getInstance().putMessage(Console::CONSOLE_MSGTYPE_NETWORK, msg, "script_key.png");
		}
	}
}

void ChatSystem::sendChat(Ogre::UTFString chatline)
{
	this->addPacket(MSG2_CHAT, chatline.size(), const_cast<char *>(chatline.asUTF8_c_str()));

	String nmsg = net->getNickname(true) + normalColour + ": " + chatline;
	Console::getInstance().putMessage(Console::CONSOLE_MSGTYPE_NETWORK, nmsg, "user_comment.png");
}

int ChatSystem::getChatUserNames(std::vector<MyGUI::UString> &names)
{
	client_t c[MAX_PEERS];
	if(net->getClientInfos(c)) return 0;

	for(int i = 0; i < MAX_PEERS; i++)
	{
		names.push_back(c[i].user.username);
	}
	return names.size();
}

void ChatSystem::sendPrivateChat(Ogre::String targetUsername, Ogre::UTFString chatline)
{
	char buffer[MAX_MESSAGE_LENGTH] = "";

	// first: find id to username:
	const char *target_username = targetUsername.c_str();
	const char *chat_msg = chatline.asUTF8_c_str();

	client_t c[MAX_PEERS];
	if(net->getClientInfos(c))
		return;
	int target_uid = -1, target_index = -1;
	for(int i = 0; i < MAX_PEERS; i++)
	{
		if(!strcmp(c[i].user.username, target_username))
		{
			// found it :)
			target_uid = c[i].user.uniqueid;
			target_index = i;
			break;
		}
	}

	if(target_uid < 0)
	{
		Console::getInstance().putMessage(Console::CONSOLE_MSGTYPE_INFO, ChatSystem::commandColour + _L("user not found: ") + targetUsername, "error.png");
		return;
	}

	// format: int of UID, then chat message
	memcpy(buffer, &target_uid, sizeof(int));
	strncpy(buffer + sizeof(int), chat_msg, MAX_MESSAGE_LENGTH - sizeof(int));

	size_t len = sizeof(int) + strlen(chat_msg);
	buffer[len] = 0;

	this->addPacket(MSG2_PRIVCHAT, len, buffer);

	// add local visual
	String nmsg = net->getNickname(true) + normalColour + whisperColour + _L("[whispered to ") + normalColour + getColouredName(c[target_index]) + "]" + normalColour + ": " + chatline;
	Console::getInstance().putMessage(Console::CONSOLE_MSGTYPE_NETWORK, nmsg, "script_key.png");
}

String ChatSystem::getColouredName(client_t &c)
{
	return getColouredName(c.user);
}

String ChatSystem::getColouredName(user_info_t &u)
{
	return ChatSystem::getColouredName(tryConvertUTF(u.username), u.authstatus, u.colournum);
}

String ChatSystem::getColouredName(String nick, int auth, int colourNumber)
{
	Ogre::ColourValue col_val = PlayerColours::getSingleton().getColour(colourNumber);
	char tmp[255] = "";
	sprintf(tmp, "#%02X%02X%02X", (unsigned int)(col_val.r * 255.0f), (unsigned int)(col_val.g * 255.0f), (unsigned int)(col_val.b * 255.0f));

	// replace # with X in nickname so the user cannot fake the colour
	for(unsigned int i=0; i<nick.size(); i++)
		if(nick[i] == '#') nick[i] = 'X';

	return String(tmp) + nick;

#if 0
	// old code: colour not depending on auth status anymore ...

	if(auth == AUTH_NONE)  col = "#c9c9c9"; // grey
	if(auth & AUTH_BOT )   col = "#0000c9"; // blue
	if(auth & AUTH_RANKED) col = "#00c900"; // green
	if(auth & AUTH_MOD)    col = "#c90000"; // red
	if(auth & AUTH_ADMIN)  col = "#c97100"; // orange

	return col + nick;
#endif //0
}

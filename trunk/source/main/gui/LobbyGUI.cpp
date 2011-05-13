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
#ifdef USE_MYGUI 

#include "LobbyGUI.h"
#include "ScriptEngine.h"
#include "InputEngine.h"
#include "OgreLogManager.h"

#include "Settings.h"
#include "RoRFrameListener.h"
#include "network.h"

#include <MyGUI.h>

#include "libircclient.h"

// class

LobbyGUI::LobbyGUI()
{
	initialiseByAttributes(this);

	commandBox->eventEditSelectAccept += MyGUI::newDelegate(this, &LobbyGUI::eventCommandAccept);

	initIRC();
}

LobbyGUI::~LobbyGUI()
{
}

void LobbyGUI::setVisible( bool _visible )
{
	mainWindow->setVisible(_visible);
}

bool LobbyGUI::getVisible()
{
	return mainWindow->getVisible();
}

void LobbyGUI::eventCommandAccept(MyGUI::Edit* _sender)
{
	MyGUI::UString message = _sender->getCaption();
	if(!sendMessage(message))
	{
		std::string fmt = "<" + nick + "> " + message;
		addTextToChatWindow(fmt);
	} else
	{
		addTextToChatWindow("cannot send to channel");
	}
	_sender->setCaption("");
}

void LobbyGUI::processIRCEvent(message_t &msg)
{
	// cutoff the nickname after the !
	if(!msg.nick.empty())
	{
		size_t s = msg.nick.find("!");
		if(s != msg.nick.npos)
		{
			msg.nick = msg.nick.substr(0, s);
		}
	}
	
	// properly replace all # with ## in order not to confuse the GUI
	if(!msg.message.empty())
	{
		string::size_type pos = 0;
		while ( (pos = msg.message.find("#", pos)) != string::npos )
		{
			msg.message.replace( pos, 1, "##" );
			pos++;
		}
	}

	switch(msg.type)
	{
	case MT_Channel:
		{
			std::string fmt = "<" + msg.nick + "> " + msg.message;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_JoinChannelSelf:
		{
			std::string fmt = "joined channel " + msg.channel + " as " + msg.nick + "";
			addTextToChatWindow(fmt);
		}
		break;
	case MT_JoinChannelOther:
		{
			std::string fmt = "*** " + msg.nick + " has joined " + msg.channel;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_PartChannelOther:
		{
			std::string fmt = "*** " + msg.nick + " has left " + msg.channel;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_QuitOther:
		{
			std::string fmt = "*** " + msg.nick + " has quit (" + msg.message + ")";
			addTextToChatWindow(fmt);
		}
		break;
	case MT_PartChannelSelf:
		{
			std::string fmt = "*** you left the channel " + msg.channel;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_ChangedChannelTopic:
		{
			std::string fmt = "*** " + msg.nick + " has changed the channel topic of " + msg.channel + " to " + msg.message;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_WeGotKicked:
		{
			std::string fmt = "*** we got kicked from " + msg.channel + " with the reason " + msg.message;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_SomeoneGotKicked:
		{
			std::string fmt = "*** " + msg.nick + " got kicked from " + msg.channel + " with the reason " + msg.message;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_GotPrivateMEssage:
		{
			std::string fmt = "(private) <" + msg.nick + "> " + msg.message;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_GotNotice:
	case MT_VerboseMessage:
		{
			std::string fmt;
			if(msg.nick.empty())
				fmt = "(NOTICE) " + msg.message;
			else
				fmt = "(NOTICE) <" + msg.nick + "> " + msg.message;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_GotInvitation:
		{
			std::string fmt = "*** " + msg.nick + " invited you to channel " + msg.message;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_TopicInfo:
		{
			std::string fmt = "*** Topic is " + msg.message;
			addTextToChatWindow(fmt);
		}
		break;
	case MT_NameList:
		{
			/*
			Ogre::StringVector v = Ogre::StringUtil::split(msg.message, " ");
			for(int i=0; i<v.size(); i++)
			{

			}
			*/
			std::string fmt = "*** Users are: " + msg.message;
			addTextToChatWindow(fmt);
		}
		break;

		
	}
}

void LobbyGUI::addTextToChatWindow(std::string txt)
{
	if (!chatWindow->getCaption().empty())
		chatWindow->addText("\n" + txt);
	else
		chatWindow->addText(txt);

	chatWindow->setTextSelection(chatWindow->getTextLength(), chatWindow->getTextLength());	
}

void LobbyGUI::update(float dt)
{
	process();
}

#endif //USE_MYGUI 

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

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "RoRPrerequisites.h"
#include "Singleton.h"
#include "mygui/BaseLayout.h"

#include "InterThreadStoreVector.h"

#include <OgreLog.h>
#include <OgreUTFString.h>


typedef struct msg_t {
	char type;
	unsigned long time; // post time in milliseconds since RoR start
	unsigned long ttl; // in milliseconds
	char txt[2048];
	char icon[50];
	//Ogre::String channel;
} msg_t;

class Console :
	public Singleton2<Console>,
	public Ogre::LogListener,
	public InterThreadStoreVector<msg_t>
{
	friend class Singleton2<Console>;
	Console();
	~Console();
public:
	void setVisible(bool _visible);
	bool getVisible();

	void unselect();
	void select();

	void setNetwork(Network *net);
	void setNetChat(ChatSystem *net);

	void frameEntered(float dt);

	enum {CONSOLE_MSGTYPE_LOG, CONSOLE_MSGTYPE_INFO, CONSOLE_MSGTYPE_SCRIPT, CONSOLE_MSGTYPE_NETWORK, CONSOLE_MSGTYPE_FLASHMESSAGE};

	void putMessage(int type, Ogre::String msg, Ogre::String icon = "bullet_black.png", unsigned long ttl = 120000);

	void resized();
protected:
	static const unsigned int lineheight   = 16;
	static const unsigned int LINES_MAX    = 200;
	static const unsigned int MESSAGES_MAX = 3000;
	unsigned int top_border, bottom_border;
	unsigned int linecount, scroll_size;
	int scrollOffset;
	bool inputMode, linesChanged;
	MyGUI::ImageBox *scrollImgUp, *scrollImgDown;

	int messageUpdate(float dt);
	void updateGUILines( float dt );
	void updateGUIVisual( float dt );

	void eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
	void eventCommandAccept(MyGUI::Edit* _sender);

	void saveChat(Ogre::String filename);
	void outputCurrentPosition();


	MyGUI::Edit* mCommandEdit;
	MyGUI::WindowPtr mMainWidget;

	Network *net;
	ChatSystem *netChat;

	typedef struct mygui_console_line_t {
		MyGUI::TextBox  *txtctrl;
		MyGUI::ImageBox *iconctrl;
		int number;
		msg_t *msg;
		bool expired;
	} mygui_console_line_t;

	mygui_console_line_t lines[LINES_MAX];
	msg_t messages[MESSAGES_MAX];
	unsigned int message_counter;

	std::vector<MyGUI::UString> mHistory;
	int mHistoryPosition;

#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
	void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName);
#else
	void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName, bool& skipThisMessage);
#endif // OGRE_VERSION
};

#endif // __CONSOLE_H__

#endif //MYGUI


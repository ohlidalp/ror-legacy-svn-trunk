/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
	int sender_uid;
	unsigned long time; // post time in milliseconds since RoR start
	unsigned long ttl;  // in milliseconds
	Ogre::UTFString txt; // not POD, beware...
	char icon[50];
	bool forcevisible;
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
	void select(Ogre::UTFString start = "");

	void setNetwork(Network *net);
	void setNetChat(ChatSystem *net);

	void startPrivateChat(int target_uid);

	void frameEntered(float dt);

	enum {CONSOLE_MSGTYPE_LOG, CONSOLE_MSGTYPE_INFO, CONSOLE_MSGTYPE_SCRIPT, CONSOLE_MSGTYPE_NETWORK, CONSOLE_MSGTYPE_FLASHMESSAGE, CONSOLE_MSGTYPE_HIGHSCORE};
	
	enum {
		// detailed message type identifier, mostly used for message filtering
		CONSOLE_VEHILCE_ADD, 
		CONSOLE_VEHILCE_DELETE,
		CONSOLE_JOIN_GAME,
		CONSOLE_LEAVE_GAME,

		CONSOLE_HELP,
		CONSOLE_CHAT,
		CONSOLE_LOCAL_CHAT, // spoken word by self

		CONSOLE_LOCAL_SCRIPT, // script self

		CONSOLE_SYSTEM_NOTICE,
		CONSOLE_SYSTEM_ERROR,
		CONSOLE_SYSTEM_REPLY, // reply to a commands

		CONSOLE_SYSTEM_DEBUG,

		CONSOLE_LOGMESSAGE,
		CONSOLE_LOGMESSAGE_SCRIPT,

		MSG_CUSTOM,
		};

	void putMessage(int type, int uid, Ogre::UTFString msg, Ogre::String icon = "bullet_black.png", unsigned long ttl = 30000, bool forcevisible = true);

	void resized();
protected:
	static const unsigned int lineheight   = 16;
	static const unsigned int LINES_MAX    = 200;
	static const unsigned int MESSAGES_MAX = 3000;
	static const Ogre::UTFString wordDelimiters;

	unsigned int top_border, bottom_border;
	unsigned int linecount, scroll_size;
	int scrollOffset, autoCompleteIndex;
	bool inputMode, linesChanged, angelscriptMode;
	MyGUI::ImageBox *scrollImgUp, *scrollImgDown;

	int messageUpdate(float dt);
	void updateGUILines( float dt );
	void updateGUIVisual( float dt );

	void eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
	void eventCommandAccept(MyGUI::Edit* _sender);

	void saveChat(Ogre::String filename);

	void outputCurrentPosition();
	void outputCurrentTerrainHeight();
	void jumpToPosition(Ogre::Vector3 pos);


	MyGUI::Edit* mCommandEdit;
	MyGUI::WindowPtr mMainWidget;
	MyGUI::ListBox* mAutoCompleteList;
	MyGUI::PopupMenu *popMenu;

	void onPopUpBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item);
	void onLineClicked(MyGUI::Widget* _sender);

	/// Auto-completion things start
	std::vector<Ogre::UTFString> autoCompleteChoices; // array containing the possible completions
	int autoCompletionWordStart, autoCompletionWordEnd, autoCompletionCursor; // variables holding the current word start and end
	Ogre::UTFString autoCompletionWord;

	void walkAutoCompletion(bool direction=true);
	void initOrWalkAutoCompletion();
	void finalizeAutoCompletion();
	void abortAutoCompletion();
	void findCurrentWord();
	/// Auto-completion things end

	Network *net;
	ChatSystem *netChat;

	typedef struct mygui_console_line_t {
		MyGUI::TextBox  *txtctrl;
		MyGUI::ImageBox *iconctrl;
		int number;
		msg_t *msg;
		bool expired;
	} mygui_console_line_t;

	/// filter related things start
	msg_t popUpContext;
	int popUpContextNumber;
	
	void loadFilters();
	void saveFilters();
	
	typedef struct console_filter_t {
		int type; // type of filter: 0=filter unique message id, 1=user filter: filter username
		int msg_uid;
	} console_filter_t;
	std::vector<console_filter_t> filters;
	/// filter related things end

	mygui_console_line_t lines[LINES_MAX];
	msg_t messages[MESSAGES_MAX];
	unsigned int message_counter;

	std::vector<Ogre::UTFString> mHistory;
	int mHistoryPosition;

#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
	void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName);
#else
	void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName, bool& skipThisMessage);
#endif // OGRE_VERSION
};

#endif // __CONSOLE_H__

#endif //MYGUI


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

#include <OgreLog.h>
#include <OgreUTFString.h>
#include <pthread.h>

#define NETCHAT Console::get()


typedef struct irc_ctx_t
{
	char 	* channel;
	char 	channel1[255];
	char 	nick[255];
	int nickRetry;

} irc_ctx_t;


ATTRIBUTE_CLASS_LAYOUT(Console, "Console.layout");
class Console :
	public wraps::BaseLayout,
	public Singleton2<Console>,
	public Ogre::LogListener
{
	friend class Singleton2<Console>;
	Console();
	~Console();
public:

	typedef struct msg_t {
		MyGUI::UString txt;
		Ogre::String channel;
	} msg_t;

	typedef struct tabctx_t {
		Ogre::String name;
		
		MyGUI::TabItemPtr tab;
		MyGUI::EditPtr txt;
		
		int mHistoryPosition;
		std::vector<MyGUI::UString> mHistory;
	} tabctx_t;


	void setVisible(bool _visible);
	bool getVisible();

	void print(const MyGUI::UString &text, Ogre::String channel);
	void printUTF(const Ogre::UTFString &text, Ogre::String channel = "OgreLog");

	void select();

	void setNetwork(Network *net);

	// method from Ogre::LogListener
	virtual void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName );
	// print waiting messages
	void frameEntered(float _frame);
protected:
	void eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
	void eventCommandAccept(MyGUI::Edit* _sender);
	void eventChangeTab(MyGUI::TabControl* _sender, size_t _index);

	ATTRIBUTE_FIELD_WIDGET_NAME(Console, mTabControl, "TabControl");
	MyGUI::TabControl* mTabControl;

	ATTRIBUTE_FIELD_WIDGET_NAME(Console, mCommandEdit, "Command");
	MyGUI::Edit* mCommandEdit;

	bool mVisible;

	Network *net;

	pthread_mutex_t mWaitingMessagesMutex;
	std::vector<msg_t> mWaitingMessages;

	std::map<Ogre::String , tabctx_t > tabs;

	tabctx_t *current_tab;

	void initIRC(irc_ctx_t *ctx);
	void addTab(Ogre::String name);
};

#endif // __CONSOLE_H__

#endif //MYGUI


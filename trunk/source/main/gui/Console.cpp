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

#include "Console.h"
#include "ScriptEngine.h"
#include "InputEngine.h"
#include "OgreLogManager.h"

#include "Settings.h"
#include "RoRFrameListener.h"
#include "network.h"

#include "libircclient.h"

// hack, hacky but works well
irc_session_t *s   = 0;
irc_ctx_t     *ctx = 0;

// class

Console::Console() : net(0)
{
	initialiseByAttributes(this);

	// make it 1/3 of screen height and place over screen

	/*
	MyGUI::IntSize size = MyGUI::RenderManager::getInstance().getViewSize();
	size.height = size.height/3;
	mMainWidget->setCoord(0, -size.height, size.width, size.height);
	*/

	mCommandEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &Console::eventButtonPressed);
	mCommandEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &Console::eventCommandAccept);

	mTabControl->eventTabChangeSelect   += MyGUI::newDelegate(this, &Console::eventChangeTab);

	addTab("OgreLog");
	addTab("IRCDebug");

	// BUG: all editboxes visible on startup D:
	mTabControl->selectSheetIndex(0, false);

	current_tab = &tabs["OgreLog"];

	setVisible(true);

	pthread_mutex_init(&mWaitingMessagesMutex, NULL);

	MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate( this, &Console::frameEntered );

	bool ogre_log = BSETTING("Enable Ingame Console");
	if(ogre_log)
		Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this);
}

Console::~Console()
{
	Ogre::LogManager::getSingleton().getDefaultLog()->removeListener(this);
	MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate( this, &Console::frameEntered );

	pthread_mutex_destroy(&mWaitingMessagesMutex);
}

void Console::addTab(Ogre::String name)
{
/*
<Widget type="TabItem" skin="" position="2 24 304 193">
<Property key="Caption" value="Log"/>
<Widget type="EditBox" skin="EditBoxStretch" position="0 0 306 195" align="Stretch" name="OgreLog">
<Property key="WordWrap" value="true"/>
<Property key="TextAlign" value="Left Bottom"/>
<Property key="ReadOnly" value="true"/>
<Property key="OverflowToTheLeft" value="true"/>
<Property key="MaxTextLength" value="8192"/>
<Property key="FontName" value="Default"/>
</Widget>
</Widget>
*/
	tabctx_t *t = &tabs[name];
	t->name = name;

	// create tab
	t->tab = mTabControl->addItem(name);
	t->tab->setProperty("Caption", name);
	t->tab->setCaption(name);

	// and the textbox inside
	t->txt = MyGUI::Gui::getInstance().createWidget<MyGUI::EditBox>("EditBoxStretch", 0, 0, 304, 193,  MyGUI::Align::Stretch, name);
	t->txt->setProperty("WordWrap", "true");
	t->txt->setProperty("TextAlign", "Left Bottom");
	t->txt->setProperty("ReadOnly", "true");
	t->txt->setProperty("OverflowToTheLeft", "true");
	t->txt->setProperty("MaxTextLength", "8192");
	t->txt->setWidgetStyle(MyGUI::WidgetStyle::Child);

	// attach it to the tab
	t->txt->detachFromWidget();
	t->txt->attachToWidget(t->tab);


	t->txt->setSize(t->tab->getSize().width, t->tab->getSize().height);

	//mTabControl->setItemData(t->tab, t);
	t->tab->setUserData(t);
	
	t->mHistoryPosition = 0;
	t->mHistory.push_back("");

}

void Console::setVisible(bool _visible)
{
	mVisible = _visible;

	mMainWidget->setEnabledSilent(_visible);
	if (_visible)
	{
		MyGUI::InputManager::getInstance().setKeyFocusWidget(mCommandEdit);
	}
	else
	{
		MyGUI::InputManager::getInstance().resetKeyFocusWidget(mCommandEdit);
	}
}

bool Console::getVisible()
{
	return mVisible;
}

void Console::print(const MyGUI::UString &_text, Ogre::String channel)
{
	pthread_mutex_lock(&mWaitingMessagesMutex);
	msg_t t;
	t.txt = _text;
	t.channel = channel; 
	mWaitingMessages.push_back(t);
	pthread_mutex_unlock(&mWaitingMessagesMutex);
}

void Console::printUTF(const Ogre::UTFString &_text, Ogre::String channel)
{
	this->print(_text.asWStr_c_str(), channel);
}

#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
std::wstring ansi_to_utf16(const std::string& _source)
{
	const char* srcPtr = _source.c_str();
	int tmpSize = MultiByteToWideChar( CP_ACP, 0, srcPtr, -1, 0, 0 );
	WCHAR* tmpBuff = new WCHAR [ tmpSize + 1 ];
	MultiByteToWideChar( CP_ACP, 0, srcPtr, -1, tmpBuff, tmpSize );
	std::wstring ret = tmpBuff;
	delete[] tmpBuff;
	return ret;
}
#endif

void Console::messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName )
{
	Ogre::String msg = message;
	//this->print(logName+": "+message);
	// strip script engine things
	if(message.substr(0,4) == "SE| ")
		msg = message.substr(4);
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
	this->print(MyGUI::UString("#988310") + ansi_to_utf16(msg), "OgreLog");
#else
	this->printUTF(msg, "OgreLog");
#endif
}

void Console::select()
{
	MyGUI::InputManager::getInstance().setKeyFocusWidget(mCommandEdit);
}

void Console::frameEntered(float _frame)
{
	// only copy the content and then unlock again
	// this prevents that loggers are hung due to time consuming iteration over it
	std::vector<msg_t> tmpWaitingMessages;
	pthread_mutex_lock(&mWaitingMessagesMutex);
	tmpWaitingMessages = mWaitingMessages;
	mWaitingMessages.clear();
	pthread_mutex_unlock(&mWaitingMessagesMutex);

	if (tmpWaitingMessages.empty())
		return;
	
	for (std::vector<msg_t>::iterator iter = tmpWaitingMessages.begin(); iter != tmpWaitingMessages.end(); ++iter)
	{
		if(tabs.find(iter->channel) == tabs.end())
		{
			// add a new tab
			addTab(iter->channel);
		}

		MyGUI::Edit* ec = tabs[iter->channel].txt;
		
		if (!ec->getCaption().empty())
			ec->addText("\n" + iter->txt);
		else
			ec->addText(iter->txt);

		ec->setTextSelection(ec->getTextLength(), ec->getTextLength());

	}
}

void Console::eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	if(_key == MyGUI::KeyCode::Escape || _key == MyGUI::KeyCode::Enum(INPUTENGINE.getKeboardKeyForCommand(EV_COMMON_CONSOLEDISPLAY)))
	{
		setVisible(false);
		// delete last character (to avoid printing `)
		size_t lastChar = mCommandEdit->getTextLength() - 1;
		if (_key != MyGUI::KeyCode::Escape && mCommandEdit->getCaption()[lastChar] == '`')
			mCommandEdit->eraseText(lastChar);
		return;
	}

	switch(_key.toValue())
	{
	case MyGUI::KeyCode::ArrowUp:
		if(current_tab->mHistoryPosition > 0)
		{
			// first we save what we was writing
			if (current_tab->mHistoryPosition == (int)current_tab->mHistory.size() - 1)
			{
				current_tab->mHistory[current_tab->mHistoryPosition] = mCommandEdit->getCaption();
			}
			current_tab->mHistoryPosition--;
			mCommandEdit->setCaption(current_tab->mHistory[current_tab->mHistoryPosition]);
		}
		break;

	case MyGUI::KeyCode::ArrowDown:
		if(current_tab->mHistoryPosition < (int)current_tab->mHistory.size() - 1)
		{
			current_tab->mHistoryPosition++;
			mCommandEdit->setCaption(current_tab->mHistory[current_tab->mHistoryPosition]);
		}
		break;

	case MyGUI::KeyCode::PageUp:
		if (current_tab->txt->getVScrollPosition() > (size_t)current_tab->txt->getHeight())
			current_tab->txt->setVScrollPosition(current_tab->txt->getVScrollPosition() - current_tab->txt->getHeight());
		else
			current_tab->txt->setVScrollPosition(0);
		break;

	case MyGUI::KeyCode::PageDown:
		current_tab->txt->setVScrollPosition(current_tab->txt->getVScrollPosition() + current_tab->txt->getHeight());
		break;
	}
}

void Console::eventCommandAccept(MyGUI::Edit* _sender)
{
	MyGUI::UString command = _sender->getCaption();


	if(current_tab->name.substr(0, 2) == "##" && net)
	{
		// its a channel, send message there

		// send via IRC
		irc_cmd_msg (s, current_tab->name.substr(1).c_str(), command.asUTF8_c_str());

		// add our message to the textbox
		String nick  = net->getNickname(false);
		MyGUI::UString msg = "<"+String(ctx->nick)+"> " + command;
		Console::getInstance().print(msg, current_tab->name);
	}
/*
	// special command
	if(command == "hide")
	{
		setVisible(false);
	}
	else
	{
		print(command, current_tab->name);
#ifdef USE_ANGELSCRIPT
		ScriptEngine::getSingleton().executeString(command);
#endif //ANGELSCRIPT

		//if(netChat) netChat->sendChat(chatline);
	}
*/
	*current_tab->mHistory.rbegin() = command;
	current_tab->mHistory.push_back(""); // new, empty last entry
	current_tab->mHistoryPosition = current_tab->mHistory.size() - 1; // switch to the new line
	mCommandEdit->setCaption(current_tab->mHistory[current_tab->mHistoryPosition]);
}

void Console::eventChangeTab(MyGUI::TabControl* _sender, size_t _index)
{
	MyGUI::TabItemPtr tab = _sender->getItemAt(_index);
	if(!tab) return;
	String n = tab->getCaption();
	if(tabs.find(n) == tabs.end())
		return;

	current_tab = &tabs[n];
}

/*
int irc_output(char* params, irc_reply_data* hostd, void* conn)
{
	Console::getInstance().print(MyGUI::UString(params));
	return 0;
}
*/

void addlog (const char * fmt, ...)
{
	char buf[1024];
	va_list va_alist;

	va_start (va_alist, fmt);
#if defined (WIN32)
	_vsnprintf (buf, sizeof(buf), fmt, va_alist);
#else
	vsnprintf (buf, sizeof(buf), fmt, va_alist);
#endif
	va_end (va_alist);

	Console::getInstance().print(MyGUI::UString(buf), "IRCDebug");
	LOG("IRC| " + String(buf));
	//printf ("%s\n", buf);
}

void dump_event (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	char buf[512];
	unsigned int cnt;

	buf[0] = '\0';

	for ( cnt = 0; cnt < count; cnt++ )
	{
		if ( cnt )
			strcat (buf, "|");

		strcat (buf, params[cnt]);
	}

	addlog ("Event \"%s\", origin: \"%s\", params: %d [%s]", event, origin ? origin : "NULL", cnt, buf);
}

void event_notice (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	if(count != 1) return;
	Console::getInstance().print(MyGUI::UString(params[0]), "IRCStatus");
}

void event_connect (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	irc_ctx_t * ctx = (irc_ctx_t *) irc_get_ctx (session);
	dump_event (session, event, origin, params, count);

	irc_cmd_join (session, ctx->channel, 0);
	irc_cmd_join (session, ctx->channel1, 0);
}

void event_join (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	dump_event (session, event, origin, params, count);
	irc_cmd_user_mode (session, "+i");

	Console::getInstance().print("joined channel #" + String(params[0]),  "#" + String(params[0]));
	//irc_cmd_msg (session, params[0], "Hi all");
}

void event_topic (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	if(count != 2) return;

	MyGUI::UString msg = "*** Topic is " + String(params[1]);
	Console::getInstance().print(msg, "#" + String(params[0]));
}

void event_channel (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	char nickbuf[128];

	if ( count != 2 )
		return;

	MyGUI::UString nick = MyGUI::UString(origin ? origin : "someone");
	if(nick.find("!") != nick.npos)
	{
		nick = nick.substr(0, nick.find("!"));
	}
	MyGUI::UString msg = "<"+nick+"> " + MyGUI::UString(params[1]);
	Console::getInstance().print(msg, "#" + String(params[0]));
	
	/*
	LOG("IRC| " + String(buf));

	addlog ("'%s' said in channel %s: %s\n", 
		origin ? origin : "someone",
		params[0], params[1] );
	*/

	if ( !origin )
		return;

	irc_target_get_nick (origin, nickbuf, sizeof(nickbuf));

	if ( !strcmp (params[1], "quit") )
		irc_cmd_quit (session, "of course, Master!");

	if ( !strcmp (params[1], "help") )
	{
		irc_cmd_msg (session, params[0], "quit, help, dcc chat, dcc send, ctcp");
	}

	if ( !strcmp (params[1], "ctcp") )
	{
		irc_cmd_ctcp_request (session, nickbuf, "PING 223");
		irc_cmd_ctcp_request (session, nickbuf, "FINGER");
		irc_cmd_ctcp_request (session, nickbuf, "VERSION");
		irc_cmd_ctcp_request (session, nickbuf, "TIME");
	}

	/*
	if ( !strcmp (params[1], "dcc chat") )
	{
		irc_dcc_t dccid;
		irc_dcc_chat (session, 0, nickbuf, dcc_recv_callback, &dccid);
		printf ("DCC chat ID: %d\n", dccid);
	}

	if ( !strcmp (params[1], "dcc send") )
	{
		irc_dcc_t dccid;
		irc_dcc_sendfile (session, 0, nickbuf, "irctest.c", dcc_file_recv_callback, &dccid);
		printf ("DCC send ID: %d\n", dccid);
	}
	*/

	if ( !strcmp (params[1], "topic") )
		irc_cmd_topic (session, params[0], 0);
	else if ( strstr (params[1], "topic ") == params[1] )
		irc_cmd_topic (session, params[0], params[1] + 6);

	if ( strstr (params[1], "mode ") == params[1] )
		irc_cmd_channel_mode (session, params[0], params[1] + 5);

	if ( strstr (params[1], "nick ") == params[1] )
		irc_cmd_nick (session, params[1] + 5);

	if ( strstr (params[1], "whois ") == params[1] )
		irc_cmd_whois (session, params[1] + 5);
}

void event_privmsg (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	dump_event (session, event, origin, params, count);

	printf ("'%s' said me (%s): %s\n", 
		origin ? origin : "someone",
		params[0], params[1] );
}

void event_numeric (irc_session_t * session, unsigned int eventNum, const char * origin, const char ** params, unsigned int count)
{
	char buf[24];
	sprintf (buf, "%d", eventNum);

	if(eventNum == 433)
	{

		irc_ctx_t * ctx = (irc_ctx_t *) irc_get_ctx (session);
		ctx->nickRetry++;
		sprintf(ctx->nick, "%s_", ctx->nick);

		// now change the nick
		char buf[1024] = "";
		sprintf (buf, "NICK %s", ctx->nick);
		irc_send_raw (session, buf);

		// and rejoin the channels
		irc_cmd_join (session, ctx->channel, 0);
		irc_cmd_join (session, ctx->channel1, 0);
	}

	dump_event (session, buf, origin, params, count);
}


void *s_ircthreadstart(void* arg)
{
	irc_ctx_t *ctx = (irc_ctx_t *)arg;
#ifdef WIN32
	WORD wVersionRequested = MAKEWORD (1, 1);
	WSADATA wsaData;

	if ( WSAStartup (wVersionRequested, &wsaData) != 0 )
	{
		LOG("IRC| failed to init IRC socket");
		return 0;
	}
#endif //WIN32

	irc_callbacks_t	callbacks;

	memset (&callbacks, 0, sizeof(callbacks));

	callbacks.event_connect = event_connect;
	callbacks.event_join = event_join;
	callbacks.event_nick = dump_event;
	callbacks.event_quit = dump_event;
	callbacks.event_part = dump_event;
	callbacks.event_mode = dump_event;
	callbacks.event_topic = event_topic;
	callbacks.event_kick = dump_event;
	callbacks.event_channel = event_channel;
	callbacks.event_privmsg = event_privmsg;
	callbacks.event_notice = event_notice;
	callbacks.event_invite = dump_event;
	callbacks.event_umode = dump_event;
	callbacks.event_ctcp_rep = dump_event;
	callbacks.event_ctcp_action = dump_event;
	callbacks.event_unknown = dump_event;
	callbacks.event_numeric = event_numeric;

	//callbacks.event_dcc_chat_req = irc_event_dcc_chat;
	//callbacks.event_dcc_send_req = irc_event_dcc_send;

	s = irc_create_session (&callbacks);

	if ( !s )
	{
		LOG ("Could not create session");
		return 0;
	}


	irc_set_ctx (s, ctx);

	if ( irc_connect (s, "irc.rigsofrods.org", 6667, 0, ctx->nick, ctx->nick, ctx->nick) )
	{
		LOG ("IRC| Could not connect: " + String(irc_strerror (irc_errno(s))));
		return 0;
	}

	irc_run (s);
	return 0;
}

void Console::initIRC(irc_ctx_t *ctx)
{
	pthread_t ircthread;
	pthread_create(&ircthread, NULL, s_ircthreadstart, (void *)ctx);
}

void Console::setNetwork(Network *n)
{
	net = n;

	// we need presistent memory, so alloc
	ctx = (irc_ctx_t *)malloc(sizeof(irc_ctx_t));
	ctx->channel  = (char *)"#rigsofrods";
	ctx->channel1 = (char *)"#server1";
	ctx->nickRetry=0;


	strcpy(ctx->nick, net->getNickname(false).c_str());


	initIRC(ctx);
};

#endif //MYGUI

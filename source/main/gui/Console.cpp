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
#include "Scripting.h"
#include "InputEngine.h"
#include "OgreLogManager.h"
#include "gui_manager.h"

#include "Settings.h"
#include "RoRFrameListener.h"
#include "network.h"

#include "language.h"

#include "libircclient.h"

// hack, hacky but works well
irc_session_t *s   = 0;
irc_ctx_t     *ctx = 0;

// class
Console::Console() : net(0), autoCompleteNum(0)
{
	initialiseByAttributes(this);

	// make it 1/3 of screen height and place over screen

	/*
	MyGUI::IntSize size = MyGUI::RenderManager::getInstance().getViewSize();
	size.height = size.height/3;
	mMainWidget->setCoord(0, -size.height, size.width, size.height);
	*/

	((MyGUI::Window*)mMainWidget)->setCaption(_L("Console"));

	MyGUI::Window *wnd = dynamic_cast<MyGUI::Window *>(mMainWidget);
	wnd->eventWindowButtonPressed += MyGUI::newDelegate(this, &Console::notifyWindowXPressed);

	mCommandEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &Console::eventButtonPressed);
	mCommandEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &Console::eventCommandAccept);
	mTabControl->eventTabChangeSelect   += MyGUI::newDelegate(this, &Console::eventChangeTab);

	bool enable_ingame_console = BSETTING("Enable Ingame Console");

	if(enable_ingame_console)
	{
		addTab("OgreLog");
	
		//addTab("IRCDebug");
#ifdef USE_ANGELSCRIPT
		addTab("Angelscript");
#endif //ANGELSCRIPT


		// BUG: all editboxes visible on startup D:
		mTabControl->selectSheetIndex(0, false);

		current_tab = &tabs["OgreLog"];
	}


	setVisible(false);

	MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate( this, &Console::frameEntered );

	if(enable_ingame_console)
		Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this);
}

Console::~Console()
{
	Ogre::LogManager::getSingleton().getDefaultLog()->removeListener(this);
	MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate( this, &Console::frameEntered );
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

	if(!_visible)
	{
		autoCompleteTemp.clear();
		autoCompleteNum = 0;
	}

	mMainWidget->setEnabledSilent(_visible);
	mMainWidget->setVisible(_visible);
	if (_visible)
	{
		MyGUI::InputManager::getInstance().setKeyFocusWidget(mCommandEdit);
	}
	else
	{
		MyGUI::InputManager::getInstance().resetKeyFocusWidget(mCommandEdit);
		GUIManager::getSingleton().unfocus();
	}
}

bool Console::getVisible()
{
	return mVisible;
}

void Console::print(const MyGUI::UString &_text, Ogre::String channel)
{
	msg_t t;
	t.txt = _text;
	t.channel = channel; 
	push(t);
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

#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
void Console::messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName)
#else
void Console::messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName, bool& skipThisMessage)
#endif // OGRE_VERSION
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
	int results = pull(tmpWaitingMessages);

	if (results == 0)
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

void Console::notifyWindowXPressed(MyGUI::WidgetPtr _widget, const std::string& _name)
{
	if (_name == "close")
	{
		MyGUI::WindowPtr window = dynamic_cast<MyGUI::WindowPtr>(_widget);
		window->hideSmooth();
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

	case MyGUI::KeyCode::Tab:
		{
			// yay, auto-completion :)
#ifdef USE_ANGELSCRIPT
			if(current_tab->name == "Angelscript")
			{
				String command = mCommandEdit->getCaption();
				
				//if(autoCompleteTemp.empty())
				autoCompleteTemp = ScriptEngine::getSingleton().getAutoComplete(command);
				
				if(!autoCompleteTemp.empty())
				{
					autoCompleteNum++;
					if(autoCompleteNum >= autoCompleteTemp.size()) autoCompleteNum = 0;
					// TODO: improve
					Console::getInstance().print(">>> " + autoCompleteTemp[autoCompleteNum], current_tab->name);
				}
			}
#endif //ANGELSCRIPT
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

	if(current_tab->name == "Angelscript")
	{
#ifdef USE_ANGELSCRIPT
		Console::getInstance().print(">>> " + command, current_tab->name);
		int res = ScriptEngine::getSingleton().executeString(command);
		autoCompleteTemp.clear();
		autoCompleteNum = 0;
#endif //ANGELSCRIPT
	}

	//if(netChat) netChat->sendChat(chatline);

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

	bool enabled = true;
	if(n == "OgreLog")  enabled = false;
	if(n == "IRCDebug") enabled = false;
	mCommandEdit->setEnabled(enabled);

	current_tab = &tabs[n];
}


void Console::setNetwork(Network *n)
{
	net = n;

	// we need persistent memory, so alloc
	ctx = (irc_ctx_t *)malloc(sizeof(irc_ctx_t));
	ctx->channel  = (char *)"#rigsofrods";
	ctx->nickRetry=0;


	strcpy(ctx->nick, net->getNickname(false).c_str());

	String servername = "#" + SSETTING("Server name") + ":" + SSETTING("Server port");
	strcpy(ctx->channel1, servername.c_str());


	//initIRC(ctx);
};

#endif //MYGUI

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
#include "gui_menu.h"
#include "OverlayWrapper.h"
#include "ChatSystem.h"

#include "Settings.h"
#include "RoRFrameListener.h"
#include "network.h"

#include "Beam.h"
#include "BeamFactory.h"

#include "language.h"
#include "utils.h"

#include "libircclient.h"

#if MYGUI_PLATFORM == MYGUI_PLATFORM_LINUX
#include <iconv.h>
#endif // LINUX

// class
Console::Console() : net(0), netChat(0), top_border(20), bottom_border(100), message_counter(0), mHistory(), mHistoryPosition(0), inputMode(false), linesChanged(false), scrollOffset(0), linecount(10), scroll_size(5)
{
	mMainWidget = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("default", 0, 0, 400, 300,  MyGUI::Align::Center, "Back", "Console");
	mMainWidget->setCaption(_L("Console"));
	mMainWidget->setAlpha(0.9f);

	memset(&lines, 0, sizeof(lines));

	mHistory.push_back(MyGUI::UString());

	// and the textbox inside
	mCommandEdit = mMainWidget->createWidget<MyGUI::EditBox>("EditBoxChat", 0, 0, 304, lineheight * 1.2f,  MyGUI::Align::Default, "ConsoleInput");
	
	mCommandEdit->setProperty("WordWrap", "false");
	mCommandEdit->setProperty("TextAlign", "Left Bottom");
	mCommandEdit->setProperty("ReadOnly", "false");
	mCommandEdit->setProperty("OverflowToTheLeft", "true");
	mCommandEdit->setProperty("MaxTextLength", "8192");
	mCommandEdit->setWidgetStyle(MyGUI::WidgetStyle::Child);
	mCommandEdit->setCaption("");
	mCommandEdit->setEnabled(false);
	mCommandEdit->setVisible(false);

	mCommandEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &Console::eventButtonPressed);
	mCommandEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &Console::eventCommandAccept);

	// scroll icons
	scrollImgUp = mMainWidget->createWidget<MyGUI::ImageBox>("ChatIcon", 0, 0, lineheight, lineheight * 2,  MyGUI::Align::Default, "ConsoleIconScrollUp");
	scrollImgUp->setProperty("ImageTexture", "arrow_up.png");
	scrollImgUp->setWidgetStyle(MyGUI::WidgetStyle::Child);
	scrollImgUp->setVisible(false);
	MyGUI::RotatingSkin *rotatingIcon = scrollImgUp->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
	rotatingIcon->setCenter(MyGUI::IntPoint(lineheight*0.5f,lineheight*0.5f));
	rotatingIcon->setAngle(Degree(90).valueRadians());

	scrollImgDown = mMainWidget->createWidget<MyGUI::ImageBox>("ChatIcon", 0, 0, lineheight, lineheight * 2,  MyGUI::Align::Default, "ConsoleIconScrollDown");
	scrollImgDown->setProperty("ImageTexture", "arrow_down.png");
	scrollImgDown->setWidgetStyle(MyGUI::WidgetStyle::Child);
	scrollImgDown->setVisible(false);
	rotatingIcon = scrollImgDown->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
	rotatingIcon->setCenter(MyGUI::IntPoint(lineheight*0.5f,lineheight*0.5f));
	rotatingIcon->setAngle(Degree(90).valueRadians());


	// the rest
	setVisible(false);

	MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate( this, &Console::frameEntered );

	Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this);
}

Console::~Console()
{
	Ogre::LogManager::getSingleton().getDefaultLog()->removeListener(this);
	MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate( this, &Console::frameEntered );
}


void Console::setVisible(bool _visible)
{
	mMainWidget->setEnabledSilent(_visible);
	mMainWidget->setVisible(_visible);

	// DO NOT change focus here

	if(!_visible)
	{
		inputMode = false;
	}
}

bool Console::getVisible()
{
	return mMainWidget->getVisible();
}


void Console::select()
{
	MyGUI::InputManager::getInstance().setKeyFocusWidget(mCommandEdit);
	mCommandEdit->setEnabled(true);
	mCommandEdit->setVisible(true);
	inputMode = true;
	linesChanged = true;
}


void Console::unselect()
{
	MyGUI::InputManager::getInstance().resetKeyFocusWidget();
	GUIManager::getSingleton().unfocus();
}


void Console::frameEntered(float dt)
{
	messageUpdate(dt);

	updateGUIVisual(dt);
}

void Console::eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	switch(_key.toValue())
	{
	case MyGUI::KeyCode::ArrowUp:
		{
			if(mHistoryPosition > 0)
			{
				if (mHistoryPosition == (int)mHistory.size() - 1)
					mHistory[mHistoryPosition] = mCommandEdit->getCaption();
				mHistoryPosition--;
				mCommandEdit->setCaption(mHistory[mHistoryPosition]);
			}
		}
		break;
	case MyGUI::KeyCode::ArrowDown:
		{
			if(mHistoryPosition < (int)mHistory.size() - 1)
			{
				mHistoryPosition++;
				mCommandEdit->setCaption(mHistory[mHistoryPosition]);
			}
		}
		break;
	case MyGUI::KeyCode::PageUp:
		{
			if(scrollOffset + scroll_size <= (message_counter - linecount))
			{
				scrollOffset += scroll_size;
				linesChanged = true;
			}
		}
		break;
	case MyGUI::KeyCode::PageDown:
		{
			scrollOffset -= scroll_size;
			if(scrollOffset < 0) scrollOffset = 0;
			linesChanged = true;
		}
		break;
	}
}

void Console::eventCommandAccept(MyGUI::Edit* _sender)
{
	MyGUI::UString msg = _sender->getCaption();
	_sender->setCaption("");

	// unfocus, so we return to the main game for the keyboard focus
	inputMode = false;
	linesChanged = true;
	scrollOffset = 0; // reset offset
	unselect();
	mCommandEdit->setEnabled(false);
	mCommandEdit->setVisible(false);

	if(msg.empty())
	{
		// discard the empty message
		return;
	}

	// record the history
	*mHistory.rbegin() = msg;
	mHistory.push_back(""); // new, empty last entry
	mHistoryPosition = mHistory.size() - 1; // switch to the new line
	mCommandEdit->setCaption(mHistory[mHistoryPosition]);

	// scripting?
#ifdef USE_ANGELSCRIPT
	if(msg[0] == '\\')
	{
		String command = msg.substr(1);

		Ogre::StringUtil::trim(command);
		if(command.empty()) return;

		String nmsg = ">>> " + command;
		putMessage(CONSOLE_MSGTYPE_SCRIPT, nmsg, "user_comment.png");
		int res = ScriptEngine::getSingleton().executeString(command);
		return;
	}
#endif //ANGELSCRIPT

	// some specials
	if(msg == "/pos")
	{
		outputCurrentPosition();
		return;
	} else if(msg == "/help")
	{
		putMessage(CONSOLE_MSGTYPE_INFO, ChatSystem::commandColour + _L("possible commands:"), "help.png");
		putMessage(CONSOLE_MSGTYPE_INFO, _L("#dd0000/help#000000 - this help information"), "help.png");
		putMessage(CONSOLE_MSGTYPE_INFO, _L("#dd0000/ver#000000  - shows the Rigs of Rods version"), "information.png");
		putMessage(CONSOLE_MSGTYPE_INFO, _L("#dd0000/pos#000000  - outputs the current position"), "world.png");
		putMessage(CONSOLE_MSGTYPE_INFO, _L("#dd0000/save#000000 - saves the chat history to a file"), "table_save.png");
		putMessage(CONSOLE_MSGTYPE_INFO, _L("#dd0000/log#000000  - toggles log output on the console"), "table_save.png");
		putMessage(CONSOLE_MSGTYPE_INFO, _L("#dd0000/quit#000000 - exits"), "table_save.png");
		putMessage(CONSOLE_MSGTYPE_INFO, ChatSystem::commandColour + _L("tips:"), "help.png");
		putMessage(CONSOLE_MSGTYPE_INFO, _L("- use #dd0000Arrow Up/Down Keys#000000 in the InputBox to reuse old messages"), "information.png");
		putMessage(CONSOLE_MSGTYPE_INFO, _L("- use #dd0000Page Up/Down Keys#000000 in the InputBox to scroll through the history"), "information.png");
		return;

	} else if(msg == "/ver")
	{
		putMessage(CONSOLE_MSGTYPE_INFO, ChatSystem::commandColour + getVersionString(false), "information.png");
		return;
	} else if(msg == "/quit")
	{
		RoRFrameListener::eflsingleton->shutdown_final();
		return;
	} else if(msg == "/save")
	{
		saveChat(SSETTING("Log Path") + "chat-log.txt");
		return;
	} else if(msg == "/log")
	{
		// switch to console logging
		bool logging = BSETTING("Enable Ingame Console");
		if(!logging)
		{
			putMessage(CONSOLE_MSGTYPE_INFO, ChatSystem::commandColour + _L(" logging to console enabled"), "information.png");
			SETTINGS.setSetting("Enable Ingame Console", "Yes");
		} else
		{
			putMessage(CONSOLE_MSGTYPE_INFO, ChatSystem::commandColour + _L(" logging to console disabled"), "information.png");
			SETTINGS.setSetting("Enable Ingame Console", "No");
		}
		return;
		
	// some debugging things below ;)
	} else if(msg == "/test1")
	{
		for(int i=0; i<600; i++)
			putMessage(CONSOLE_MSGTYPE_INFO, "TEST " + TOSTRING(i), "cog.png");
		return;
	} else if(msg == "/test2")
	{
		for(int i=0; i<MESSAGES_MAX*3; i++)
			putMessage(CONSOLE_MSGTYPE_INFO, "OVERFLOW_TEST " + TOSTRING(i) + " / size: " + TOSTRING(size()), "cog.png");
		return;
	} else if(msg == "/fadetest")
	{
		for(int i=0; i<10; i++)
			putMessage(CONSOLE_MSGTYPE_INFO, "FADE-TEST: down " + TOSTRING(i), "cog.png", i*1000);
		for(int i=0; i<10; i++)
			putMessage(CONSOLE_MSGTYPE_INFO, "FADE-TEST: up " + TOSTRING(10-i), "cog.png", (10-i)*1000);
		return;
	}

	// network chat
	if(net && netChat)
	{
		netChat->sendChat(msg.c_str());

		String nmsg = net->getNickname(true) + String("#000000: ") + msg;
		putMessage(CONSOLE_MSGTYPE_NETWORK, nmsg, "user_comment.png");
		return;
	}
}

void Console::setNetwork(Network *n)
{
	net = n;
}

void Console::setNetChat(ChatSystem *c)
{
	netChat = c;
}

std::string ansi_to_utf16(const char* srcPtr)
{
	// TODO: fix UTF8 handling
	return std::string(srcPtr);
#if 0
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
	int tmpSize = MultiByteToWideChar( CP_ACP, 0, srcPtr, -1, 0, 0 );
	WCHAR* tmpBuff = new WCHAR [ tmpSize + 1 ];
	MultiByteToWideChar( CP_ACP, 0, srcPtr, -1, tmpBuff, tmpSize );
	std::wstring ret = tmpBuff;
	delete[] tmpBuff;
	return ret;
#else
	// http://www.lemoda.net/c/iconv-example/iconv-example.html
	
	//iconv_t conv_desc iconv_open ("ANSI", "UTF16");
	//if ((int) conv_desc == -1) return std::wstring();

	// TODO: fix: use iconv to convert the string similar to the windows implementation above

	return std::string(srcPtr);
#endif
#endif //0
}

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
	{
		msg = message.substr(4);
		//putMessage(CONSOLE_MSGTYPE_SCRIPT, MyGUI::UString("#988310") + ansi_to_utf16(msg), "bricks.png");
		putMessage(CONSOLE_MSGTYPE_SCRIPT, String("#988310") + ansi_to_utf16(msg.c_str()), "page_white_code.png");
	} else
	{
		if(BSETTING("Enable Ingame Console"))
		{
			//putMessage(CONSOLE_MSGTYPE_LOG, MyGUI::UString("#988310") + ansi_to_utf16(msg), "book_open.png");
			if(lml == LML_NORMAL)
				putMessage(CONSOLE_MSGTYPE_LOG, String("#988310") + ansi_to_utf16(msg.c_str()), "script_error.png");
			else if(lml == LML_TRIVIAL)
				putMessage(CONSOLE_MSGTYPE_LOG, String("#988310") + ansi_to_utf16(msg.c_str()), "script.png");
			else if(lml == LML_CRITICAL)
				putMessage(CONSOLE_MSGTYPE_LOG, String("#988310") + ansi_to_utf16(msg.c_str()), "script_lightning.png");
		}
	}
}

void Console::resized()
{
	GUI_MainMenu *menu = GUI_MainMenu::getSingletonPtr();
	if(menu) top_border = menu->getMenuHeight();

	MyGUI::IntSize size = MyGUI::RenderManager::getInstance().getViewSize();
	
	// 15% of the window height is the overlay
	OverlayWrapper *ow = OverlayWrapper::getSingletonPtr();
	if(ow) bottom_border = size.height - ow->getDashBoardHeight() + 20;

	int height = size.height - bottom_border - top_border;
	int width  = size.width;
	mMainWidget->setCoord(0, top_border, width, size.height - bottom_border);
	
	linecount = height / (float)lineheight;
	if(linecount >= LINES_MAX)
		linecount = LINES_MAX - 1;

	scroll_size = linecount * 0.5f;

	// add controls
	for(unsigned int i = 0; i < linecount; i++)
	{
		if(lines[i].txtctrl)
		{
			// do not set visibility here, we do that in updateGUILines()
			lines[i].txtctrl->setSize(width, lines[i].txtctrl->getHeight());
			continue;
		}
		mygui_console_line_t line;
		memset(&line, 0, sizeof(line));
		line.number = i;

		line.txtctrl = mMainWidget->createWidget<MyGUI::TextBox>("TextBoxChat", lineheight + 2, i*lineheight, width, lineheight,  MyGUI::Align::Left | MyGUI::Align::Right | MyGUI::Align::Top, "ConsoleLine" + TOSTRING(i));
		line.txtctrl->setWidgetStyle(MyGUI::WidgetStyle::Child);
		line.txtctrl->setCaption("> LINE " + TOSTRING(i) + " >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
		line.txtctrl->setVisible(false);

		line.iconctrl = mMainWidget->createWidget<MyGUI::ImageBox>("ChatIcon", 0, i*lineheight - 2, lineheight, lineheight,  MyGUI::Align::Default, "ConsoleIcon"+TOSTRING(i));
		line.iconctrl->setProperty("ImageTexture", "arrow_left.png");
		line.iconctrl->setWidgetStyle(MyGUI::WidgetStyle::Child);
		line.iconctrl->setVisible(false);
		MyGUI::ISubWidget* waitDisplaySub = line.iconctrl->getSubWidgetMain();
		MyGUI::RotatingSkin *rotatingIcon = waitDisplaySub->castType<MyGUI::RotatingSkin>();
		rotatingIcon->setCenter(MyGUI::IntPoint(lineheight*0.5f,lineheight*0.5f));
		
		// funny but resource hungry
		rotatingIcon->setAngle(Degree(90).valueRadians());
		//rotatingIcon->setAngle(Degree(Math::RangeRandom(0, 360)).valueRadians());
		lines[i] = line;
	}
	// hide the rest of the lines which are not visible anyways
	for(unsigned int i = linecount; i < LINES_MAX; i++)
	{
		if(!lines[i].txtctrl) break;
		lines[i].txtctrl->setVisible(false);
		lines[i].iconctrl->setVisible(false);
	}

	// resize the rest
	mCommandEdit->setCoord(0, linecount*lineheight, width, lineheight * 1.2f);
	scrollImgUp->setPosition(width * 0.3f, 2);
	scrollImgDown->setPosition(width * 0.3f, linecount * lineheight);

	// trigger lines update
	linesChanged = true;
}

void Console::updateGUILines( float dt )
{
	// update GUI
	// bottom line is last entry always
	int msgi = 0;
	int ctrli = linecount - 1;

	while(ctrli >= 0)
	{
		if(!lines[ctrli].txtctrl) break; // text control missing?!

		int msgid = message_counter - msgi - 1 - scrollOffset;
		if(msgid < 0)
		{
			// hide this entry, its empty
			lines[ctrli].txtctrl->setVisible(false);
			lines[ctrli].iconctrl->setVisible(false);
			ctrli--;
			msgi++;
			continue;
		}
		if(msgid >= (int)message_counter)
		{
			// no messages left
			break;
		}

		msg_t &m = messages[msgid];

		// check if TTL expired
		unsigned long t = Ogre::Root::getSingleton().getTimer()->getMilliseconds() - m.time;
		if(t > m.ttl && !inputMode)
		{
			// expired, take the next message instead, when not in input mode
			msgi++;
			continue;
		}

		// not empty, not expired, add content
		lines[ctrli].txtctrl->setVisible(true);
		lines[ctrli].iconctrl->setVisible(true);


		MyGUI::UString txt = ansi_to_utf16(m.txt);

		lines[ctrli].txtctrl->setCaption(txt);
		lines[ctrli].iconctrl->setProperty("ImageTexture", std::string(m.icon));
		lines[ctrli].expired = false;

		lines[ctrli].msg = &messages[msgid];
		
		ctrli--;
		msgi++;
	}
	
	linesChanged = false;
}

void Console::updateGUIVisual( float dt )
{
	for(unsigned int i = 0; i < linecount; i++)
	{
		if(lines[i].expired) continue;
		if(lines[i].iconctrl && lines[i].msg)
		{
			
			// rotating icons - funny but resource hungry
			//MyGUI::RotatingSkin* rotatingIcon = lines[i].iconctrl->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
			//rotatingIcon->setAngle(rotatingIcon->getAngle() + Math::PI / 360.0f);

			unsigned long ot = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
			unsigned long t = ot - lines[i].msg->time;
			if(t > lines[i].msg->ttl && !inputMode)
			{
				// expired
				lines[i].txtctrl->setVisible(false);
				lines[i].iconctrl->setVisible(false);
				lines[i].expired = true;
				lines[i].txtctrl->setCaption(lines[i].txtctrl->getCaption() + " EXPIRED");
				linesChanged = true;
				continue;
			}

			float alpha = 1.0f;
			if(!inputMode)
			{
				// logic
				unsigned long endTime   = lines[i].msg->time + lines[i].msg->ttl;
				const float fadeTime    = 2000.0f;
				unsigned long startTime = endTime - (long)fadeTime;

				if(ot < startTime)
				{
					alpha = 1.0f;
				} else 
				{
					alpha = 1 - ((ot - startTime) / fadeTime);
				}
			} else
			{
				// different logic in input mode: display all messages
				alpha = 0.9f;
			}

			// set the alpha
			lines[i].txtctrl->setAlpha (alpha);
			lines[i].iconctrl->setAlpha(alpha);
		}
	}

	// show/hide the scroll icons
	scrollImgDown->setVisible(scrollOffset > 0 && inputMode);
	scrollImgUp->setVisible(scrollOffset < (message_counter - linecount) && (message_counter - linecount) > 0  && inputMode);
}


int Console::messageUpdate( float dt )
{
	// collect the waiting messages and handle them
	std::vector<msg_t> tmpWaitingMessages;
	int results = pull(tmpWaitingMessages);

	// nothing to add?
	int r = 0;
	if (results > 0)
	{
		for (int i = 0; i < results; i++, r++)
		{
			// copy over to our storage
			messages[message_counter] = tmpWaitingMessages[i];
		
			// increase pointer and overwrite oldest if overflown
			message_counter++;
			if(message_counter >= MESSAGES_MAX)
				message_counter = 0;
		}
		// new lines, update them
		updateGUILines(dt);
	}
	else if (results == 0 && linesChanged)
	{
		// in inputmode we display the last lines, so change them
		updateGUILines(dt);
	}

	return r;
}

void Console::putMessage( int type, Ogre::String txt, Ogre::String icon, unsigned long ttl )
{
	msg_t t;

	t.type    = type;
	t.time    = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
	t.ttl     = ttl;
	strncpy(t.txt,  txt.c_str(), 2048);
	strncpy(t.icon, icon.c_str(), 50);
	//t.channel = "default";

	push(t);
}

void Console::saveChat(String filename)
{
	FILE *f = fopen(filename.c_str(), "a");
	if(!f)
	{
		putMessage(CONSOLE_MSGTYPE_INFO, ChatSystem::commandColour + "Unable to open file " + filename, "error.png");
		return;
	}
	fprintf(f, "==== \n");
	for(unsigned int i = 0; i < message_counter; i++)
	{
		fprintf(f, "%ld %s\n", messages[i].time, messages[i].txt);
	}
	fclose(f);
	putMessage(CONSOLE_MSGTYPE_INFO, ChatSystem::commandColour + "History saved as " + filename, "table_save.png");
}

void Console::outputCurrentPosition()
{
	Beam *b = BeamFactory::getSingleton().getCurrentTruck();
	if(!b && RoRFrameListener::eflsingleton->person)
	{
		Vector3 pos = RoRFrameListener::eflsingleton->person->getPosition();
		putMessage(CONSOLE_MSGTYPE_INFO, _L("Character position: ") + String("#dd0000") + TOSTRING(pos.x) +  String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
	}
	else
	{
		Vector3 pos = b->getPosition();
		putMessage(CONSOLE_MSGTYPE_INFO, _L("Vehicle position: ") + String("#dd0000") + TOSTRING(pos.x) +  String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
	}
}

#endif //MYGUI


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

#ifndef __INGAMECONSOLE_H__
#define __INGAMECONSOLE_H__

#include <vector>
#include "Ogre.h"
#include "OgreTextAreaOverlayElement.h"
#include "ColoredTextAreaOverlayElement.h"

//#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
// broken on all platforms!
#define COLOROVERLAYWORKAROUND
//#endif

#define CONSOLE IngameConsole::getInstance()

enum {CONSOLE_LEFT_SMALL, CONSOLE_LEFT_FULL, CONSOLE_TOP, CONSOLE_END};
enum {CONSOLE_MSG_CHAT, CONSOLE_MSG_COMMAND};

class ExampleFrameListener;

class IngameConsole
{
public:
	bool update(float dt);
	void setMode(ExampleFrameListener *mefl, int mode, bool visible);
	int getMode();
	int toggleMode(ExampleFrameListener *mefl);
	void toggleVisible(ExampleFrameListener *mefl);
	static IngameConsole & getInstance();
	~IngameConsole();
	bool addText(Ogre::String msg, bool addtime=false);
	bool setEnterText(Ogre::String msg, bool visible=true, bool cursor=false);
	void resize(int left, int top, int width, int height);
	bool getVisible() { return isvisible; };
	void setVisible(bool visible);

	int getMessageType(Ogre::String msg);
	int parseCommand(Ogre::String &msg, std::vector<Ogre::String> &arguments);

	void scrollPageUp();
	void scrollPageDown();
	void noScroll();

protected:
	IngameConsole();
	IngameConsole(const IngameConsole&);
	IngameConsole& operator= (const IngameConsole&);

private:
    static IngameConsole *myInstance;

	int top, left, width, height;
	int lineheight, bordersize;
	int displaymode;
	ExampleFrameListener *mefl;
	
	// cursor stuff
	Ogre::String enterText;
	bool cursorBlink;
	bool cursorBlinkState;
	float lastCursorBlink;

	//scroll stuff
	int scrollOffset;
	bool scrolling;

	bool isvisible;
	bool createOverlays();
	bool destroyOverlays();
	void updateEnterText();

	Ogre::OverlayContainer *consoleOverlay;
#ifdef COLOROVERLAYWORKAROUND
	std::vector < Ogre::TextAreaOverlayElement * > chatLines;
#else
	std::vector < ColoredTextAreaOverlayElement * > chatLines;
#endif
	std::vector < Ogre::String > chatBuffer;
	void updateDisplay();

	void splitString(Ogre::String &str, Ogre::String &delim, std::vector<Ogre::String> &output);
};


#endif

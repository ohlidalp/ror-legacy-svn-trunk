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

#ifndef __INGAMECHAT_H__
#define __INGAMECHAT_H__


#include <vector>
#include "Ogre.h"
#include "OgreTextAreaOverlayElement.h"
#include "ColoredTextAreaOverlayElement.h"

#define NETCHAT IngameConsole::getInstance()

enum {NETCHAT_LEFT_SMALL, NETCHAT_LEFT_FULL, NETCHAT_TOP, NETCHAT_END};
enum {NETCHAT_MSG_CHAT, NETCHAT_MSG_COMMAND};

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

	int parseCommand(Ogre::String &msg, std::vector<Ogre::String> &arguments);

	void scrollPageUp();
	void scrollPageDown();
	void noScroll();

	bool getScriptMode() { return scriptMode; };
	void setScriptMode(bool value);

protected:
	IngameConsole();
	IngameConsole(const IngameConsole&);
	IngameConsole& operator= (const IngameConsole&);

private:
    static IngameConsole *myInstance;

	int top, left, width, height;
	int lineheight, bordersize;
	int displaymode;
	bool scriptMode;
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
	std::vector < ColoredTextAreaOverlayElement * > chatLines;
	std::vector < Ogre::String > chatBuffer;
	void updateDisplay();

	void splitString(Ogre::String &str, Ogre::String &delim, std::vector<Ogre::String> &output);
};


#endif

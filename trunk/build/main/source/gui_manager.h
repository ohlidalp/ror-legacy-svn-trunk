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
#ifndef __GUI_MAIN_H__
#define __GUI_MAIN_H__

// thomas fischer, thomas@thomasfischer.biz, 3rd June 2008

#include "Ogre.h"

#include <vector>
#include <map>
//#include <pthread.h>

#include "OISEvents.h"
#include "OISInputManager.h"
#include "OISMouse.h"
#include "OISKeyboard.h"
#include "OISJoyStick.h"

#include "MyGUI.h"

//forward declarations
namespace MyGUI { class Gui; };

#define MYGUI GUIManager::Instance()
#define GETMYGUI GUIManager::Instance().getGUI()

class GUIManager
{
protected:
	MyGUI::Gui *mGUI;

	GUIManager();
	~GUIManager();
	GUIManager(const GUIManager&);
	GUIManager& operator= (const GUIManager&);
	static GUIManager* myInstance;
	bool initialized;
	bool enabled;

public:
	static GUIManager &Instance();
	
	void setup(Ogre::Camera *cam, Ogre::SceneManager *scm, Ogre::RenderWindow *mWindow);
	

	void shutdown();

	void activate();
	void desactivate();
	
	void setCursorPosition(float x, float y);
	void setCamera(Ogre::Camera *mCamera);
	
	// window events
	void windowResized(Ogre::RenderWindow *rw);
	void windowMoved(Ogre::RenderWindow *rw);

	// engine / input events
	bool frameStarted(const Ogre::FrameEvent& evt);
	bool frameEnded(const Ogre::FrameEvent& evt);
	bool mouseMoved( const OIS::MouseEvent &arg );
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );


	// new user functions
	MyGUI::Gui *getGUI() { return mGUI; };
	void loadLayout(Ogre::String name);
};


#endif

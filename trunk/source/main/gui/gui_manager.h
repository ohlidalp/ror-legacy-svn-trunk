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

#ifndef GUI_MANAGER_H__
#define GUI_MANAGER_H__

#include "RoRPrerequisites.h"
#include "gui_inputmanager.h"
#include "Singleton.h"

#include <Ogre.h>
#include <MyGUI.h>

#define GETMYGUI GUIManager::getSingleton().getGUI()

namespace MyGUI { class OgrePlatform; }

class GUIManager :
	  public RoRSingletonNoCreation<GUIManager>
	, public GUIInputManager
	, public Ogre::FrameListener
	, public Ogre::WindowEventListener
{
public:
	GUIManager(Ogre::Root *root, Ogre::SceneManager *mgr, Ogre::RenderWindow *win);
	virtual ~GUIManager();

	void destroy();

	MyGUI::Gui* getGUI() { return mGUI; }

	void unfocus();

	static Ogre::String getRandomWallpaperImage();

	void windowResized(Ogre::RenderWindow* _rw);
protected:
private:
	bool create();
	void createGui();
	void destroyGui();

	virtual bool frameStarted(const Ogre::FrameEvent& _evt);
	virtual bool frameEnded(const Ogre::FrameEvent& _evt);
	virtual void windowClosed(Ogre::RenderWindow* _rw);

	void eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result);


private:
	MyGUI::Gui* mGUI;
	MyGUI::OgrePlatform* mPlatform;

	Ogre::Root *mRoot;
	Ogre::SceneManager* mSceneManager;
	Ogre::RenderWindow* mWindow;

	bool mExit;

	Ogre::String mResourceFileName;
};

#endif // GUI_MANAGER_H__

#endif //MYGUI
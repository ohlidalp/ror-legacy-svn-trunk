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

#ifndef __GUI_Manager_H_
#define __GUI_Manager_H_

#include "RoRPrerequisites.h"

#include "GUIInputManager.h"
#include "Ogre.h"
#include "Singleton.h"

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

	GUIManager();
	virtual ~GUIManager();

	void destroy();

	MyGUI::Gui* getGUI() { return mGUI; }

	void unfocus();

	static Ogre::String getRandomWallpaperImage();

	void windowResized();

private:

	bool create();
	void createGui();
	void destroyGui();

	virtual bool frameStarted(const Ogre::FrameEvent& _evt);
	virtual bool frameEnded(const Ogre::FrameEvent& _evt);
	virtual void windowClosed();

	void eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result);


private:

	MyGUI::Gui* mGUI;
	MyGUI::OgrePlatform* mPlatform;

	bool mExit;

	Ogre::String mResourceFileName;
};

#endif // __GUI_Manager_H_

#endif // USE_MYGUI

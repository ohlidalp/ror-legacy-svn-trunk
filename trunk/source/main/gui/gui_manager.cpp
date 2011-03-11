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
// based on the basemanager code from mygui common
#ifdef USE_MYGUI

#include "gui_manager.h"
#include "Settings.h"
#include "language.h"
#include <MyGUI_OgrePlatform.h>
#include <MyGUI_LanguageManager.h>

#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#	include <windows.h>
#endif

using namespace Ogre;

template<> GUIManager * Singleton< GUIManager >::ms_Singleton = 0;

GUIManager* GUIManager::getSingletonPtr(void)
{
	return ms_Singleton;
}
GUIManager& GUIManager::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

GUIManager::GUIManager(Ogre::Root *root, Ogre::SceneManager *mgr, Ogre::RenderWindow *win) :
	mGUI(nullptr),
	mPlatform(nullptr),
	mRoot(root),
	mSceneManager(mgr),
	mWindow(win),
	mExit(false),
	mResourceFileName("MyGUI_Core.xml")
{
	create();
}

GUIManager::~GUIManager()
{
}

bool GUIManager::create()
{
	mRoot->addFrameListener(this);
	Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

	windowResized(mWindow);
	createGui();
#ifdef WIN32
	MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &GUIManager::eventRequestTag);
#endif // WIN32
	return true;
}

void GUIManager::destroy()
{
	destroyGui();
}

void GUIManager::createGui()
{
	String gui_logfilename = SSETTING("Log Path") + "mygui.log";
	mPlatform = new MyGUI::OgrePlatform();
	mPlatform->initialise(mWindow, mSceneManager, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, gui_logfilename); // use cache resource group so preview images are working
	mGUI = new MyGUI::Gui();
	mGUI->initialise(mResourceFileName);
	//MyGUI::PluginManager::getInstance().loadPlugin("Plugin_BerkeliumWidget.dll");
	MyGUI::PointerManager::getInstance().setVisible(true);
}

void GUIManager::destroyGui()
{
	if (mGUI)
	{
		mGUI->shutdown();
		delete mGUI;
		mGUI = nullptr;
	}

	if (mPlatform)
	{
		mPlatform->shutdown();
		delete mPlatform;
		mPlatform = nullptr;
	}
}

bool GUIManager::frameStarted(const Ogre::FrameEvent& evt)
{
	if (mExit) return false;
	if (!mGUI) return true;
	return true;
}

bool GUIManager::frameEnded(const Ogre::FrameEvent& evt)
{
	return true;
};

void GUIManager::windowResized(Ogre::RenderWindow* _rw)
{
	int width = (int)_rw->getWidth();
	int height = (int)_rw->getHeight();
	setInputViewSize(width, height);
}

void GUIManager::windowClosed(Ogre::RenderWindow* _rw)
{
	mExit = true;
}

void GUIManager::eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result)
{
	_result = MyGUI::LanguageManager::getInstance().getTag(_tag);
}

#endif //MYGUI
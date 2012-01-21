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
// based on the basemanager code from mygui common
#ifdef USE_MYGUI

#include "gui_manager.h"
#include "Settings.h"
#include "language.h"
#include "RoRWindowEventUtilities.h"
#include <MyGUI_OgrePlatform.h>
#include <MyGUI_LanguageManager.h>

#include "Console.h"

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
	RoRWindowEventUtilities::addWindowEventListener(mWindow, this);

	windowResized(mWindow);
	createGui();
#ifdef WIN32
	MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &GUIManager::eventRequestTag);
#endif // WIN32
	return true;
}

void GUIManager::unfocus()
{
	MyGUI::InputManager::getInstance().resetKeyFocusWidget(); 
	MyGUI::InputManager::getInstance().resetMouseCaptureWidget(); 
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
	
	MyGUI::ResourceManager::getInstance().load(LanguageEngine::Instance().getMyGUIFontConfigFilename());

	// move the mouse into the middle of the screen, assuming we start at the top left corner (0,0)
	MyGUI::InputManager::getInstance().injectMouseMove(mWindow->getWidth()*0.5f, mWindow->getHeight()*0.5f, 0);

	// now find that font texture and save it - for debugging purposes
	/*
	Ogre::ResourceManager::ResourceMapIterator it = Ogre::TextureManager::getSingleton().getResourceIterator();
	while (it.hasMoreElements())
	{
		Ogre::ResourcePtr res = it.getNext();
		if(res->getName().find("TrueTypeFont") != String::npos)
		{
			Ogre::Image image;
			Ogre::TexturePtr tex = (Ogre::TexturePtr)res;
			tex->convertToImage(image);
			image.save(res->getName() + ".png");
			LOG(">> saved TTF texture: " + res->getName());
		}
	}
	*/

	//MyGUI::PluginManager::getInstance().loadPlugin("Plugin_BerkeliumWidget.dll");
	MyGUI::PointerManager::getInstance().setVisible(true);
	Console *c = Console::getInstancePtrNoCreation();
	if(c) c->resized();
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


	// now hide the mouse cursor if not used since a long time
	if(getLastMouseMoveTime() > 5000)
	{
		MyGUI::PointerManager::getInstance().setVisible(false);
		//GUI_MainMenu::getSingleton().setVisible(false);
	}

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
	
	Console *c = Console::getInstancePtrNoCreation();
	if(c) c->resized();
}

void GUIManager::windowClosed(Ogre::RenderWindow* _rw)
{
	mExit = true;
}

void GUIManager::eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result)
{
	_result = MyGUI::LanguageManager::getInstance().getTag(_tag);
}

Ogre::String GUIManager::getRandomWallpaperImage()
{
	
	FileInfoListPtr files = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo("Wallpapers", "*.jpg", false);
	if(files.isNull() || files->empty())
	{
		return "";
	}
	srand ( time(NULL) );

	int num = 0;
	for(int i = 0; i<Ogre::Math::RangeRandom(0, 10); i++)
		num = Ogre::Math::RangeRandom(0, files->size());

	return files->at(num).filename;
}

#endif //MYGUI
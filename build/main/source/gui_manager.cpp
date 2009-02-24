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
#include "InputEngine.h"
#include "gui_manager.h"

#include "MyGUI.h"
#include "Ogre.h"
#include "Settings.h"

// thomas fischer, thomas@thomasfischer.biz, 3rd June 2008

using namespace Ogre;

// singleton pattern
GUIManager* GUIManager::myInstance = 0;

GUIManager &GUIManager::Instance () 
{
	if (myInstance == 0)
		myInstance = new GUIManager;
	return *myInstance;
}

GUIManager::GUIManager()
{
	initialized=false;
	enabled=false;
	mGUI = 0;
}

GUIManager::~GUIManager()
{
	if (mGUI)
	{
		mGUI->shutdown();
		delete mGUI;
		mGUI = 0;
	}
}

void GUIManager::setup(Ogre::Camera *cam, Ogre::SceneManager *scm, Ogre::RenderWindow *mWindow)
{
	LogManager::getSingleton().logMessage("loading GUI...");
	mGUI = new MyGUI::Gui();
	String gui_logfilename = SETTINGS.getSetting("Log Path") + "mygui.log";
	String gui_core = "core.xml";
	if(SETTINGS.getSetting("GUI Core") != "")
		gui_core = SETTINGS.getSetting("GUI Core");
	mGUI->initialise(mWindow, gui_core, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, gui_logfilename);
	initialized=true;
	enabled=true;

	// load all layouts
	loadLayout("loader.layout");
	MyGUI::WindowPtr window = mGUI->findWidget<MyGUI::Window>("loaderWindow");
	window->setVisible(false);

	loadLayout("progress.layout");
	window = mGUI->findWidget<MyGUI::Window>("loaderWindow");
	window->setVisible(false);

	// hide mouse
	MyGUI::PointerManager::getInstance().setVisible(false);

	//MyGUI::StaticImagePtr image = mGUI->createWidget<MyGUI::StaticImage>("StaticImage", MyGUI::IntCoord(MyGUI::IntPoint(), mGUI->getViewSize()), MyGUI::Align::Stretch, "Back");
	//image->setImageTexture("grid1.png");

	// update the screen size, important
	windowResized(mWindow);
	LogManager::getSingleton().logMessage("GUI loaded!");
}

void GUIManager::loadLayout(Ogre::String name)
{
	MyGUI::LayoutManager::getInstance().load(name);
}

void GUIManager::windowResized(Ogre::RenderWindow *rw)
{
	LogManager::getSingleton().logMessage("GUI windowResized()");
	if(mGUI)
		mGUI->windowResized(rw);
}

void GUIManager::windowMoved(Ogre::RenderWindow *rw)
{
	LogManager::getSingleton().logMessage("GUI windowMoved()");
	if(mGUI)
		mGUI->windowMoved(rw);
}


bool GUIManager::frameStarted(const FrameEvent& evt)
{
	if(mGUI && enabled)
		mGUI->injectFrameEntered(evt.timeSinceLastFrame);
	return true;
}

bool GUIManager::frameEnded(const FrameEvent& evt)
{
	return true;
}

void GUIManager::setCamera(Ogre::Camera *mCamera)
{
	mGUI->setSceneManager(mCamera->getSceneManager());
}



void GUIManager::shutdown()
{
	LogManager::getSingleton().logMessage("GUI shutdown");
}

void GUIManager::activate()
{
	LogManager::getSingleton().logMessage("GUI activated");
	enabled = true;
}

void GUIManager::desactivate()
{
	LogManager::getSingleton().logMessage("GUI desactivated");
	enabled = false;
}

void GUIManager::setCursorPosition(float x, float y)
{
}

bool GUIManager::mouseMoved( const OIS::MouseEvent &arg )
{
	if (enabled && mGUI)
		mGUI->injectMouseMove(arg);
	return true;
}

bool GUIManager::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	if (enabled && mGUI)
		mGUI->injectMousePress(arg, id);
	return true;
}

bool GUIManager::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	if (enabled && mGUI)
		mGUI->injectMouseRelease(arg, id);
	return true;
}

bool GUIManager::keyPressed( const OIS::KeyEvent &arg )
{
	if (enabled && mGUI)
		mGUI->injectKeyPress(arg);
	return true;
}

bool GUIManager::keyReleased( const OIS::KeyEvent &arg )
{
	if (enabled && mGUI)
		mGUI->injectKeyRelease(arg);
	return true;
}

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

#include "LoadingWindow.h"
#include "gui_manager.h"
#include "RoRWindowEventUtilities.h"

#include "Settings.h"

#include "language.h"

LoadingWindow::LoadingWindow() :
	mFrameForced(false)
{
	initialiseByAttributes(this);

	MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
	mMainWidget->setPosition(gui_area.width/2 - mMainWidget->getWidth()/2, gui_area.height/2 - mMainWidget->getHeight()/2);
	((MyGUI::Window*)mMainWidget)->setCaption(_L("Loading ..."));
	t = new Ogre::Timer();
	mMainWidget->setVisible(false);
}

LoadingWindow::~LoadingWindow()
{
	delete(t);
	t=NULL;
}

bool LoadingWindow::getFrameForced()
{
	if (!mFrameForced) return false;
	mFrameForced = false;
	return true;
}

void LoadingWindow::setProgress(int _percent, const Ogre::String& _text, bool _updateRenderFrame)
{
	if(BSETTING("REPO_MODE")) return;
	mMainWidget->setVisible(true);
	mInfoStaticText->setCaption(_text);

	mBarProgress->setProgressAutoTrack(false);
	mBarProgress->setProgressPosition(_percent);

	if( _updateRenderFrame ) renderOneFrame();
}

void LoadingWindow::setAutotrack(const Ogre::String& _text, bool _updateRenderFrame)
{
	if(BSETTING("REPO_MODE")) return;
	mMainWidget->setVisible(true);
	mInfoStaticText->setCaption(_text);
	mBarProgress->setProgressPosition(0);
	mBarProgress->setProgressAutoTrack(true);

	if( _updateRenderFrame ) renderOneFrame();
}

void LoadingWindow::hide()
{
	if(BSETTING("REPO_MODE")) return;
	GUIManager::getSingleton().unfocus();
	mMainWidget->setVisible(false);
}

void LoadingWindow::renderOneFrame(bool force)
{
	if(BSETTING("REPO_MODE")) return;
	if(t->getMilliseconds() > 200 || force)
	{
		mFrameForced=true;
		// we must pump the window messages, otherwise the window will get white on Vista ...
		RoRWindowEventUtilities::messagePump();
		Ogre::Root::getSingleton().renderOneFrame();
		t->reset();
	}
}
#endif //MYGUI


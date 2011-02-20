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
#include "RigsOfRods.h"

#include <Ogre.h>

#include "GameState.h"
#include "Settings.h"
#include "ContentManager.h"

using namespace Ogre;

RigsOfRods::RigsOfRods(Ogre::String name, unsigned int hwnd, unsigned int mainhwnd) : 
	stateManager(0),
	hwnd(hwnd),
	mainhwnd(mainhwnd),
	name(name)
{
}

RigsOfRods::~RigsOfRods()
{
	delete stateManager;
    delete OgreFramework::getSingletonPtr();

}

void RigsOfRods::go(void)
{
	// init ogre
	new OgreFramework();
	if(!OgreFramework::getSingletonPtr()->initOgre(name, hwnd, mainhwnd))
		return;

	// then the base content setup
	new ContentManager();
	ContentManager::getSingleton().init();

	// now add the game states
	stateManager = new AppStateManager();

	GameState::create(stateManager,  "GameState");

	// select the first one
	stateManager->start(stateManager->findByName("GameState"));

	LogManager::getSingleton().logMessage("Rigs of Rods initialized!");
}

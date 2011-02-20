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
/*! @mainpage

	have fun coding :)

	please note that the documentation is work in progress
*/
#ifndef RIGSOFRODS_H__
#define RIGSOFRODS_H__

#include "RoRPrerequisites.h"

#include "AdvancedOgreFramework.h"
#include "AppStateManager.h"

class RigsOfRods
{
public:
	RigsOfRods(Ogre::String name = Ogre::String("RoR"), unsigned int hwnd=0, unsigned int mainhwnd=0);
	~RigsOfRods();

	void go();
	void shutdown(void);

protected:
	AppStateManager *stateManager;
	unsigned int hwnd, mainhwnd;
	Ogre::String name;
};

#endif //RIGSOFRODS_H__

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

#include "LobbyGUI.h"
#include "ScriptEngine.h"
#include "InputEngine.h"
#include "OgreLogManager.h"

#include "Settings.h"
#include "RoRFrameListener.h"
#include "network.h"

#include "libircclient.h"

// class

LobbyGUI::LobbyGUI()
{
	initialiseByAttributes(this);
}

LobbyGUI::~LobbyGUI()
{
}

#endif //USE_MYGUI 

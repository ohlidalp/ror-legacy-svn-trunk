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

#ifndef LOBBYGUI_H__
#define LOBBYGUI_H__

#include "RoRPrerequisites.h"
#include "Singleton.h"
#include "mygui/BaseLayout.h"

#include <OgreLog.h>
#include <OgreUTFString.h>

ATTRIBUTE_CLASS_LAYOUT(LobbyGUI, "Lobby.layout");
class LobbyGUI :
	public wraps::BaseLayout,
	public Singleton2<LobbyGUI>,
	public Ogre::LogListener
{
	friend class Singleton2<LobbyGUI>;
	LobbyGUI();
	~LobbyGUI();
public:

	void setVisible(bool _visible);
	bool getVisible();

	void select();

	void setNetwork(Network *net);

	// print waiting messages
	void frameEntered(float _frame);
protected:
};

#endif // LOBBYGUI_H__

#endif //MYGUI


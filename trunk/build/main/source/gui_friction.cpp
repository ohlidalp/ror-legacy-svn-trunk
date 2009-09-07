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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of September 2009

#include "gui_friction.h"
#include "gui_manager.h"
#include "Ogre.h"
#include "Settings.h"

#include "language.h"

using namespace Ogre;

template<> GUI_Friction * Singleton< GUI_Friction >::ms_Singleton = 0;
GUI_Friction* GUI_Friction::getSingletonPtr(void)
{
	return ms_Singleton;
}
GUI_Friction& GUI_Friction::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

GUI_Friction::GUI_Friction()
{
	int x=0, y=0;
	win =  MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCSX", 0, 0, 400, 500,  MyGUI::Align::Center, "Back");
	
	// Fluid parameters
	x=10; y= 10;
	MyGUI::StaticTextPtr t = win->createWidget<MyGUI::StaticText>("StaticText", x, y, 100, 20,  MyGUI::Align::Default, "Back");
	t->setCaption(_L("Fluid Settings"));

	win->setVisible(false);
}

GUI_Friction::~GUI_Friction()
{
}

void GUI_Friction::setVisible(bool value)
{
	win->setVisible(value);
	MyGUI::PointerManager::getInstance().setVisible(value);
}

bool GUI_Friction::getVisible()
{
	return win->isVisible();
}

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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 13th of August 2009

#include "gui_menu.h"
#include "gui_loader.h"
#include "gui_friction.h"
#include "gui_manager.h"
#include "ExampleFrameListener.h"
#include "Ogre.h"
#include "Settings.h"

#include "language.h"

using namespace Ogre;

template<> GUI_MainMenu * Singleton< GUI_MainMenu >::ms_Singleton = 0;
GUI_MainMenu* GUI_MainMenu::getSingletonPtr(void)
{
	return ms_Singleton;
}
GUI_MainMenu& GUI_MainMenu::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

GUI_MainMenu::GUI_MainMenu(ExampleFrameListener *efl) : mefl(efl)
{
	//MyGUI::WidgetPtr back = createWidget<MyGUI::Widget>("Panel", 0, 0, 912, 652,MyGUI::Align::Default, "Back");
	mainmenu = MyGUI::Gui::getInstance().createWidget<MyGUI::MenuBar>("MenuBar", 0, 0, 300, 26,  MyGUI::Align::HStretch | MyGUI::Align::Top, "Back"); 
	mainmenu->setRealCoord(0,0,100,0.001);
	
	// File menu
	MyGUI::MenuItemPtr mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, 22,  MyGUI::Align::Default); 
	MyGUI::PopupMenuPtr pop = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("File");

	pop->addItem("Hide Menu", MyGUI::MenuItemType::Normal);
	pop->addItem("entry3", MyGUI::MenuItemType::Normal);
	pop->addItem("-", MyGUI::MenuItemType::Separator);
	pop->addItem("Exit Game", MyGUI::MenuItemType::Normal);

	// windows
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, 22,  MyGUI::Align::Default); 
	pop = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("Windows");
	pop->addItem("Camera Control", MyGUI::MenuItemType::Normal, "cameratool");
	pop->addItem("Friction Settings", MyGUI::MenuItemType::Normal, "frictiongui");
	
	mainmenu->eventMenuCtrlAccept = MyGUI::newDelegate(this, &GUI_MainMenu::onMenuBtn);



	//MyGUI::PointerManager::getInstance().setVisible(true);
	//UILOADER.setProgress(UI_PROGRESSBAR_HIDE);
	setVisible(false);
}

GUI_MainMenu::~GUI_MainMenu()
{
}

void GUI_MainMenu::onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item)
{
	String miname = _item->getCaption();
	if(miname == "Hide Menu")
	{
		setVisible(false);
	} else if(miname == "Friction Settings")
	{
		GUI_Friction::getSingleton().setVisible(true);
	} else if(miname == "Exit Game")
	{
		mefl->shutdown_pre();
	}

	//LogManager::getSingleton().logMessage(" menu button pressed: " + _item->getCaption());
}

void GUI_MainMenu::setVisible(bool value)
{
	mainmenu->setVisible(value);
	MyGUI::PointerManager::getInstance().setVisible(value);
}

bool GUI_MainMenu::getVisible()
{
	return mainmenu->isVisible();
}

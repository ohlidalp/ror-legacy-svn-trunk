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
#include "gui_manager.h"
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

GUI_MainMenu::GUI_MainMenu()
{
	//MyGUI::WidgetPtr back = createWidget<MyGUI::Widget>("Panel", 0, 0, 912, 652,MyGUI::Align::Default, "Back");
	mainmenu = MyGUI::Gui::getInstance().createWidget<MyGUI::MenuBar>("MenuBar", 0, 0, 300, 26,  MyGUI::Align::HStretch | MyGUI::Align::Top, "Back"); 
	mainmenu->setRealCoord(0,0,100,0.001);
	
	// File menu
	MyGUI::MenuItemPtr mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, 22,  MyGUI::Align::Default); 
	MyGUI::PopupMenuPtr pop = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("File");


	pop->addItem("entry1", MyGUI::MenuItemType::Normal, "entry1");
	pop->addItem("entry2", MyGUI::MenuItemType::Normal, "entry2");
	pop->addItem("entry3", MyGUI::MenuItemType::Normal, "entry3");
	pop->addItem("-", MyGUI::MenuItemType::Separator);
	pop->addItem("exit", MyGUI::MenuItemType::Normal, "exit");


	// view menu
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, 22,  MyGUI::Align::Default); 
	pop = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("View");


	MyGUI::MenuItemPtr mi2 = pop->addItem("Camera Mode", MyGUI::MenuItemType::Normal, "cm");
	MyGUI::PopupMenuPtr pop2 = mi2->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");

	pop2->createWidget<MyGUI::Button>("CheckBox",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Internal Camera");
	pop2->createWidget<MyGUI::Button>("CheckBox",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "External Camera");




	pop->eventMenuCtrlAccept = MyGUI::newDelegate(this, &GUI_MainMenu::onMenuBtn);
	pop2->eventMenuCtrlAccept = MyGUI::newDelegate(this, &GUI_MainMenu::onMenuBtn);
	//window->eventWindowButtonPressed = MyGUI::newDelegate(this, &DemoKeeper::notifyWindowPressed);	
	MyGUI::PointerManager::getInstance().setVisible(true);

	UILOADER.setProgress(UI_PROGRESSBAR_HIDE);
	setVisible(false);
}

GUI_MainMenu::~GUI_MainMenu()
{
}

void GUI_MainMenu::onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item)
{
	LogManager::getSingleton().logMessage(" menu button pressed: " + _item->getCaption());
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

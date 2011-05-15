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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 13th of August 2009
#ifdef USE_MYGUI 
#include "gui_menu.h"
#include "gui_friction.h"
#include "gui_manager.h"
#include "SelectorWindow.h"
#include "RoRFrameListener.h"
#include "Ogre.h"
#include "Settings.h"
#include "BeamFactory.h"

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

GUI_MainMenu::GUI_MainMenu(RoRFrameListener *efl) : mefl(efl)
{
	//MyGUI::WidgetPtr back = createWidget<MyGUI::Widget>("Panel", 0, 0, 912, 652,MyGUI::Align::Default, "Back");
	mainmenu = MyGUI::Gui::getInstance().createWidget<MyGUI::MenuBar>("MenuBar", 0, 0, 300, 26,  MyGUI::Align::HStretch | MyGUI::Align::Top, "Back"); 
	mainmenu->setCoord(0, 0, 800, 20);
	int popCount = 0;
	
	// Simulation menu
	MyGUI::MenuItemPtr mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, 22,  MyGUI::Align::Default); 
	pop[popCount] = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("Simulation");
	pop[popCount]->addItem("get new Vehicle", MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem("remove current Vehicle", MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem("activate all Vehicles", MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem("send all Vehicles to sleep", MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem("switch Truck", MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem("-", MyGUI::MenuItemType::Separator);
	pop[popCount]->addItem("Exit", MyGUI::MenuItemType::Normal);

	// view
	popCount++;
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, 22,  MyGUI::Align::Default); 
	pop[popCount] = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("View");
	MyGUI::MenuItemPtr mi2 = pop[popCount]->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, 22,  MyGUI::Align::Default); 
	mi2->setItemType(MyGUI::MenuItemType::Popup);
	mi2->setCaption("Camera Mode");

	popCount++;
	pop[popCount] = mi2->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	pop[popCount]->addItem("First Person", MyGUI::MenuItemType::Normal, "camera_first_person");
	pop[popCount]->addItem("External", MyGUI::MenuItemType::Normal, "camera_external");
	pop[popCount]->addItem("Free Mode", MyGUI::MenuItemType::Normal, "camera_free");
	pop[popCount]->addItem("Free fixed mode", MyGUI::MenuItemType::Normal, "camera_freefixed");
	pop[popCount]->addItem("-", MyGUI::MenuItemType::Separator);

	mi2 = pop[popCount]->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, 22,  MyGUI::Align::Default); 
	mi2->setItemType(MyGUI::MenuItemType::Popup);
	mi2->setCaption("Truck Camera");

	popCount++;
	pop[popCount] = mi2->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	pop[popCount]->addItem("1. Camera", MyGUI::MenuItemType::Normal, "camera_truck_1");
	pop[popCount]->addItem("2. Camera", MyGUI::MenuItemType::Normal, "camera_truck_2");
	pop[popCount]->addItem("3. Camera", MyGUI::MenuItemType::Normal, "camera_truck_3");

	// windows
	popCount++;
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, 22,  MyGUI::Align::Default); 
	pop[popCount] = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("Windows");
	pop[popCount]->addItem("Camera Control", MyGUI::MenuItemType::Normal, "cameratool");
	pop[popCount]->addItem("Friction Settings", MyGUI::MenuItemType::Normal, "frictiongui");
	pop[popCount]->addItem("Show Console", MyGUI::MenuItemType::Normal, "showConsole");
	

	// if you add a menu, fix NUM_POPUPMENUS

	// event callbacks
	mainmenu->eventMenuCtrlAccept += MyGUI::newDelegate(this, &GUI_MainMenu::onMenuBtn);
}

GUI_MainMenu::~GUI_MainMenu()
{
}

void GUI_MainMenu::onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item)
{
	String miname = _item->getCaption();

	if(miname == "get new Vehicle" && mefl->person)
	{
		// get out first
		if(BeamFactory::getSingleton().getCurrentTruckNumber() != -1)
			BeamFactory::getSingleton().setCurrentTruck(-1);
		mefl->reload_pos = mefl->person->getPosition() + Vector3(0, 1, 0); // 1 meter above the character
		mefl->freeTruckPosition=true;
		mefl->loading_state=RELOADING;
		SelectorWindow::get()->show(SelectorWindow::LT_AllBeam);

	} else if(miname == "remove current Vehicle")
	{
		BeamFactory::getSingleton().removeCurrentTruck();

	} else if(miname == "activate all Vehicles")
	{
		BeamFactory::getSingleton().activateAllTrucks();
		
	} else if(miname == "send all Vehicles to sleep")
	{
		// get out first
		if(BeamFactory::getSingleton().getCurrentTruckNumber() != -1)
			BeamFactory::getSingleton().setCurrentTruck(-1);
		BeamFactory::getSingleton().sendAllTrucksSleeping();

	} else if(miname == "reload Truck from File")
	{
		if(BeamFactory::getSingleton().getCurrentTruckNumber() != -1)
			mefl->reloadCurrentTruck();

	} else if(miname == "switch Truck")
	{
		// TODO

	} else if(miname == "Friction Settings")
	{
		GUI_Friction::getSingleton().setVisible(true);

	} else if(miname == "Exit")
	{
		mefl->shutdown_final();
	} else if(miname == "Show Console")
	{
		Console::getInstance().setVisible(true);
	}

	//LOG(" menu button pressed: " + _item->getCaption());
}

void GUI_MainMenu::setVisible(bool value)
{
	mainmenu->setVisible(value);
	//MyGUI::PointerManager::getInstance().setVisible(value);
}

bool GUI_MainMenu::getVisible()
{
	return mainmenu->getVisible();
}

void GUI_MainMenu::updatePositionUponMousePosition(int x, int y)
{
	int h = mainmenu->getHeight();
	bool focused = false;
	for(int i=0;i<NUM_POPUPMENUS; i++)
		focused |= pop[i]->getVisible();

	if(focused)
	{
		mainmenu->setPosition(0, 0);
	} else
	{
		if(y > 2*h)
			mainmenu->setPosition(0, -h);

		else
			mainmenu->setPosition(0, std::min(0, -y+10));
	}
}

#endif // MYGUI


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
#include "Savegame.h"

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

GUI_MainMenu::GUI_MainMenu(RoRFrameListener *efl) : mefl(efl), menuWidth(800), menuHeight(20), vehicleListNeedsUpdate(false)
{
	pthread_mutex_init(&updateLock, NULL);

	//MyGUI::WidgetPtr back = createWidget<MyGUI::Widget>("Panel", 0, 0, 912, 652,MyGUI::Align::Default, "Back");
	mainmenu = MyGUI::Gui::getInstance().createWidget<MyGUI::MenuBar>("MenuBar", 0, 0, menuWidth, menuHeight,  MyGUI::Align::HStretch | MyGUI::Align::Top, "Back"); 
	mainmenu->setCoord(0, 0, menuWidth, menuHeight);
	int popCount = 0;
	
	// Simulation menu
	MyGUI::MenuItemPtr mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default); 
	pop[popCount] = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption(_L("Simulation"));
	pop[popCount]->setPopupAccept(true);
	//mi->setPopupAccept(true);

	MyGUI::IntSize s = mi->getTextSize();
	menuHeight = s.height + 6;
	mainmenu->setCoord(0, 0, menuWidth, menuHeight);

	
	pop[popCount]->addItem(_L("get new Vehicle"), MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem(_L("remove current Vehicle"), MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem(_L("activate all Vehicles"), MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem(_L("send all Vehicles to sleep"), MyGUI::MenuItemType::Normal);
	//pop[popCount]->addItem(_L("switch Truck"), MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem("-", MyGUI::MenuItemType::Separator);
	pop[popCount]->addItem(_L("Save Scenery"), MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem(_L("Load Scenery"), MyGUI::MenuItemType::Normal);
	pop[popCount]->addItem("-", MyGUI::MenuItemType::Separator);
	pop[popCount]->addItem(_L("Exit"), MyGUI::MenuItemType::Normal);

	// vehicles
	popCount++;
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default);
	vehiclesMenu = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu", MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	pop[popCount] = vehiclesMenu;
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("Vehicles");
	vehiclesMenu->addItem(_L("TEST"), MyGUI::MenuItemType::Normal, "test");

	// this is not working :(
	//vehiclesMenu->setPopupAccept(true);
	//vehiclesMenu->eventMenuCtrlAccept += MyGUI::newDelegate(this, &GUI_MainMenu::onVehicleMenu);


	// view
	popCount++;
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default); 
	pop[popCount] = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption(_L("View"));
	MyGUI::MenuItemPtr mi2 = pop[popCount]->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default); 
	mi2->setItemType(MyGUI::MenuItemType::Popup);
	mi2->setCaption(_L("Camera Mode"));

	popCount++;
	pop[popCount] = mi2->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	pop[popCount]->addItem(_L("First Person"), MyGUI::MenuItemType::Normal, "camera_first_person");
	pop[popCount]->addItem(_L("External"), MyGUI::MenuItemType::Normal, "camera_external");
	pop[popCount]->addItem(_L("Free Mode"), MyGUI::MenuItemType::Normal, "camera_free");
	pop[popCount]->addItem(_L("Free fixed mode"), MyGUI::MenuItemType::Normal, "camera_freefixed");
	pop[popCount]->addItem("-", MyGUI::MenuItemType::Separator);

	mi2 = pop[popCount]->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default); 
	mi2->setItemType(MyGUI::MenuItemType::Popup);
	mi2->setCaption(_L("Truck Camera"));

	popCount++;
	pop[popCount] = mi2->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	pop[popCount]->addItem(_L("1. Camera"), MyGUI::MenuItemType::Normal, "camera_truck_1");
	pop[popCount]->addItem(_L("2. Camera"), MyGUI::MenuItemType::Normal, "camera_truck_2");
	pop[popCount]->addItem(_L("3. Camera"), MyGUI::MenuItemType::Normal, "camera_truck_3");

	// windows
	popCount++;
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default); 
	pop[popCount] = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("Windows");
	pop[popCount]->addItem(_L("Camera Control"), MyGUI::MenuItemType::Normal, "cameratool");
	pop[popCount]->addItem(_L("Friction Settings"), MyGUI::MenuItemType::Normal, "frictiongui");
	pop[popCount]->addItem(_L("Show Console"), MyGUI::MenuItemType::Normal, "showConsole");
	

	// if you add a menu, fix NUM_POPUPMENUS

	// event callbacks
	mainmenu->eventMenuCtrlAccept += MyGUI::newDelegate(this, &GUI_MainMenu::onMenuBtn);

	// initial mouse position somewhere so the menu is hidden
	updatePositionUponMousePosition(500, 500);
}

GUI_MainMenu::~GUI_MainMenu()
{
}

void GUI_MainMenu::vehiclesListUpdate()
{
	vehiclesMenu->removeAllItems();
	
	int numTrucks = BeamFactory::getSingleton().getTruckCount();
	Beam **trucks = BeamFactory::getSingleton().getTrucks();
	for(int i = 0; i < numTrucks; i++)
	{
		if(!trucks[i]) continue;
		
		char tmp[255] = "";
		sprintf(tmp, "[%d] %s", i, trucks[i]->realtruckname.c_str());

		vehiclesMenu->addItem(String(tmp), MyGUI::MenuItemType::Normal, "TRUCK_"+TOSTRING(i));
	}
}

void GUI_MainMenu::onVehicleMenu(MyGUI::MenuControl* _sender, MyGUI::MenuItem* _item)
{
	// not working :(
	//vehiclesListUpdate();
}

void GUI_MainMenu::onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item)
{
	String miname = _item->getCaption();
	String id     = _item->getItemId();

	if(id.substr(0,6) == "TRUCK_")
	{
		int truck = PARSEINT(id.substr(6));
		if(truck >= 0 && truck < BeamFactory::getSingleton().getTruckCount())
			BeamFactory::getSingleton().setCurrentTruck(truck);
	}

	if(miname == _L("get new Vehicle") && mefl->person)
	{
		// get out first
		if(BeamFactory::getSingleton().getCurrentTruckNumber() != -1)
			BeamFactory::getSingleton().setCurrentTruck(-1);
		mefl->reload_pos = mefl->person->getPosition() + Vector3(0, 1, 0); // 1 meter above the character
		mefl->freeTruckPosition=true;
		mefl->loading_state=RELOADING;
		SelectorWindow::get()->show(SelectorWindow::LT_AllBeam);

	} else if(miname == _L("Save Scenery") || miname == _L("Load Scenery"))
	{
		if(mefl->loading_state != ALL_LOADED)
		{
			LOG("you need to open a map before trying to save or load its scenery.");
			return;
		}
		String fname = SSETTING("Cache Path") + mefl->loadedTerrain + ".rorscene";

		Savegame s;
		if(miname == _L("Save Scenery"))
		{
			s.save(fname);
		} else
		{
			s.load(fname);
		}

	} else if(miname == _L("remove current Vehicle"))
	{
		BeamFactory::getSingleton().removeCurrentTruck();

	} else if(miname == _L("activate all Vehicles"))
	{
		BeamFactory::getSingleton().activateAllTrucks();
		
	} else if(miname == _L("send all Vehicles to sleep"))
	{
		// get out first
		if(BeamFactory::getSingleton().getCurrentTruckNumber() != -1)
			BeamFactory::getSingleton().setCurrentTruck(-1);
		BeamFactory::getSingleton().sendAllTrucksSleeping();

	} else if(miname == _L("reload Truck from File"))
	{
		if(BeamFactory::getSingleton().getCurrentTruckNumber() != -1)
		{
			mefl->reloadCurrentTruck();
			GUIManager::getSingleton().unfocus();
		}

	} else if(miname == _L("switch Truck"))
	{
		// TODO

	} else if(miname == _L("Friction Settings"))
	{
		GUI_Friction::getSingleton().setVisible(true);

	} else if(miname == _L("Exit"))
	{
		mefl->shutdown_final();
	} else if(miname == _L("Show Console"))
	{
		Console::getInstance().setVisible(true);
	}

	//LOG(" menu button pressed: " + _item->getCaption());
}

void GUI_MainMenu::setVisible(bool value)
{
	mainmenu->setVisible(value);
	if(!value) GUIManager::getSingleton().unfocus();
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

	// this is hacky, but needed as the click callback is not working
	if(vehicleListNeedsUpdate)
	{
		vehiclesListUpdate();
		MUTEX_LOCK(&updateLock)
		vehicleListNeedsUpdate = false;
		MUTEX_UNLOCK(&updateLock)
	}
}

void GUI_MainMenu::triggerUpdateVehicleList()
{
	MUTEX_LOCK(&updateLock)
	vehicleListNeedsUpdate = true;
	MUTEX_UNLOCK(&updateLock)
};

#endif // MYGUI


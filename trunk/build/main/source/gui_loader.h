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

#ifndef GUI_LOADER_H
#define GUI_LOADER_H

#include <MyGUI.h>
#include <Ogre.h>
#include "skin.h"

#define UILOADER GUI_Loader::Instance()
#define UI_PROGRESSBAR_AUTOTRACK -1
#define UI_PROGRESSBAR_HIDE -2

class Cache_Entry;

class GUI_Loader
{
protected:
	GUI_Loader();
	~GUI_Loader();
	GUI_Loader(const GUI_Loader&);
	GUI_Loader& operator= (const GUI_Loader&);

	// members
	static GUI_Loader* myInstance;
	std::vector<Cache_Entry> myEntries;
	std::map<int, int> categoryUsage;
	std::vector<SkinPtr> current_skins;
	int loaderType;
	Cache_Entry *selectedtruck;
	bool selectiondone;
	std::vector<Ogre::String> truck_configs;
	Ogre::RenderWindow *rw;
	bool frameForced;
	SkinPtr selectedskin;

	// custom functions
	void getData();
	void onCategorySelected(int categoryID);
	void onEntrySelected(int entryID);
	void selectionDone();

	// grouped by control and events
	MyGUI::ButtonPtr btnOK;
	void event_btnOk_MouseButtonClick(MyGUI::WidgetPtr _sender);
	
	MyGUI::ButtonPtr btnCancel;
	void event_btnCancel_MouseButtonClick(MyGUI::WidgetPtr _sender);

	MyGUI::WindowPtr window, windowp;

	MyGUI::ListPtr list;
	void event_list_eventListChangePosition(MyGUI::WidgetPtr _sender, size_t _index);

	MyGUI::ComboBoxPtr combobox;
	void event_combobox_eventComboAccept(MyGUI::WidgetPtr _sender, size_t _index);
	void event_combobox_eventComboChangePosition(MyGUI::WidgetPtr _sender, size_t _index);
	void event_window_eventKeyButtonPressed(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
	void updateControls(Cache_Entry *entry);
	void setPreviewImage(Ogre::String texture);

	MyGUI::StaticImagePtr image_preview;
	MyGUI::StaticTextPtr text_entry_name, text_entry, text_entry_descr;

	MyGUI::SheetPtr sheet_cache, sheet_search;
	MyGUI::ButtonPtr btnSearch;


	MyGUI::ComboBoxPtr combo_configs;
	void event_combo_configs_eventComboAccept(MyGUI::WidgetPtr _sender, size_t _index);
	
	// progress window things
	MyGUI::ProgressPtr prog;
	MyGUI::StaticTextPtr prog_text;

public:
	static GUI_Loader &Instance();
	enum loaderType {LT_Terrain, LT_Vehicle, LT_Truck, LT_Car, LT_Boat, LT_Airplane, LT_Trailer, LT_Load, LT_Extension, LT_Network, LT_NetworkWithBoat, LT_Heli, LT_SKIN};

	bool isFinishedSelecting();
	void show(int type);
	void hide();
	void setup(Ogre::RenderWindow *rw);

	void setProgress(int percent, Ogre::String text="", bool updateRenderFrame=true);
	bool getFrameForced();

	Cache_Entry *getSelection();
	SkinPtr getSelectedSkin();
	std::vector<Ogre::String> getTruckConfig();
	void setEnableCancel(bool enabled);
};

#endif

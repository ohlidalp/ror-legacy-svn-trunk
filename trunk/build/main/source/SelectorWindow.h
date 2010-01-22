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
#ifndef __SELECTOR_WINDOW_H__
#define __SELECTOR_WINDOW_H__

#include "Singleton.h"
#include <BaseLayout.h>
#include <Ogre.h>
#include "skin.h"

class Cache_Entry;

ATTRIBUTE_CLASS_LAYOUT(SelectorWindow, "SelectorWindow.layout");
class SelectorWindow :
	public wraps::BaseLayout,
	public Singleton2<SelectorWindow>
{
public:
	SelectorWindow();
	~SelectorWindow();

	bool isFinishedSelecting();
	void show(int type);
	void hide();

	Cache_Entry *getSelection();
	SkinPtr getSelectedSkin();
	std::vector<Ogre::String> getTruckConfig();
	void setEnableCancel(bool enabled);

	void setupCamera(Ogre::Camera* _camera) { mCamera = _camera; }
	enum loaderType {LT_Terrain, LT_Vehicle, LT_Truck, LT_Car, LT_Boat, LT_Airplane, LT_Trailer, LT_Load, LT_Extension, LT_Network, LT_NetworkWithBoat, LT_Heli, LT_SKIN, LT_AllBeam};

private:
	void eventKeyButtonPressed_Main(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
	void eventComboChangePositionTypeComboBox(MyGUI::ComboBoxPtr _sender, size_t _index);
	void eventListChangePositionModelList(MyGUI::ListPtr _sender, size_t _index);
	void eventComboAcceptConfigComboBox(MyGUI::ComboBoxPtr _sender, size_t _index);
	void eventMouseButtonClickOkButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickCancelButton(MyGUI::WidgetPtr _sender);

	// junk
	std::vector<Cache_Entry> myEntries;
	std::map<int, int> categoryUsage;
	std::vector<SkinPtr> current_skins;
	int loaderType;
	Cache_Entry *selectedtruck;
	bool selectiondone;
	std::vector<Ogre::String> truck_configs;
	SkinPtr selectedskin;
	Ogre::Camera *mCamera;


	void getData();
	void onCategorySelected(int categoryID);
	void onEntrySelected(int entryID);
	void selectionDone();
	
	void updateControls(Cache_Entry *entry);
	void setPreviewImage(Ogre::String texture);
private:
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mCacheSheet, "Cache");
	MyGUI::Sheet* mCacheSheet;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mTypeComboBox, "Type");
	MyGUI::ComboBox* mTypeComboBox;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mModelList, "Model");
	MyGUI::List* mModelList;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mEntryNameStaticText, "EntryName");
	MyGUI::StaticText* mEntryNameStaticText;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mEntryDescriptionStaticText, "EntryDescription");
	MyGUI::StaticText* mEntryDescriptionStaticText;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mConfigComboBox, "Config");
	MyGUI::ComboBox* mConfigComboBox;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mPreviewStaticImage, "Preview");
	MyGUI::StaticImage* mPreviewStaticImage;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mOkButton, "Ok");
	MyGUI::Button* mOkButton;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mCancelButton, "Cancel");
	MyGUI::Button* mCancelButton;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mSearchSheet, "Search");
	MyGUI::Sheet* mSearchSheet;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mSearchLineEdit, "SearchLine");
	MyGUI::Edit* mSearchLineEdit;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mStartSearchButton, "StartSearch");
	MyGUI::Button* mStartSearchButton;
};

#endif // __SELECTOR_WINDOW_H__

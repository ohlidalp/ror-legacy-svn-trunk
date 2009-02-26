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

#include "gui_loader.h"
#include "gui_manager.h"
#include "Ogre.h"
#include "OgreWindowEventUtilities.h" // for custom window message pump (single frame rendering)
#include "CacheSystem.h"

#include "language.h"

using namespace Ogre;

// singleton pattern
GUI_Loader* GUI_Loader::myInstance = 0;

GUI_Loader &GUI_Loader::Instance () 
{
	if (myInstance == 0)
		myInstance = new GUI_Loader;
	return *myInstance;
}

GUI_Loader::GUI_Loader()
{
	selectedtruck = 0;
	frameForced=false;
}

GUI_Loader::~GUI_Loader()
{
}

void GUI_Loader::setup(RenderWindow *_rw)
{
	rw = _rw;
	window = GETMYGUI->findWidget<MyGUI::Window>("loaderWindow");
	window->setVisible(false);

	windowp = GETMYGUI->findWidget<MyGUI::Window>("progressWindow");
	windowp->setVisible(false);

	//MyGUI::MessagePtr pGUIMsg = MyGUI::Message::createMessage(Ogre::String("aaaaa"),Ogre::String("Salir"), true, MyGUI::Message::IconWarning | MyGUI::Message::Ok);

	// MyGUI will throw an Exception on a missing part, so we dont care here
	btnOK = GETMYGUI->findWidget<MyGUI::Button>("btnOk");
	btnCancel = GETMYGUI->findWidget<MyGUI::Button>("btnCancel");
	btnSearch = GETMYGUI->findWidget<MyGUI::Button>("btnSearch");
	window = GETMYGUI->findWidget<MyGUI::Window>("loaderWindow");
	list = GETMYGUI->findWidget<MyGUI::List>("list");
	combobox = GETMYGUI->findWidget<MyGUI::ComboBox>("combobox");
	image_preview = GETMYGUI->findWidget<MyGUI::StaticImage>("image_preview");
	text_entry_name = GETMYGUI->findWidget<MyGUI::StaticText>("text_entry_name");
	text_entry_descr = GETMYGUI->findWidget<MyGUI::StaticText>("text_entry_descr");
	combo_configs = GETMYGUI->findWidget<MyGUI::ComboBox>("combo_configs");

	//we expect myGUI to throw errors when elements are not found, so we dont check the actual results

	//sheet_cache = GETMYGUI->findWidget<MyGUI::Sheet>("sheet_cache");
	//sheet_search = GETMYGUI->findWidget<MyGUI::Sheet>("sheet_search");

	//set language
	btnOK->setCaption(_L("OK"));
	btnCancel->setCaption(_L("Cancel"));
	window->setCaption(_L("Loader"));
	btnSearch->setCaption(_L("Search"));
	//sheet_cache->setCaption(_L("List"));
	//sheet_search->setCaption(_L("Search"));

	// setup controls
	combobox->setComboModeDrop(true);
	combo_configs->setComboModeDrop(true);
	combo_configs->addItem("Default", String("Default"));
	combo_configs->setIndexSelected(0);

	window->setMinSize(640, 480);
	window->setRealPosition(0.1, 0.1);
	window->setRealSize(0.8, 0.8);
	//image_preview->setAlpha(1);
	//image_preview->setInheritsAlpha(false);

	// layout valid, set up delegates
	btnOK->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Loader::event_btnOk_MouseButtonClick);
	btnCancel->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Loader::event_btnCancel_MouseButtonClick);
	btnSearch->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Loader::event_btnCancel_MouseButtonClick);
	//window->eventWindowButtonPressed = MyGUI::newDelegate(this, &GUI_Loader::notifyWindowPressed);
	combobox->eventComboAccept = MyGUI::newDelegate(this, &GUI_Loader::event_combobox_eventComboAccept);
	list->eventListChangePosition = MyGUI::newDelegate(this, &GUI_Loader::event_list_eventListChangePosition);

	window->eventKeyButtonPressed = MyGUI::newDelegate(this, &GUI_Loader::event_window_eventKeyButtonPressed); 
	
	combo_configs->eventComboAccept = MyGUI::newDelegate(this, &GUI_Loader::event_combo_configs_eventComboAccept);


	// now the progress bar
	//pwin = GETMYGUI->findWidget<MyGUI::Window>("progressWindow");
	windowp->setVisible(true);
	int pwidth=580, pheight=200;
	windowp->setMinSize(pwidth, pheight);
	windowp->setAlign(MyGUI::ALIGN_CENTER);
	windowp->setCoord(rw->getWidth()/2-pwidth/2, rw->getHeight()/2-pheight/2, pwidth, pheight);

	// fix window coordinates for inner parts
	pwidth-=10;
	pheight-=40;

	prog_text = GETMYGUI->findWidget<MyGUI::StaticText>("infolabel");
	int border = 5, pstart=(pheight/4)*3;
	prog_text->setCoord(border, border, pwidth-border*3, pstart-border);

	prog = GETMYGUI->findWidget<MyGUI::Progress>("progressbar");
	prog->setCoord(border, pstart, pwidth-border*3, pheight/4-border*2);
	prog->setEnabled(true);
	prog->setProgressRange(100);
	//prog->setProgressFillTrack(false);
	//prog->setProgressStartPoint(MyGUI::ALIGN_LEFT);
	//prog->setProgressAutoTrack(true);
}

bool GUI_Loader::getFrameForced()
{
	if(!frameForced) 
	{
		return false;
	}
	frameForced=false;
	return true;
}

void GUI_Loader::setProgress(int percent, Ogre::String text, bool updateRenderFrame)
{
	// 0-100 = valid progress
	// -1 = autotrack
	// -2 = hide
	windowp->setVisible((percent>UI_PROGRESSBAR_HIDE));
	prog_text->setCaption(text);
	if(percent>=0)
	{
		prog->setProgressAutoTrack(false);
		prog->setProgressPosition(percent);
	} else if(percent==UI_PROGRESSBAR_AUTOTRACK)
	{
		prog->setProgressPosition(0);
		prog->setProgressAutoTrack(true);
	}
	if(updateRenderFrame)
	{
		// we must pump the window messages, otherwise the window will get white on Vista ...
		WindowEventUtilities::messagePump();
		
		// render one frame
		frameForced=true;
		Ogre::Root::getSingleton().renderOneFrame();
	}
}

std::vector<Ogre::String> GUI_Loader::getTruckConfig()
{
	return truck_configs;
}

void GUI_Loader::selectionDone()
{
	if(!selectedtruck || selectiondone)
		return;

	// check if the resource is loaded
	CACHE.checkResourceLoaded(*selectedtruck);

	selectiondone = true;
	hide();
}

void GUI_Loader::event_btnOk_MouseButtonClick(MyGUI::WidgetPtr _sender)
{
	selectionDone();
}

void GUI_Loader::event_btnCancel_MouseButtonClick(MyGUI::WidgetPtr _sender)
{
	selectedtruck = 0;
	selectiondone = true;
	//btnCancel->setCaption("Cancel");
}

void GUI_Loader::event_combo_configs_eventComboAccept(MyGUI::WidgetPtr _sender, size_t _index)
{
	try
	{
		truck_configs.clear();
		String config = *combo_configs->getItemDataAt<String>(_index);
		truck_configs.push_back(config);
	} catch(...)
	{
	}
}

void GUI_Loader::event_combobox_eventComboAccept(MyGUI::WidgetPtr _sender, size_t _index)
{
	try
	{
		int categoryID = *combobox->getItemDataAt<int>(_index);
		onCategorySelected(categoryID);
	} catch(...)
	{
	}
}

void GUI_Loader::event_combobox_eventComboChangePosition(MyGUI::WidgetPtr _sender, size_t _index)
{
	try
	{
		int categoryID = *combobox->getItemDataAt<int>(_index);
		onCategorySelected(categoryID);
	} catch(...)
	{
	}
}

void GUI_Loader::event_list_eventListChangePosition(MyGUI::WidgetPtr _sender, size_t _index)
{
	try
	{
		int entryID = *list->getItemDataAt<int>(_index);
		onEntrySelected(entryID);
	} catch(...)
	{
	}
}

void GUI_Loader::event_window_eventKeyButtonPressed(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	int cid = combobox->getIndexSelected();
	int iid = list->getIndexSelected();

	// category
	if(_key == MyGUI::KC_LEFT)
	{
		int newitem = cid - 1;
		if(cid == 0)
			newitem = combobox->getItemCount() - 1;
		try
		{
			combobox->setIndexSelected(newitem);
			combobox->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		event_combobox_eventComboChangePosition(combobox, newitem);

	} else if(_key == MyGUI::KC_RIGHT)
	{
		int newitem = cid + 1;
		if(cid == (int)combobox->getItemCount() - 1)
			newitem = 0;
		try
		{
			combobox->setIndexSelected(newitem);
			combobox->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		event_combobox_eventComboChangePosition(combobox, newitem);
	}

	// items
	else if(_key == MyGUI::KC_UP)
	{
		int newitem = iid - 1;
		if(iid == 0)
			newitem = (int)list->getItemCount() - 1;
		try
		{
			list->setIndexSelected(newitem);
			list->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		event_list_eventListChangePosition(list, newitem);
	} else if(_key == MyGUI::KC_DOWN)
	{
		int newitem = iid + 1;
		if(iid == (int)list->getItemCount() - 1)
			newitem = 0;
		try
		{
			list->setIndexSelected(newitem);
			list->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		event_list_eventListChangePosition(list, newitem);
	}

	// select key
	else if(_key == MyGUI::KC_RETURN && selectedtruck)
		selectionDone();


}

bool GUI_Loader::isFinishedSelecting()
{
	return selectiondone;
}

void GUI_Loader::setEnableCancel(bool enabled)
{
	btnCancel->setEnabled(enabled);
}

void GUI_Loader::show(int type)
{
	selectiondone=false;
	setProgress(UI_PROGRESSBAR_HIDE);
	// show mouse cursor
	MyGUI::PointerManager::getInstance().setVisible(true);
	// focus main window (for key input)
	truck_configs.clear();
	MyGUI::InputManager::getInstance().setKeyFocusWidget(window);
	window->showSmooth();
	selectedtruck = 0;
	loaderType = type;
	selectiondone = false;
	getData();
}

void GUI_Loader::hide()
{
	window->setVisible(false);
	// hide cursor
	MyGUI::PointerManager::getInstance().setVisible(false);
}

Cache_Entry *GUI_Loader::getSelection()
{
	return selectedtruck;
}

void GUI_Loader::getData()
{
	combobox->removeAllItems();
	if(list->getItemCount() != 0) list->setIndexSelected(0);
	list->removeAllItems();
	myEntries.clear();
	categoryUsage.clear();
	
	int ts = CACHE.getTimeStamp();

	std::vector<Cache_Entry> *entries = CACHE.getEntries();
	std::vector<Cache_Entry>::iterator it;
	for(it = entries->begin(); it!=entries->end(); it++)
	{
		// hidden category
		if(it->categoryid == 9993) continue;

		//printf("category: %d\n", it->categoryid);
		bool add =false;
		if(it->fext=="terrn")
			add = (loaderType == LT_Terrain);
		else if(it->fext=="truck")
			add = (loaderType == LT_Vehicle || loaderType == LT_Truck || loaderType == LT_Network || loaderType == LT_NetworkWithBoat);
		else if(it->fext=="car")
			add = (loaderType == LT_Vehicle || loaderType == LT_Car || loaderType == LT_Network || loaderType == LT_NetworkWithBoat);
		else if(it->fext=="boat")
			add = (loaderType == LT_Boat || loaderType == LT_NetworkWithBoat);
		else if(it->fext=="airplane")
			add = (loaderType == LT_Airplane || loaderType == LT_Network || loaderType == LT_NetworkWithBoat);
		else if(it->fext=="trailer")
			add = (loaderType == LT_Trailer || loaderType == LT_Extension);
		else if(it->fext=="load")
			add = (loaderType == LT_Load || loaderType == LT_Extension);

		if(!add)
			continue;
		
		// remove invalid ID's
		if(it->categoryid >= 9000)
			it->categoryid = -1;

		// unsorted
		if(it->categoryid == -1)
			it->categoryid = 9990;
		
		categoryUsage[it->categoryid] = categoryUsage[it->categoryid] + 1;
		
		// all
		categoryUsage[9991] = categoryUsage[9991] + 1;
		
		// fresh, 24 hours = 86400
		if(ts - it->addtimestamp < 86400)
			categoryUsage[9992] = categoryUsage[9992] + 1;
		
		myEntries.push_back(*it);
	}
	int counter=0, counter2=0;
	std::map<int, Category_Entry> *cats = CACHE.getCategories();
	std::map<int, Category_Entry>::iterator itc;
	for(itc = cats->begin(); itc!=cats->end(); itc++)
	{
		if(categoryUsage[itc->second.number]>0)
			counter++;
	}
	for(itc = cats->begin(); itc!=cats->end(); itc++)
	{
		int usage = categoryUsage[itc->second.number];
		if(usage == 0)
			continue;
		counter2++;
		
		String txt = "["+StringConverter::toString(counter2)+"/"+StringConverter::toString(counter)+"] (" + StringConverter::toString(usage)+") "+itc->second.title;

		combobox->addItem(txt, itc->second.number);
	}
	if(combobox->getItemCount() != 0) combobox->setIndexSelected(0);
	if(counter2 > 0)
	{
		try
		{
			combobox->setIndexSelected(0);
			combobox->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		onCategorySelected(*combobox->getItemDataAt<int>(0));
	}
}

void GUI_Loader::onCategorySelected(int categoryID)
{
	int ts = CACHE.getTimeStamp();
	list->removeAllItems();
	std::vector<Cache_Entry>::iterator it;
	int counter=0, counter2=0;
	for(it = myEntries.begin(); it != myEntries.end(); it++)
	{
		if(it->categoryid == categoryID || (categoryID == 9991) || (categoryID == 9992 && (ts - it->addtimestamp < 86400)))
			counter++;
	}

	for(it = myEntries.begin(); it != myEntries.end(); it++)
	{
		if(it->categoryid == categoryID || (categoryID == 9991) || (categoryID == 9992 && (ts - it->addtimestamp < 86400)) )
		{
			counter2++;
			//printf("adding item %d\n", counter2);
			String txt = StringConverter::toString(counter2)+". " + it->dname;
			list->addItem(txt, it->number);
		}
	}
	if(counter2 > 0)
	{
		try
		{
			list->setIndexSelected(0);
			list->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		onEntrySelected(*list->getItemDataAt<int>(0));
	}
}

void GUI_Loader::onEntrySelected(int entryID)
{
	Cache_Entry *entry = CACHE.getEntry(entryID);
	if(!entry)
		return;
	int modnumber = entry->number;
	String minitype = entry->minitype;

	String outBasename = "";
	String outPath = "";
	StringUtil::splitFilename(entry->filecachename, outBasename, outPath);
	
	String tex = outBasename; // "mod-mini-"+StringConverter::toString(modnumber) + "." + minitype;
	if(minitype == "none" || tex == "none")
		tex = _L("unknown.dds");

	String group="";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(tex);
	}catch(...)
	{
	}
	if(group == "")
		tex = ("unknown.dds"); // without _T() !

	image_preview->setImageTexture(tex);
	//LogManager::getSingleton().logMessage("TEX: " + tex);

	if(entry->sectionconfigs.size())
	{
		combo_configs->setVisible(true);
		combo_configs->removeAllItems();
		for(std::vector<String>::iterator its=entry->sectionconfigs.begin();its!=entry->sectionconfigs.end(); its++)
			combo_configs->addItem(*its, *its);
	} else
		combo_configs->setVisible(false);

	String authorstxt="";
	std::vector<String> authornames;
	if(entry->authors.size() > 0)
	{
		std::vector<AuthorInfo>::iterator it;
		for(it=entry->authors.begin(); it!=entry->authors.end(); it++)
			if(it->type.size() > 2 && it->name.size() > 2)
			{
				String name = it->name;
				Ogre::StringUtil::trim(name);

				// check if already used
				bool found = false;
				std::vector<String>::iterator its;
				for(its=authornames.begin(); its!=authornames.end(); its++)
				{
					if(*its == name)
					{
						found = true;
						break;
					}
				}
				if(found)
					continue;
				authornames.push_back(name);
				authorstxt+= " " + name;
			}
	}else
		authorstxt = _L("no author information available");
	text_entry_name->setCaption(entry->dname);


	String descriptiontxt = entry->description + "\n";
	descriptiontxt += _L("Authors: ") + authorstxt + "\n";
	if(entry->wheelcount > 0) descriptiontxt += _L("Wheels: ") + StringConverter::toString(entry->wheelcount) + "x" + StringConverter::toString(entry->propwheelcount) + "\n";
	if(entry->truckmass > 0) descriptiontxt += _L("Mass: ") + StringConverter::toString((int)(entry->truckmass/1000.0f)) + " " + _L("tons") + "\n";
	if(entry->nodecount > 0) descriptiontxt += _L("Nodes: ") + StringConverter::toString(entry->nodecount) + "\n";
	if(entry->nodecount > 0) descriptiontxt += _L("Torque: ") + StringConverter::toString(entry->torque) + "\n";

	String driveableStr[5] = {_L("Non-Driveable"), _L("Truck"), _L("Airplane"), _L("Boat"), _L("Machine")};
	if(entry->nodecount > 0) descriptiontxt += _L("Vehicle Type: ") + driveableStr[entry->driveable] + "\n";

	StringUtil::trim(descriptiontxt);

	text_entry_descr->setCaption(descriptiontxt);

	selectedtruck = entry;
}

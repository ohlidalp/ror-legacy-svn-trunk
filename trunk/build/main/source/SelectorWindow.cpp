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
#include "SelectorWindow.h"
#include "LoadingWindow.h"

#include "Settings.h"
#include "language.h"
#include "CacheSystem.h"
#include "skin.h"
#include "skinmanager.h"

SelectorWindow::SelectorWindow()
{
	initialiseByAttributes(this);

	//mCamera = camera;

	mMainWidget->setVisible(false);

	//set language
	mMainWidget->setCaption(_L("Loader"));
	mOkButton->setCaption(_L("OK"));
	mCancelButton->setCaption(_L("Cancel"));
	//sheet_cache->setCaption(_L("List"));
	//sheet_search->setCaption(_L("Search"));
	mStartSearchButton->setCaption(_L("Search"));

	// setup controls
	mConfigComboBox->addItem("Default", Ogre::String("Default"));
	mConfigComboBox->setIndexSelected(0);

	mMainWidget->setRealPosition(0.1, 0.1);
	mMainWidget->setRealSize(0.8, 0.8);

	mMainWidget->eventKeyButtonPressed = MyGUI::newDelegate(this, &SelectorWindow::eventKeyButtonPressed_Main); 
	mTypeComboBox->eventComboChangePosition = MyGUI::newDelegate(this, &SelectorWindow::eventComboChangePositionTypeComboBox);
	mModelList->eventListChangePosition = MyGUI::newDelegate(this, &SelectorWindow::eventListChangePositionModelList);
	mConfigComboBox->eventComboAccept = MyGUI::newDelegate(this, &SelectorWindow::eventComboAcceptConfigComboBox);
	mOkButton->eventMouseButtonClick = MyGUI::newDelegate(this, &SelectorWindow::eventMouseButtonClickOkButton);
	mCancelButton->eventMouseButtonClick = MyGUI::newDelegate(this, &SelectorWindow::eventMouseButtonClickCancelButton);
}

SelectorWindow::~SelectorWindow()
{
}

void SelectorWindow::eventKeyButtonPressed_Main(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	if(!mMainWidget->isVisible()) return;
	int cid = mTypeComboBox->getIndexSelected();
	int iid = mModelList->getIndexSelected();

	// category
	if(_key == MyGUI::KeyCode::ArrowLeft)
	{
		int newitem = cid - 1;
		if(cid == 0)
			newitem = mTypeComboBox->getItemCount() - 1;
		try
		{
			mTypeComboBox->setIndexSelected(newitem);
			mTypeComboBox->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		eventComboChangePositionTypeComboBox(mTypeComboBox, newitem);

	} else if(_key == MyGUI::KeyCode::ArrowRight)
	{
		int newitem = cid + 1;
		if(cid == (int)mTypeComboBox->getItemCount() - 1)
			newitem = 0;
		try
		{
			mTypeComboBox->setIndexSelected(newitem);
			mTypeComboBox->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		eventComboChangePositionTypeComboBox(mTypeComboBox, newitem);
	}

	// items
	else if(_key == MyGUI::KeyCode::ArrowUp)
	{
		int newitem = iid - 1;
		if(iid == 0)
			newitem = (int)mModelList->getItemCount() - 1;
		try
		{
			mModelList->setIndexSelected(newitem);
			mModelList->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		eventListChangePositionModelList(mModelList, newitem);
	} else if(_key == MyGUI::KeyCode::ArrowDown)
	{
		int newitem = iid + 1;
		if(iid == (int)mModelList->getItemCount() - 1)
			newitem = 0;
		try
		{
			mModelList->setIndexSelected(newitem);
			mModelList->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		eventListChangePositionModelList(mModelList, newitem);
	}

	// select key
	else if(_key == MyGUI::KeyCode::Return && selectedtruck)
		selectionDone();

}

void SelectorWindow::eventMouseButtonClickOkButton(MyGUI::WidgetPtr _sender)
{
	selectionDone();
}

void SelectorWindow::eventMouseButtonClickCancelButton(MyGUI::WidgetPtr _sender)
{
	selectedtruck = 0;
	selectiondone = true;
}

void SelectorWindow::eventComboChangePositionTypeComboBox(MyGUI::ComboBoxPtr _sender, size_t _index)
{
	if(!mMainWidget->isVisible()) return;
	try
	{
		int categoryID = *mTypeComboBox->getItemDataAt<int>(_index);
		onCategorySelected(categoryID);
	} catch(...)
	{
	}
}

void SelectorWindow::eventListChangePositionModelList(MyGUI::ListPtr _sender, size_t _index)
{
	if(!mMainWidget->isVisible()) return;
	try
	{
		int entryID = *mModelList->getItemDataAt<int>(_index);
		onEntrySelected(entryID);
	} catch(...)
	{
	}
}

void SelectorWindow::eventComboAcceptConfigComboBox(MyGUI::ComboBoxPtr _sender, size_t _index)
{
	if(!mMainWidget->isVisible()) return;
	try
	{
		truck_configs.clear();
		Ogre::String config = *mConfigComboBox->getItemDataAt<Ogre::String>(_index);
		truck_configs.push_back(config);
	} catch(...)
	{
	}
}

void SelectorWindow::getData()
{
	mTypeComboBox->removeAllItems();
	if(mModelList->getItemCount() != 0) mModelList->setIndexSelected(0);
	mModelList->removeAllItems();
	myEntries.clear();
	categoryUsage.clear();
	if(loaderType == LT_SKIN)
	{
		// skin specific stuff
		mTypeComboBox->setEnabled(false);
		mTypeComboBox->setCaption(_L("Skins"));
		mCancelButton->setEnabled(false);
		mConfigComboBox->setVisible(false);

		mModelList->removeAllItems();
		mModelList->addItem(_L("Default Skin"), 0);
		int i=1;
		for(std::vector<SkinPtr>::iterator it=current_skins.begin(); it!=current_skins.end(); it++, i++)
		{
			mModelList->addItem((*it)->getName(), i);
		}
		mModelList->setIndexSelected(0);
		onEntrySelected(0);
		return;
	} else
	{
		mTypeComboBox->setEnabled(true);
		mCancelButton->setEnabled(true);
	}
	
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
		
		if(loaderType == LT_AllBeam && (it->fext == "truck" || it->fext == "car" ||  it->fext == "airplane" ||  it->fext == "trailer" ||  it->fext == "boat" || it->fext == "load"))
			add = true;

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

		mTypeComboBox->addItem(txt, itc->second.number);
	}
	if(mTypeComboBox->getItemCount() != 0) mTypeComboBox->setIndexSelected(0);
	if(counter2 > 0)
	{
		try
		{
			mTypeComboBox->setIndexSelected(0);
			mTypeComboBox->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		onCategorySelected(*mTypeComboBox->getItemDataAt<int>(0));
	}
}

void SelectorWindow::onCategorySelected(int categoryID)
{
	if(loaderType == LT_SKIN) return;
	int ts = CACHE.getTimeStamp();
	mModelList->removeAllItems();
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

			try
			{
				mModelList->addItem(txt, it->number);
			} catch(...)
			{
				mModelList->addItem("ENCODING ERROR", it->number);
			}
		}
	}
	if(counter2 > 0)
	{
		try
		{
			mModelList->setIndexSelected(0);
			mModelList->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		onEntrySelected(*mModelList->getItemDataAt<int>(0));
	}
}

void SelectorWindow::onEntrySelected(int entryID)
{
	if(loaderType == LT_SKIN)
	{
		// special skin handling
		if(entryID == 0)
		{
			// default, default infos
			updateControls(selectedtruck);
			return;
		}
		entryID -= 1; // remove default skin :)
		SkinPtr &skin = current_skins[entryID];
		
		// check if loaded
		if(!skin->loaded && skin->sourcetype == "FileSystem")
		{
			try
			{
				ResourceGroupManager::getSingleton().addResourceLocation(skin->source, "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
				ResourceGroupManager::getSingleton().initialiseResourceGroup(ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			}catch(...)
			{
			}
			skin->loaded = true;
		} else if(!skin->loaded && skin->sourcetype == "Zip")
		{
			CACHE.loadSingleZip(skin->source, -1, false, false);
			skin->loaded=true;
		}

		// set selected skin as current
		selectedskin = skin;

		setPreviewImage(current_skins[entryID]->thumbnail);

		mEntryNameStaticText->setCaption(skin->name);

		String descriptiontxt = skin->description + "\n";
		descriptiontxt += _L("Author: ") + skin->authorName + "\n";
		descriptiontxt += _L("Description: ") + skin->description + "\n";

		try
		{
			mEntryDescriptionStaticText->setCaption(descriptiontxt);
		} catch(...)
		{
			mEntryDescriptionStaticText->setCaption("ENCODING ERROR");
		}
		return;
	}
	Cache_Entry *entry = CACHE.getEntry(entryID);
	if(!entry) return;
	selectedtruck = entry;
	updateControls(selectedtruck);
}

void SelectorWindow::selectionDone()
{
	if(!selectedtruck || selectiondone)
		return;

	if(loaderType != LT_SKIN)
	{
		// we show the normal loader
		// check if the resource is loaded
		CACHE.checkResourceLoaded(*selectedtruck);

		this->current_skins.clear();
		int res = SkinManager::getSingleton().getUsableSkins(selectedtruck, this->current_skins);
		if(!res && this->current_skins.size()>0)
		{
			// show skin selection dialog!
			this->show(LT_SKIN);
			selectiondone = false;
			// just let the user select a skin as well
		} else
		{
			selectedskin.setNull();
			selectiondone = true;
			hide();
		}
	} else
	{
		// we show the skin loader, set final skin and exit!
		// selectedskin should be set already!
		selectiondone = true;
		hide();
	}
}

void SelectorWindow::updateControls(Cache_Entry *entry)
{
	int modnumber = entry->number;
	String minitype = entry->minitype;

	String outBasename = "";
	String outPath = "";
	StringUtil::splitFilename(entry->filecachename, outBasename, outPath);
	
	setPreviewImage(outBasename);

	if(entry->sectionconfigs.size())
	{
		mConfigComboBox->setVisible(true);
		mConfigComboBox->removeAllItems();
		for(std::vector<String>::iterator its=entry->sectionconfigs.begin();its!=entry->sectionconfigs.end(); its++)
		{
			try
			{
				mConfigComboBox->addItem(*its, *its);
			} catch(...)
			{
				mConfigComboBox->addItem("ENCODING ERROR", *its);
			}
		}
		mConfigComboBox->setIndexSelected(0);
		
		truck_configs.clear();
		String configstr = *mConfigComboBox->getItemDataAt<String>(0);
		truck_configs.push_back(configstr);
	} else
		mConfigComboBox->setVisible(false);

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
	} else
		authorstxt = _L("no author information available");

	try
	{
		mEntryNameStaticText->setCaption(entry->dname);
	} catch(...)
	{
		mEntryNameStaticText->setCaption("ENCODING ERROR");
	}


	String descriptiontxt = entry->description + "\n";
	descriptiontxt += _L("Authors: ") + authorstxt + "\n";
	if(entry->wheelcount > 0) descriptiontxt += _L("Wheels: ") + StringConverter::toString(entry->wheelcount) + "x" + StringConverter::toString(entry->propwheelcount) + "\n";
	if(entry->truckmass > 0) descriptiontxt += _L("Mass: ") + StringConverter::toString((int)(entry->truckmass/1000.0f)) + " " + _L("tons") + "\n";
	if(entry->nodecount > 0) descriptiontxt += _L("Nodes: ") + StringConverter::toString(entry->nodecount) + "\n";
	if(entry->nodecount > 0) descriptiontxt += _L("Torque: ") + StringConverter::toString(entry->torque) + "\n";

	String driveableStr[5] = {_L("Non-Driveable"), _L("Truck"), _L("Airplane"), _L("Boat"), _L("Machine")};
	if(entry->nodecount > 0) descriptiontxt += _L("Vehicle Type: ") + driveableStr[entry->driveable] + "\n";

	StringUtil::trim(descriptiontxt);

	try
	{
		mEntryDescriptionStaticText->setCaption(descriptiontxt);
	} catch(...)
	{
		mEntryDescriptionStaticText->setCaption("ENCODING ERROR");
	}
}

void SelectorWindow::setPreviewImage(Ogre::String texture)
{
	if(texture == "" || texture == "none")
		texture = _L("unknown.dds");

	String group="";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(texture);
	}catch(...)
	{
	}
	if(group == "")
		texture = ("unknown.dds"); // without _L() !

	mPreviewStaticImage->setImageTexture(texture);
}

bool SelectorWindow::isFinishedSelecting()
{
	return selectiondone;
}

void SelectorWindow::show(int type)
{
	if (SETTINGS.getSetting("GaussianBlur") == "Yes")
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Gaussian Blur", true);
	selectiondone=false;
	LoadingWindow::get()->hide();
	// show mouse cursor
	MyGUI::PointerManager::getInstance().setVisible(true);
	// focus main mMainWidget (for key input)
	truck_configs.clear();
	MyGUI::InputManager::getInstance().setKeyFocusWidget(mMainWidget);
	mMainWidget->setEnabledSilent(true);
	mMainWidget->castType<MyGUI::Window>()->setVisibleSmooth(true);
	if(type != LT_SKIN) selectedtruck = 0; // when in skin, we still need the info
	loaderType = type;
	selectiondone = false;
	getData();
}

void SelectorWindow::hide()
{
	if (SETTINGS.getSetting("GaussianBlur") == "Yes")
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Gaussian Blur", false);
	mMainWidget->setVisible(false);
	mMainWidget->setEnabledSilent(false);
	// hide cursor
	MyGUI::PointerManager::getInstance().setVisible(false);
}

Cache_Entry *SelectorWindow::getSelection()
{
	return selectedtruck;
}

SkinPtr SelectorWindow::getSelectedSkin()
{
	return selectedskin;
}

std::vector<Ogre::String> SelectorWindow::getTruckConfig()
{
	return truck_configs;
}

void SelectorWindow::setEnableCancel(bool enabled)
{
	mCancelButton->setEnabled(enabled);
}

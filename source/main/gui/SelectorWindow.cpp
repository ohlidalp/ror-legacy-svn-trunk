/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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

#include "SelectorWindow.h"
#include "LoadingWindow.h"

#include "gui_manager.h"

#include "Settings.h"
#include "language.h"
#include "CacheSystem.h"
#include "skin.h"
#include "utils.h"
#include "skinmanager.h"
#include "BeamData.h"
#include "InputEngine.h"

#if 0
// translation help for category entries, should be commented at all times
_L("Other Land Vehicles");
_L("Street Cars");
_L("Light Racing Cars");
_L("Offroad Cars");
_L("Fantasy Cars");
_L("Bikes");
_L("Crawlers");
_L("Towercranes");
_L("Mobile Cranes");
_L("Other cranes");
_L("Buses");
_L("Tractors");
_L("Forklifts");
_L("Fantasy Trucks");
_L("Transport Trucks");
_L("Racing Trucks");
_L("Offroad Trucks");
_L("Boats");
_L("Helicopters");
_L("Aircraft");
_L("Trailers");
_L("Other Loads");
_L("Addon Terrains");
_L("Official Terrains");
_L("Night Terrains");
_L("Unsorted");
_L("All");
_L("Fresh");
_L("Hidden");
#endif // 0

SelectorWindow::SelectorWindow() : mSelectedTruck(0)
{
	initialiseByAttributes(this);

	visibleCounter=0;
	mMainWidget->setVisible(false);
	((MyGUI::Window*)mMainWidget)->setCaption(_L("Loader"));
	mSearchLineEdit->setCaption(_L("Search ..."));
	mOkButton->setCaption(_L("OK"));
	mCancelButton->setCaption(_L("Cancel"));
	

	// setup controls
	mConfigComboBox->addItem("Default", Ogre::String("Default"));
	mConfigComboBox->setIndexSelected(0);

	mMainWidget->setRealPosition(0.1, 0.1);
	mMainWidget->setRealSize(0.8, 0.8);

	mMainWidget->eventKeyButtonPressed      += MyGUI::newDelegate(this, &SelectorWindow::eventKeyButtonPressed_Main);

	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
	win->eventWindowChangeCoord             += MyGUI::newDelegate(this, &SelectorWindow::notifyWindowChangeCoord);

	mTypeComboBox->eventComboChangePosition += MyGUI::newDelegate(this, &SelectorWindow::eventComboChangePositionTypeComboBox);
	mTypeComboBox->eventKeyButtonPressed    += MyGUI::newDelegate(this, &SelectorWindow::eventKeyButtonPressed_Main);
	
	mModelList->eventListSelectAccept       += MyGUI::newDelegate(this, &SelectorWindow::eventListChangePositionModelListAccept);
	mModelList->eventListChangePosition     += MyGUI::newDelegate(this, &SelectorWindow::eventListChangePositionModelList);
	//mModelList->eventKeyButtonPressed       += MyGUI::newDelegate(this, &SelectorWindow::eventKeyButtonPressed_Main);
	mConfigComboBox->eventComboAccept       += MyGUI::newDelegate(this, &SelectorWindow::eventComboAcceptConfigComboBox);
	//mConfigComboBox->eventKeyButtonPressed  += MyGUI::newDelegate(this, &SelectorWindow::eventKeyButtonPressed_Main);
	mOkButton->eventMouseButtonClick        += MyGUI::newDelegate(this, &SelectorWindow::eventMouseButtonClickOkButton);
	mCancelButton->eventMouseButtonClick    += MyGUI::newDelegate(this, &SelectorWindow::eventMouseButtonClickCancelButton);

	// search stuff
	mSearchLineEdit->eventEditTextChange    += MyGUI::newDelegate(this, &SelectorWindow::eventSearchTextChange);
	mSearchLineEdit->eventMouseSetFocus     += MyGUI::newDelegate(this, &SelectorWindow::eventSearchTextGotFocus);
	mSearchLineEdit->eventKeySetFocus       += MyGUI::newDelegate(this, &SelectorWindow::eventSearchTextGotFocus);
	mSearchLineEdit->eventKeyButtonPressed  += MyGUI::newDelegate(this, &SelectorWindow::eventKeyButtonPressed_Main);

	mSelectedSkin=0;
}

SelectorWindow::~SelectorWindow()
{
}

void SelectorWindow::notifyWindowChangeCoord(MyGUI::Window* _sender)
{
	resizePreviewImage();
}

void SelectorWindow::eventKeyButtonPressed_Main(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	if(!mMainWidget->getVisible()) return;
	int cid = (int)mTypeComboBox->getIndexSelected();
	int iid = (int)mModelList->getIndexSelected();

	bool searching = (mTypeComboBox->getCaption() == _L("Search Results"));

	// search
	if(_key == MyGUI::KeyCode::Slash)
	{
		MyGUI::InputManager::getInstance().setKeyFocusWidget(mSearchLineEdit);
		mSearchLineEdit->setCaption("");
		searching=true;
	}

	// category
	if(_key == MyGUI::KeyCode::ArrowLeft && !searching)
	{
		int newitem = cid - 1;
		if(cid == 0)
			newitem = (int)mTypeComboBox->getItemCount() - 1;
		try
		{
			mTypeComboBox->setIndexSelected(newitem);
			mTypeComboBox->beginToItemSelected();
		} catch(...)
		{
			return;
		}
		eventComboChangePositionTypeComboBox(mTypeComboBox, newitem);

	} else if(_key == MyGUI::KeyCode::ArrowRight && !searching)
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
		// fix cursor position
		if(searching) mSearchLineEdit->setTextCursor(mSearchLineEdit->getTextLength());
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
		// fix cursor position
		if(searching) mSearchLineEdit->setTextCursor(mSearchLineEdit->getTextLength());
	}

	// select key
	else if(mLoaderType != LT_SKIN && _key == MyGUI::KeyCode::Return && mSelectedTruck)
	{
		selectionDone();

	} else if(mLoaderType == LT_SKIN && _key == MyGUI::KeyCode::Return && mSelectedSkin)
	{
		selectionDone();
	}

}

void SelectorWindow::eventMouseButtonClickOkButton(MyGUI::WidgetPtr _sender)
{
	selectionDone();
}

void SelectorWindow::eventMouseButtonClickCancelButton(MyGUI::WidgetPtr _sender)
{
	mSelectedTruck = nullptr;
	mSelectionDone = true;
	hide();
}

void SelectorWindow::eventComboChangePositionTypeComboBox(MyGUI::ComboBoxPtr _sender, size_t _index)
{
	if(!mMainWidget->getVisible()) return;
	try
	{
		int categoryID = *mTypeComboBox->getItemDataAt<int>(_index);
		mSearchLineEdit->setCaption(_L("Search ..."));
		onCategorySelected(categoryID);
	} catch(...)
	{
	}
}

void SelectorWindow::eventListChangePositionModelListAccept(MyGUI::ListPtr _sender, size_t _index)
{
	eventListChangePositionModelList(_sender, _index);
	selectionDone();
}

void SelectorWindow::eventListChangePositionModelList(MyGUI::ListPtr _sender, size_t _index)
{
	if(!mMainWidget->getVisible()) return;
	
	if(_index < 0 || _index >= mModelList->getItemCount()) return;
	
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
	if(!mMainWidget->getVisible()) return;
	try
	{
		mTruckConfigs.clear();
		Ogre::String config = *mConfigComboBox->getItemDataAt<Ogre::String>(_index);
		mTruckConfigs.push_back(config);
	} catch(...)
	{
	}
}

void SelectorWindow::getData()
{
	mTypeComboBox->removeAllItems();
	if(mModelList->getItemCount() != 0) mModelList->setIndexSelected(0);
	mModelList->removeAllItems();
	mEntries.clear();
	mCategoryUsage.clear();
	if(mLoaderType == LT_SKIN)
	{
		// skin specific stuff
		mTypeComboBox->setEnabled(false);
		mTypeComboBox->setCaption(_L("Skins"));
		mCancelButton->setEnabled(false);
		mConfigComboBox->setVisible(false);

		mModelList->removeAllItems();
		mModelList->addItem(_L("Default Skin"), 0);
		int i=1;
		for(std::vector<Skin *>::iterator it=mCurrentSkins.begin(); it!=mCurrentSkins.end(); it++, i++)
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

	int ts = getTimeStamp();
	std::vector<Cache_Entry> *entries = CACHE.getEntries();
	std::vector<Cache_Entry>::iterator it;
	for(it = entries->begin(); it!=entries->end(); it++)
	{
		// hidden category
		if(it->categoryid == 9993) continue;

		//printf("category: %d\n", it->categoryid);
		bool add =false;
		if(it->fext=="terrn")
			add = (mLoaderType == LT_Terrain);
		else if(it->fext=="truck")
			add = (mLoaderType == LT_Vehicle || mLoaderType == LT_Truck || mLoaderType == LT_Network || mLoaderType == LT_NetworkWithBoat);
		else if(it->fext=="car")
			add = (mLoaderType == LT_Vehicle || mLoaderType == LT_Car || mLoaderType == LT_Network || mLoaderType == LT_NetworkWithBoat);
		else if(it->fext=="boat")
			add = (mLoaderType == LT_Boat || mLoaderType == LT_NetworkWithBoat);
		else if(it->fext=="airplane")
			add = (mLoaderType == LT_Airplane || mLoaderType == LT_Network || mLoaderType == LT_NetworkWithBoat);
		else if(it->fext=="trailer")
			add = (mLoaderType == LT_Trailer || mLoaderType == LT_Extension);
		else if(it->fext=="load")
			add = (mLoaderType == LT_Load || mLoaderType == LT_Extension);

		if(mLoaderType == LT_AllBeam && (it->fext == "truck" || it->fext == "car" ||  it->fext == "airplane" ||  it->fext == "trailer" ||  it->fext == "boat" || it->fext == "load"))
			add = true;

		if(!add)
			continue;

		// remove invalid ID's
		if(it->categoryid >= 9000)
			it->categoryid = -1;

		// unsorted
		if(it->categoryid == -1)
			it->categoryid = 9990;

		mCategoryUsage[it->categoryid] = mCategoryUsage[it->categoryid] + 1;

		// all
		mCategoryUsage[9991] = mCategoryUsage[9991] + 1;

		// fresh, 24 hours = 86400
		if(ts - it->addtimestamp < 86400)
			mCategoryUsage[9992] = mCategoryUsage[9992] + 1;

		// hidden
		//mCategoryUsage[9993] = 0;
		// search results
		mCategoryUsage[9994] = 0;


		mEntries.push_back(*it);
	}
	int counter=0, counter2=0;
	std::map<int, Category_Entry> *cats = CACHE.getCategories();
	std::map<int, Category_Entry>::iterator itc;
	for(itc = cats->begin(); itc!=cats->end(); itc++)
	{
		if(mCategoryUsage[itc->second.number]>0)
			counter++;
	}
	for(itc = cats->begin(); itc!=cats->end(); itc++)
	{
		int usage = mCategoryUsage[itc->second.number];
		if(usage == 0)
			continue;
		counter2++;

		UTFString title = _L(itc->second.title.c_str());
		if(title.empty())
			title = _L("unknown");
		UTFString txt = U("[") + TOUTFSTRING(counter2) + U("/") + TOUTFSTRING(counter) + U("] (") + TOUTFSTRING(usage) + U(") ") + title;

		mTypeComboBox->addItem(convertToMyGUIString(txt), itc->second.number);
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

bool SelectorWindow::searchCompare(Ogre::String searchString, Cache_Entry *ce)
{
	if(searchString.find(":") == searchString.npos)
	{
		// normal search

		// the name
		String dname_lower = ce->dname;
		Ogre::StringUtil::toLowerCase(dname_lower);
		if(dname_lower.find(searchString) != String::npos)
			return true;
		
		// the filename
		String fname_lower = ce->fname;
		Ogre::StringUtil::toLowerCase(fname_lower);
		if(fname_lower.find(searchString) != String::npos)
			return true;

		// the description
		String desc = ce->description;
		Ogre::StringUtil::toLowerCase(desc);
		if(desc.find(searchString) != String::npos)
			return true;

		// the authors
		if(!ce->authors.empty())
		{
			std::vector<authorinfo_t>::const_iterator it;
			for(it = ce->authors.begin(); it != ce->authors.end(); it++)
			{
				// author name
				String aname = it->name;
				Ogre::StringUtil::toLowerCase(aname);
				if(aname.find(searchString) != String::npos)
					return true;

				// author email
				String aemail = it->email;
				Ogre::StringUtil::toLowerCase(aemail);
				if(aemail.find(searchString) != String::npos)
					return true;
			}
		}
		return false;
	} else
	{
		StringVector v = StringUtil::split(searchString, ":");
		if(v.size() < 2) return false; //invalid syntax

		if(v[0] == "hash")
		{
			String hash = ce->hash;
			Ogre::StringUtil::toLowerCase(hash);
			return (hash.find(v[1]) != String::npos);
		} else if(v[0] == "guid")
		{
			String guid = ce->guid;
			Ogre::StringUtil::toLowerCase(guid);
			return (guid.find(v[1]) != String::npos);
		} else if(v[0] == "author")
		{
			// the authors
			if(!ce->authors.empty())
			{
				std::vector<authorinfo_t>::const_iterator it;
				for(it = ce->authors.begin(); it != ce->authors.end(); it++)
				{
					// author name
					String aname = it->name;
					Ogre::StringUtil::toLowerCase(aname);
					if(aname.find(v[1]) != String::npos)
						return true;

					// author email
					String aemail = it->email;
					Ogre::StringUtil::toLowerCase(aemail);
					if(aemail.find(v[1]) != String::npos)
						return true;
				}
			}
			return false;
		} else if(v[0] == "wheels")
		{
			String wheelsStr = TOUTFSTRING(ce->wheelcount) + "x" + TOUTFSTRING(ce->propwheelcount);
			return (wheelsStr == v[1]);
		} else if(v[0] == "file")
		{
			String fn = ce->fname;
			Ogre::StringUtil::toLowerCase(fn);
			return (fn.find(v[1]) != String::npos);
		}

		
	}
	return false;
}

void SelectorWindow::onCategorySelected(int categoryID)
{
	if(mLoaderType == LT_SKIN) return;

	String search_cmd = mSearchLineEdit->getCaption();
	Ogre::StringUtil::toLowerCase(search_cmd);

	int ts = getTimeStamp();
	mModelList->removeAllItems();
	std::vector<Cache_Entry>::iterator it;
	int counter=0, counter2=0;
	// re-test too see how much matches
	for(it = mEntries.begin(); it != mEntries.end(); it++)
	{
		if(it->categoryid == categoryID || (categoryID == 9991) || (categoryID == 9992 && (ts - it->addtimestamp < 86400)))
			counter++;

		// search results
		if(categoryID == 9994 && searchCompare(search_cmd, &(*it)))
		{
			counter++;
		}
	}

	for(it = mEntries.begin(); it != mEntries.end(); it++)
	{
		if(it->categoryid == categoryID || (categoryID == 9991) || (categoryID == 9992 && (ts - it->addtimestamp < 86400)) )
		{
			counter2++;
			//printf("adding item %d\n", counter2);
			String txt = TOSTRING(counter2)+". " + it->dname;

			try
			{
				mModelList->addItem(txt, it->number);
			} catch(...)
			{
				mModelList->addItem("ENCODING ERROR", it->number);
			}
		} else if(categoryID == 9994 && searchCompare(search_cmd, &(*it)))
		{
			counter2++;
			//printf("adding item %d\n", counter2);
			String txt = TOSTRING(counter2)+". " + it->dname;
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
	if(mLoaderType == LT_SKIN)
	{
		// special skin handling
		if(entryID == 0)
		{
			// default, default infos
			updateControls(mSelectedTruck);
			return;
		}
		entryID -= 1; // remove default skin :)
		Skin *skin = mCurrentSkins[entryID];

		// we assume its already loaded
		// set selected skin as current
		mSelectedSkin = skin;

		setPreviewImage(mCurrentSkins[entryID]->thumbnail);

		mEntryNameStaticText->setCaption(skin->name);

		String descriptiontxt = skin->description + String("\n");
		descriptiontxt = descriptiontxt +_L("Author(s): ") + skin->authorName + String("\n");
		descriptiontxt = descriptiontxt +_L("Description: ") + skin->description + String("\n");

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
	mSelectedTruck = entry;
	updateControls(mSelectedTruck);
}

void SelectorWindow::selectionDone()
{
	if(!mSelectedTruck || mSelectionDone)
		return;
	
	if(mLoaderType != LT_SKIN)
	{
		// we show the normal loader
		// check if the resource is loaded
		CACHE.checkResourceLoaded(*mSelectedTruck);

		this->mCurrentSkins.clear();
		int res = SkinManager::getSingleton().getUsableSkins(mSelectedTruck->guid, this->mCurrentSkins);
		if(!res && this->mCurrentSkins.size()>0)
		{
			// show skin selection dialog!
			this->show(LT_SKIN);
			mSelectionDone = false;
			// just let the user select a skin as well
		} else
		{
			mSelectedSkin = 0;
			mSelectionDone = true;
			hide();
		}
	} else
	{
		// we show the skin loader, set final skin and exit!
		// mSelectedSkin should be set already!
		mSelectionDone = true;
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

		mTruckConfigs.clear();
		String configstr = *mConfigComboBox->getItemDataAt<String>(0);
		mTruckConfigs.push_back(configstr);
	} else
		mConfigComboBox->setVisible(false);

	UTFString authorstxt;
	std::vector<String> authornames;
	if(entry->authors.size() > 0)
	{
		std::vector<authorinfo_t>::iterator it;
		for(it=entry->authors.begin(); it!=entry->authors.end(); it++)
			if(!it->type.empty() && !it->name.empty())
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
				authorstxt = authorstxt + U(" ") + name;
			}
	} else
		authorstxt = _L("no author information available");

	try
	{
		mEntryNameStaticText->setCaption(convertToMyGUIString(ANSI_TO_UTF(entry->dname)));
	} catch(...)
	{
		mEntryNameStaticText->setCaption("ENCODING ERROR");
	}

	UTFString c       = U("#257900"); // colour key shortcut
	UTFString nc      = U("#000000"); // colour key shortcut
	UTFString newline = U("\n");

	UTFString descriptiontxt = U("#003dae") + ANSI_TO_UTF(entry->description) + nc + newline;
	descriptiontxt = descriptiontxt +_L("Author(s): ") + c + authorstxt + nc +newline;

	
	if(entry->version > 0) descriptiontxt                = descriptiontxt + _L("Version: ")   + c + TOUTFSTRING(entry->version) + nc + newline;
	if(entry->wheelcount > 0) descriptiontxt             = descriptiontxt + _L("Wheels: ")    + c + TOUTFSTRING(entry->wheelcount) + U("x") + TOUTFSTRING(entry->propwheelcount) + nc + newline;
	if(entry->truckmass > 0) descriptiontxt              = descriptiontxt + _L("Mass: ")      + c + TOUTFSTRING((int)(entry->truckmass/1000.0f)) + U(" ") + _L("tons") + nc + newline;
	if(entry->loadmass > 0) descriptiontxt               = descriptiontxt + _L("Load Mass: ") + c + TOUTFSTRING((int)(entry->loadmass/1000.0f)) + U(" ") + _L("tons") + nc + newline;
	if(entry->nodecount > 0) descriptiontxt              = descriptiontxt + _L("Nodes: ")     + c + TOUTFSTRING(entry->nodecount) + nc + newline;
	if(entry->beamcount > 0) descriptiontxt              = descriptiontxt + _L("Beams: ")     + c + TOUTFSTRING(entry->beamcount) + nc + newline;
	if(entry->shockcount > 0) descriptiontxt             = descriptiontxt + _L("Shocks: ")    + c + TOUTFSTRING(entry->shockcount) + nc + newline;
	if(entry->hydroscount > 0) descriptiontxt            = descriptiontxt + _L("Hydros: ")    + c + TOUTFSTRING(entry->hydroscount) + nc + newline;
	if(entry->soundsourcescount > 0) descriptiontxt      = descriptiontxt + _L("SoundSources: ") + c + TOUTFSTRING(entry->soundsourcescount) + nc + newline;
	if(entry->commandscount > 0) descriptiontxt          = descriptiontxt + _L("Commands: ")  + c + TOUTFSTRING(entry->commandscount) + nc + newline;
	if(entry->rotatorscount > 0) descriptiontxt          = descriptiontxt + _L("Rotators: ")  + c + TOUTFSTRING(entry->rotatorscount) + nc + newline;
	if(entry->exhaustscount > 0) descriptiontxt          = descriptiontxt + _L("Exhausts: ")  + c + TOUTFSTRING(entry->exhaustscount) + nc + newline;
	if(entry->flarescount > 0) descriptiontxt            = descriptiontxt + _L("Flares: ")    + c + TOUTFSTRING(entry->flarescount) + nc + newline;
	if(entry->torque > 0) descriptiontxt                 = descriptiontxt + _L("Torque: ")    + c + TOUTFSTRING(entry->torque) + nc + newline;
	if(entry->flexbodiescount > 0) descriptiontxt        = descriptiontxt + _L("Flexbodies: ") + c + TOUTFSTRING(entry->flexbodiescount) + nc + newline;
	if(entry->propscount > 0) descriptiontxt             = descriptiontxt + _L("Props: ")     + c + TOUTFSTRING(entry->propscount) + nc + newline;
	if(entry->wingscount > 0) descriptiontxt             = descriptiontxt + _L("Wings: ")     + c + TOUTFSTRING(entry->wingscount) + nc + newline;
	if(entry->hasSubmeshs) descriptiontxt                = descriptiontxt + _L("Using Submeshs: ") + c + TOUTFSTRING(entry->hasSubmeshs) + nc + newline;
	if(entry->numgears > 0) descriptiontxt               = descriptiontxt + _L("Transmission Gear Count: ") + c + TOUTFSTRING(entry->numgears) + nc + newline;
	if(entry->minrpm > 0) descriptiontxt                 = descriptiontxt + _L("Engine RPM: ") + c + TOUTFSTRING(entry->minrpm) + U(" - ") + TOUTFSTRING(entry->maxrpm) + nc + newline;
	if(!entry->uniqueid.empty() && entry->uniqueid != "no-uid") descriptiontxt = descriptiontxt + _L("Unique ID: ") + c + entry->uniqueid + nc + newline;
	if(!entry->guid.empty() && entry->guid != "no-guid") descriptiontxt = descriptiontxt + _L("GUID: ") + c + entry->guid + nc + newline;
	if(entry->usagecounter > 0) descriptiontxt           = descriptiontxt + _L("Times used: ") + c + TOUTFSTRING(entry->usagecounter) + nc + newline;

	if(entry->addtimestamp > 0)
	{
		char tmp[255] = "";
		time_t epch = entry->addtimestamp;
		sprintf(tmp, "%s", asctime(gmtime(&epch)));
		descriptiontxt = descriptiontxt +_L("Date and Time installed: ") + c + String(tmp) + nc + newline;
	}

	UTFString driveableStr[5] = {_L("Non-Driveable"), _L("Truck"), _L("Airplane"), _L("Boat"), _L("Machine")};
	if(entry->nodecount > 0) descriptiontxt = descriptiontxt +_L("Vehicle Type: ") + c + driveableStr[entry->driveable] + nc + newline;

	descriptiontxt = descriptiontxt +"#448b9a\n"; // different colour for the props

	if(entry->forwardcommands) descriptiontxt = descriptiontxt +_L("[forwards commands]") + newline;
	if(entry->importcommands) descriptiontxt = descriptiontxt +_L("[imports commands]") + newline;
	if(entry->rollon) descriptiontxt = descriptiontxt +_L("[is rollon]") + newline;
	if(entry->rescuer) descriptiontxt = descriptiontxt +_L("[is rescuer]") + newline;
	if(entry->custom_particles) descriptiontxt = descriptiontxt +_L("[uses custom particles]") + newline;
	if(entry->fixescount > 0) descriptiontxt = descriptiontxt +_L("[has fixes]") + newline;
	// t is the default, do not display it
	//if(entry->enginetype == 't') descriptiontxt = descriptiontxt +_L("[TRUCK ENGINE]") + newline;
	if(entry->enginetype == 'c') descriptiontxt = descriptiontxt +_L("[car engine]") + newline;
	if(entry->type == "Zip") descriptiontxt = descriptiontxt +_L("[zip archive]") + newline;
	if(entry->type == "FileSystem") descriptiontxt = descriptiontxt +_L("[unpacked in directory]") + newline;

	descriptiontxt = descriptiontxt +"#666666\n"; // now grey-ish colour

	if(!entry->dirname.empty()) descriptiontxt = descriptiontxt +_L("Source: ") + entry->dirname + newline;
	if(!entry->fname.empty()) descriptiontxt = descriptiontxt +_L("Filename: ") + entry->fname + newline;
	if(!entry->hash.empty() && entry->hash != "none") descriptiontxt = descriptiontxt +_L("Hash: ") + entry->hash + newline;
	if(!entry->hash.empty()) descriptiontxt = descriptiontxt +_L("Mod Number: ") + TOUTFSTRING(entry->number) + newline;
	
	if(!entry->sectionconfigs.empty())
	{
		descriptiontxt = descriptiontxt + U("\n\n#e10000") + _L("Please select a configuration below!") + nc + U("\n\n");
	}

	trimUTFString(descriptiontxt);

	mEntryDescriptionStaticText->setCaption(convertToMyGUIString(descriptiontxt));
}

void SelectorWindow::setPreviewImage(Ogre::String texture)
{
	if(texture == "" || texture == "none")
	{
		mPreviewStaticImage->setVisible(false);
		return;
	}

	String group="";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(texture);
	}catch(...)
	{
	}
	if(group == "")
	{
		// texture not found, hide widget
		mPreviewStaticImage->setVisible(false);
		return;
	}
	mPreviewStaticImage->setVisible(true);

	mPreviewStaticImage->setImageTexture(texture);
	lastImageTextureName = texture;

	resizePreviewImage();
}

void SelectorWindow::resizePreviewImage()
{
	// now get the texture size
	MyGUI::IntSize imgSize(0,0);
	TexturePtr t = Ogre::TextureManager::getSingleton().load(lastImageTextureName,Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
	if(!t.isNull())
	{
		imgSize.width  = (int)t->getWidth() * 10;
		imgSize.height = (int)t->getHeight() * 10;
	}

	if(imgSize.width != 0)
	{
		MyGUI::IntSize maxSize = mPreviewStaticImagePanel->getSize();

		float imgRatio = imgSize.width / (float)imgSize.height;
		float maxRatio = maxSize.width / (float)maxSize.height;

		MyGUI::IntSize newSize;
		MyGUI::IntPoint newPosition;

		// now scale with aspect ratio
		if (imgRatio > maxRatio)
		{
			newSize.width  = maxSize.width;
			newSize.height = maxSize.width / imgRatio;
			newPosition.left = 0;
			newPosition.top  = maxSize.height - newSize.height;
		}
		else
		{
			newSize.width  = maxSize.height * imgRatio;
			newSize.height = maxSize.height;
			newPosition.left = maxSize.width - newSize.width;
			newPosition.top  = 0;
		}

		mPreviewStaticImage->setSize(newSize);
		mPreviewStaticImage->setPosition(newPosition);
	}
}

bool SelectorWindow::isFinishedSelecting()
{
	return mSelectionDone;
}

void SelectorWindow::show(LoaderType type)
{
	mSelectedSkin=0;
	mSearchLineEdit->setCaption(_L("Search ..."));
	mSelectionDone=false;
	// reset all keys
	INPUTENGINE.resetKeys();
	LoadingWindow::get()->hide();
	// focus main mMainWidget (for key input)
	mTruckConfigs.clear();
	MyGUI::InputManager::getInstance().setKeyFocusWidget(mMainWidget);
	mMainWidget->setEnabledSilent(true);
	// first time fast
	if(!visibleCounter)
		mMainWidget->castType<MyGUI::Window>()->setVisible(true);
	else
		mMainWidget->castType<MyGUI::Window>()->setVisibleSmooth(true);
	if (type != LT_SKIN) mSelectedTruck = nullptr; // when in skin, we still need the info

	mLoaderType = type;
	mSelectionDone = false;
	getData();
	visibleCounter++;
}

void SelectorWindow::hide()
{
	GUIManager::getSingleton().unfocus();
	mMainWidget->setVisible(false);
	mMainWidget->setEnabledSilent(false);
}

void SelectorWindow::setEnableCancel(bool enabled)
{
	mCancelButton->setEnabled(enabled);
}

void SelectorWindow::eventSearchTextChange(MyGUI::EditBox *_sender)
{
	if(!mMainWidget->getVisible()) return;
	onCategorySelected(9994);
	mTypeComboBox->setCaption(_L("Search Results"));
}

void SelectorWindow::eventSearchTextGotFocus(MyGUI::WidgetPtr _sender, MyGUI::WidgetPtr oldWidget)
{
	if(!mMainWidget->getVisible()) return;
	
	if(mSearchLineEdit->getCaption() == _L("Search ..."))
		mSearchLineEdit->setCaption("");
}
#endif //MYGUI


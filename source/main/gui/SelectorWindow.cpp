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

SelectorWindow::SelectorWindow() : mSelectedTruck(0)
{
	initialiseByAttributes(this);

	visibleCounter=0;
	mMainWidget->setVisible(false);

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
	int cid = mTypeComboBox->getIndexSelected();
	int iid = mModelList->getIndexSelected();

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

void SelectorWindow::eventListChangePositionModelList(MyGUI::ListPtr _sender, size_t _index)
{
	if(!mMainWidget->getVisible()) return;
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

		String title = itc->second.title;
		if(title.empty())
			title = _L("unknown");
		String txt = "["+TOSTRING(counter2)+"/"+TOSTRING(counter)+"] (" + TOSTRING(usage)+") " + title;

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
			String wheelsStr = TOSTRING(ce->wheelcount) + "x" + TOSTRING(ce->propwheelcount);
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

	String authorstxt="";
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
	
	if(entry->version > 0) descriptiontxt += _L("Version: ") + TOSTRING(entry->version) + "\n";
	if(entry->wheelcount > 0) descriptiontxt += _L("Wheels: ") + TOSTRING(entry->wheelcount) + "x" + TOSTRING(entry->propwheelcount) + "\n";
	if(entry->truckmass > 0) descriptiontxt += _L("Mass: ") + TOSTRING((int)(entry->truckmass/1000.0f)) + " " + _L("tons") + "\n";
	if(entry->loadmass > 0) descriptiontxt += _L("Load Mass: ") + TOSTRING((int)(entry->loadmass/1000.0f)) + " " + _L("tons") + "\n";
	if(entry->nodecount > 0) descriptiontxt += _L("Nodes: ") + TOSTRING(entry->nodecount) + "\n";
	if(entry->beamcount > 0) descriptiontxt += _L("Beams: ") + TOSTRING(entry->beamcount) + "\n";
	if(entry->shockcount > 0) descriptiontxt += _L("Shocks: ") + TOSTRING(entry->shockcount) + "\n";
	if(entry->hydroscount > 0) descriptiontxt += _L("Hydros: ") + TOSTRING(entry->hydroscount) + "\n";
	if(entry->soundsourcescount > 0) descriptiontxt += _L("SoundSources: ") + TOSTRING(entry->soundsourcescount) + "\n";
	if(entry->commandscount > 0) descriptiontxt += _L("Commands: ") + TOSTRING(entry->commandscount) + "\n";
	if(entry->rotatorscount > 0) descriptiontxt += _L("Rotators: ") + TOSTRING(entry->rotatorscount) + "\n";
	if(entry->exhaustscount > 0) descriptiontxt += _L("Exhausts: ") + TOSTRING(entry->exhaustscount) + "\n";
	if(entry->flarescount > 0) descriptiontxt += _L("Flares: ") + TOSTRING(entry->flarescount) + "\n";
	if(entry->torque > 0) descriptiontxt += _L("Torque: ") + TOSTRING(entry->torque) + "\n";
	if(entry->flexbodiescount > 0) descriptiontxt += _L("Flexbodies: ") + TOSTRING(entry->flexbodiescount) + "\n";
	if(entry->propscount > 0) descriptiontxt += _L("Props: ") + TOSTRING(entry->propscount) + "\n";
	if(entry->wingscount > 0) descriptiontxt += _L("Wings: ") + TOSTRING(entry->wingscount) + "\n";
	if(entry->hasSubmeshs) descriptiontxt += _L("Using Submeshs: ") + TOSTRING(entry->hasSubmeshs) + "\n";
	if(entry->numgears > 0) descriptiontxt += _L("Transmission Gear Count: ") + TOSTRING(entry->numgears) + "\n";
	if(entry->minrpm > 0) descriptiontxt += _L("Engine RPM: ") + TOSTRING(entry->minrpm) + " - " + TOSTRING(entry->maxrpm) + "\n";
	if(!entry->uniqueid.empty() && entry->uniqueid != "no-uid") descriptiontxt += _L("Unique ID: ") + entry->uniqueid + "\n";
	if(!entry->guid.empty() && entry->guid != "no-guid") descriptiontxt += _L("GUID: ") + entry->guid + "\n";
	if(entry->usagecounter > 0) descriptiontxt += _L("Times used: ") + TOSTRING(entry->usagecounter) + "\n";

	if(entry->addtimestamp > 0)
	{
		char tmp[255] = "";
		time_t epch = entry->addtimestamp;
		sprintf(tmp, "%s", asctime(gmtime(&epch)));
		descriptiontxt += _L("Date and Time installed: ") + String(tmp) + "\n";
	}

	String driveableStr[5] = {_L("Non-Driveable"), _L("Truck"), _L("Airplane"), _L("Boat"), _L("Machine")};
	if(entry->nodecount > 0) descriptiontxt += _L("Vehicle Type: ") + driveableStr[entry->driveable] + "\n";

	descriptiontxt += "\n";

	if(entry->forwardcommands) descriptiontxt += _L("[forwards commands]") + "\n";
	if(entry->importcommands) descriptiontxt += _L("[imports commands]") + "\n";
	if(entry->rollon) descriptiontxt += _L("[is rollon]") + "\n";
	if(entry->rescuer) descriptiontxt += _L("[is rescuer]") + "\n";
	if(entry->custom_particles) descriptiontxt += _L("[uses custom particles]") + "\n";

	if(entry->fixescount > 0) descriptiontxt += _L("[has fixes]") + "\n";
	
	// t is the default, do not display it
	//if(entry->enginetype == 't') descriptiontxt += _L("[TRUCK ENGINE]") + "\n";
	if(entry->enginetype == 'c') descriptiontxt += _L("[car engine]") + "\n";

	if(entry->type == "Zip") descriptiontxt += _L("[zip archive]") + "\n";
	if(entry->type == "FileSystem") descriptiontxt += _L("[unpacked in directory]") + "\n";

	descriptiontxt += "\n";

	if(!entry->dirname.empty()) descriptiontxt += _L("Source: ") + entry->dirname + "\n";
	if(!entry->fname.empty()) descriptiontxt += _L("Filename: ") + entry->fname + "\n";
	if(!entry->hash.empty() && entry->hash != "none") descriptiontxt += _L("Hash: ") + entry->hash + "\n";
	if(!entry->hash.empty()) descriptiontxt += _L("Mod Number: ") + TOSTRING(entry->number) + "\n";
	

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
		imgSize.width  = t->getWidth() * 10;
		imgSize.height = t->getHeight() * 10;
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
	mSelectionDone=false;
	// reset all keys
	INPUTENGINE.resetKeys();
	LoadingWindow::get()->hide();
	// show mouse cursor
	//MyGUI::PointerManager::getInstance().setVisible(true);
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


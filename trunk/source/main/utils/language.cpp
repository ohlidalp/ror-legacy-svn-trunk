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
// created: 12th of January 2009, thomas fischer thomas{AT}thomasfischer{DOT}biz
#ifndef NOLANG

#include "OgreFontManager.h"
#include "language.h"
#include "Settings.h"

#ifdef USE_MYGUI
#include <MyGUI.h>
#include <MyGUI_IFont.h>
#include <MyGUI_FontData.h>
#include <MyGUI_FontManager.h>
#endif // USE_MYGUI

#include "fontTextureHelper.h"

using namespace std;
using namespace Ogre;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

// singleton pattern
LanguageEngine* LanguageEngine::myInstance = 0;

LanguageEngine &LanguageEngine::Instance()
{
	if (myInstance == 0)
		myInstance = new LanguageEngine;
	return *myInstance;
}

LanguageEngine::LanguageEngine() : working(false), myguiConfigFilename("MyGUI_FontsEnglish.xml")
{
}

LanguageEngine::~LanguageEngine()
{
}

Ogre::String LanguageEngine::getMyGUIFontConfigFilename()
{
	// this is a fallback to the default English Resource if the language specific file was not found
	String group = "";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(myguiConfigFilename);
	}catch(...)
	{
	}
	if(group == "")
		return String("MyGUI_FontsEnglish.xml");

	return myguiConfigFilename;
}

void LanguageEngine::setup()
{
#ifdef USE_MOFILEREADER
	// load language, must happen after initializing Settings class and Ogre Root!
	// also it must happen after loading all basic resources!
	reader = new moFileLib::moFileReader();

	String language = SSETTING("Language");
	String language_short = SSETTING("Language Short").substr(0, 2); // only first two characters are important

	// Load a .mo-File.
	LOG("*** Loading Language ***");
	String langfile = SSETTING("Program Path") + String("languages/") + language_short + String("/LC_MESSAGES/ror.mo");
	if (reader->ReadFile(langfile.c_str()) != moFileLib::moFileReader::EC_SUCCESS )
	{
		LOG("* error loading language file " + langfile);
		return;
	}
	working=true;

	// add resource path
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("Program Path") + String("languages/") + language_short + String("/LC_MESSAGES"), "FileSystem", "LanguageRanges");

	// now load the code ranges
	// be aware, that this approach only works if we load just one language, and not multiple
	setupCodeRanges("code_range.txt", "LanguageRanges");
#else
	// init gettext
	bindtextdomain("ror","languages");
	textdomain("ror");

	char *curr_locale = setlocale(LC_ALL,NULL);
	if(curr_locale)
		LOG("system locale is: " + String(curr_locale));
	else
		LOG("unable to read system locale!");

	if(!SSETTING("Language Short").empty())
	{
		LOG("setting new locale to " + SSETTING("Language Short"));
		char *newlocale = setlocale(LC_ALL, SSETTING("Language Short").c_str());
		if(newlocale)
			LOG("new locale is: " + String(newlocale));
		else
			LOG("error setting new locale");
	} else
	{
		LOG("not changing locale, using system locale");
	}

#endif // USE_MOFILEREADER
	LOG("* Language successfully loaded");
}

Ogre::String LanguageEngine::lookUp(Ogre::String name)
{
#ifdef USE_MOFILEREADER
	if(working)
		return reader->Lookup(name.c_str());
	return name;
#else
	return String(gettext(str));
#endif //MOFILEREADER
}

void LanguageEngine::setupCodeRanges(String codeRangesFilename, String codeRangesGroupname)
{
	// not using the default mygui font config
	myguiConfigFilename = "MyGUI_FontConfig.xml";
	DataStreamPtr ds;
	try
	{
		ds = ResourceGroupManager::getSingleton().openResource(codeRangesFilename, codeRangesGroupname);
		if(ds.isNull())
		{
			LOG("unable to load language code points file: " + codeRangesFilename);
			return;
		}

	} catch(...)
	{
		LOG("unable to load language code points file: " + codeRangesFilename);
		return;
	}
	LOG("loading code_range file: " + codeRangesFilename);

	char line[9046] = "";
	while (!ds->eof())
	{
		size_t ll = ds->readLine(line, 9045);
		// only process valid lines
		if(strncmp(line, "code_points ", 12) && strlen(line) > 13)
			continue;
		Ogre::StringVector args = StringUtil::split(line + 12, " ");
		for(Ogre::StringVector::iterator it=args.begin(); it!=args.end(); it++)
		{
			Font::CodePointRange range;
			StringVector itemVec = StringUtil::split(*it, "-");
			if (itemVec.size() == 2)
				range = Font::CodePointRange(StringConverter::parseLong(itemVec[0]), StringConverter::parseLong(itemVec[1]));

			// add this code range to all available fonts
			ResourceManager::ResourceMapIterator itf = FontManager::getSingleton().getResourceIterator();
			while (itf.hasMoreElements())
				((FontPtr)itf.getNext())->addCodePointRange(range);

			// add code points to all MyGUI fonts
#ifdef USE_MYGUI
			// well, we load a reconfigured mygui font config xml file, easier for now
			//MyGUI::IFont *fp = MyGUI::FontManager::getInstance().getByName("Default");
			//if(fp) fp-> addCodePointRange(range.first, range.second);
			//fp = MyGUI::FontManager::getInstance().getByName("DefaultBig");
			//if(fp) fp->addCodePointRange(range.first, range.second);
#endif //USE_MYGUI
		}
	}

	// reload all ttf fonts
	ResourceManager::ResourceMapIterator itf = FontManager::getSingleton().getResourceIterator();
	while (itf.hasMoreElements())
	{
		FontPtr font = itf.getNext();
		// if the font is a ttf font and loaded, then reload it in order to regenenerate the glyphs with corrected code_points
		if(font->getType() == Ogre::FT_TRUETYPE && font->isLoaded())
			font->reload();
	}

	//fontCacheInit("Cyberbit");
	//generateAllFontTextures();
}
#endif //NOLANG

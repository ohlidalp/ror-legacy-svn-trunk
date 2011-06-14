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

#ifdef USE_MOFILEREADER
// windows only

#ifndef NOLANG

#include "OgreFontManager.h"
#include "language.h"
#include "Settings.h"

//#include "MyGUI_Font.h"
//#include "MyGUI_FontManager.h"

#include "fontTextureHelper.h"

using namespace std;
using namespace Ogre;
using namespace moFileLib;

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

LanguageEngine::LanguageEngine() : working(false)
{
}

LanguageEngine::~LanguageEngine()
{
}

void LanguageEngine::setup()
{
	reader = new moFileReader();

	String language = SSETTING("Language");
	String language_short = SSETTING("Language Short").substr(0, 2); // only first two characters are important

	// Load a .mo-File.
	LOG("*** Loading Language ***");
	String langfile = SSETTING("Program Path") + String("languages/") + language_short + String(".mo");
	if (reader->ReadFile(langfile.c_str()) != moFileLib::moFileReader::EC_SUCCESS )
	{
			LOG("* error loading language file " + langfile);
			return;
	}
	working=true;

	// add resource path
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("Program Path") + "languages/" + language_short, "FileSystem", "LanguageRanges");

	// now load the code ranges
	// be aware, that this approach only works if we load just one language, and not multiple
	setupCodeRanges("codes.txt", "LanguageRanges");

	LOG("* Language successfully loaded");
}

Ogre::String LanguageEngine::lookUp(Ogre::String name)
{
	if(working)
		return reader->Lookup(name.c_str());
	return name;
}

void LanguageEngine::setupCodeRanges(String codeRangesFilename, String codeRangesGroupname)
{
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

	char line[1024] = "";
	while (!ds->eof())
	{
		size_t ll = ds->readLine(line, 1023);
		// only process valid lines
		if(strncmp(line, "code_points ", 12) && strnlen(line, 50) > 13)
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
			// XXX: TOFIX: CRASH!
			//MyGUI::Font *fp = (MyGUI::Font *)MyGUI::FontManager::getInstance().getByName("Default");
			//if(fp) fp->addCodePointRange(range.first, range.second);
			//fp = (MyGUI::Font *)MyGUI::FontManager::getInstance().getByName("DefaultBig");
			//if(fp) fp->addCodePointRange(range.first, range.second);
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
#endif //USE_MOFILEREADER


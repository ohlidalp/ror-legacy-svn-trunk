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

#ifndef LANGUAGE_H_
#define LANGUAGE_H_
#include "RoRPrerequisites.h"

// three configurations currently supported:
// #define NOLANG            = no language translations at all, removes any special parsing tags
// #define USE_MOFILEREADER  = windows gettext replacement
// #define !USE_MOFILEREADER = using linux gettext

#ifdef NOLANG
// no language mode
// used when building with wxwidgets for example (as they ship their own i18n)

#define _L(str) Ogre::String(str)

#else // NOLANG

#include "Ogre.h"
#ifdef USE_MOFILEREADER
# include "moFileReader.h"
# define _L(str) LanguageEngine::Instance().lookUp(str).c_str()
#else
// gettext
#include <libintl.h>
#include <locale.h>
# define _L(str) gettext(str)
#endif //MOFILEREADER


#ifdef USE_MOFILEREADER
#define MOFILENAME "ror"

class LanguageEngine
{
public:
    static LanguageEngine & Instance();
    void setup();
	Ogre::String lookUp(Ogre::String name);
    
protected:
	LanguageEngine();
	~LanguageEngine();
	LanguageEngine(const LanguageEngine&);
	LanguageEngine& operator= (const LanguageEngine&);
	static LanguageEngine* myInstance;
	bool working;
	moFileLib::moFileReader *reader;
	void setupCodeRanges(Ogre::String codeRangesFilename, Ogre::String codeRangesGroupname);
};
#endif // MOFILEREADER
#endif //NOLANG
#endif //LANGUAGE_H_

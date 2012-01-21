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
// created by thomas fischer, 4th of January 2009
#ifndef SETTINGS_H
#define SETTINGS_H

#include "RoRPrerequisites.h"

#include "Ogre.h"
#include <map>

// some shortcuts to improve code readability
#define SETTINGS Settings::Instance()
#define SSETTING(x) Settings::Instance().getSetting(x) //<! get string setting
#define UTFSSETTING(x) Settings::Instance().getUTFSetting(x) //<! get UTF string setting
#define BSETTING(x) Settings::Instance().getBooleanSetting(x) //<! get boolean setting
#define ISETTING(x) PARSEINT(Settings::Instance().getSetting(x)) //<! get int setting
#define FSETTING(x) PARSEREAL(Settings::Instance().getSetting(x)) //<! get float setting

class Settings
{
public:
	static Settings & Instance();

	Ogre::String getSetting(Ogre::String key);
	Ogre::UTFString getUTFSetting(Ogre::UTFString key);
	bool getBooleanSetting(Ogre::String key);
	
	Ogre::String getSettingScriptSafe(const Ogre::String &key);
	void setSettingScriptSafe(const Ogre::String &key, const Ogre::String &value);

	void setSetting(Ogre::String key, Ogre::String value);
	void setUTFSetting(Ogre::UTFString key, Ogre::UTFString value);
	
	bool setupPaths();
	void loadSettings(Ogre::String configFile, bool overwrite=false);
	void saveSettings(Ogre::String configFile);
	void saveSettings();

	void checkGUID();
	void createGUID();

#ifdef USE_ANGELSCRIPT
	// we have to add this to be able to use the class as reference inside scripts
	void addRef(){};
	void release(){};
#endif

protected:
	Settings();
	~Settings();
	Settings(const Settings&);
	Settings& operator= (const Settings&);
	static Settings* myInstance;

	// members
	// TODO: use wide chat / UTFString ...
	typedef std::map<Ogre::String, Ogre::String> settings_map_t;
	settings_map_t settings;

	// methods
	void path_descend(char* path);
	void path_add(char* path, const char* dirname);

	bool get_system_paths(char *program_path, char *user_path);
	int generateBinaryHash();

};
#endif

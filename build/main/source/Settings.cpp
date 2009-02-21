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
// created by thomas fischer, 4th of January 2009
#include "Settings.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#include <shlobj.h>
#endif
#include "language.h"

using namespace std;
using namespace Ogre;

// singleton pattern
Settings* Settings::myInstance = 0;

Settings & Settings::Instance () 
{
	if (myInstance == 0)
		myInstance = new Settings;
	return *myInstance;
}

Settings::Settings()
{
}

Settings::~Settings()
{
}

Ogre::String Settings::getSetting(Ogre::String key)
{
	return settings[key];
}

void Settings::setSetting(Ogre::String key, Ogre::String value)
{
	settings[key] = value;
}

void Settings::saveSettings(Ogre::String configFile)
{
	//printf("saving Config to file: %s\n", configFile.c_str());
	FILE *fd = fopen(configFile.c_str(), "w");
	if (!fd)
		return;

	// now save the settings to RoR.cfg
	std::map<std::string, std::string>::iterator it;
	for(it = settings.begin(); it != settings.end(); it++)
	{
		fprintf(fd, "%s=%s\n", it->first.c_str(), it->second.c_str());
	}

	fclose(fd);
}

void Settings::loadSettings(Ogre::String configFile, bool overwrite)
{
	Ogre::ConfigFile cfg;
	cfg.load(configFile, "=:\t", false);

	// load all settings into a map!
	Ogre::ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
	Ogre::String svalue, sname;
	while (i.hasMoreElements())
	{
		sname = i.peekNextKey();
		svalue = i.getNext();
		if(!overwrite && settings[sname] != "") continue;
		settings[sname] = svalue;
		//logfile->AddLine(conv("### ") + conv(sname) + conv(" : ") + conv(svalue));logfile->Write();
	}
}

bool Settings::setupPaths()
{
	char program_path[1024]="";
	char resources_path[1024]="";
	char streams_path[1024]="";
	char user_path[1024]="";
	char config_root[1024]="";
	char dirsep='/';

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	if (!GetModuleFileName(NULL, program_path, 512))
	{
		MessageBox( NULL, _L("Error while retrieving program space path").c_str(), _L("Startup error").c_str(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return false;
	}
	GetShortPathName(program_path, program_path, 512); //this is legal
	path_descend(program_path);

	if (SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, user_path)!=S_OK)
	{
		MessageBox( NULL, _L("Error while retrieving user space path").c_str(), _L("Startup error").c_str(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return false;
	}
	GetShortPathName(user_path, user_path, 512); //this is legal
	sprintf(user_path, "%s\\Rigs of Rods\\", user_path);
//MessageBox( NULL, program_path, "Program root", MB_OK | MB_ICONERROR | MB_TASKMODAL);
//MessageBox( NULL, user_path, "User root", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	//true program path is impossible to get from POSIX functions
	//lets hack!
	pid_t pid = getpid();
	char procpath[256];
	sprintf(procpath, "/proc/%d/exe", pid);
	int ch = readlink(procpath,program_path,240);
	if (ch != -1)
	{
		program_path[ch] = 0;
		path_descend(program_path);
	} else return false;
	//user path is easy
	strncpy(user_path, getenv ("HOME"), 240);
	sprintf(user_path, "%s/RigsOfRods/", user_path);
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	//found this code, will look later
	std::string path = "./";
	ProcessSerialNumber PSN;
	ProcessInfoRec pinfo;
	FSSpec pspec;
	FSRef fsr;
	OSStatus err;
	/* set up process serial number */
	PSN.highLongOfPSN = 0;
	PSN.lowLongOfPSN = kCurrentProcess;
	/* set up info block */
	pinfo.processInfoLength = sizeof(pinfo);
	pinfo.processName = NULL;
	pinfo.processAppSpec = &pspec;
	/* grab the vrefnum and directory */
	err = GetProcessInformation(&PSN, &pinfo);
	if (! err ) {
	char c_path[2048];
	FSSpec fss2;
	int tocopy;
	err = FSMakeFSSpec(pspec.vRefNum, pspec.parID, 0, &fss2);
	if ( ! err ) {
	err = FSpMakeFSRef(&fss2, &fsr);
	if ( ! err ) {
	char c_path[2049];
	err = (OSErr)FSRefMakePath(&fsr, (UInt8*)c_path, 2048);
	if (! err ) {
	path = c_path;
	}
	}
#endif
	//NEXT, derive the resources and stream paths from the base paths (depends on configuration)
	//from now we are mostly platform neutral
	//default mode (released)
	strcpy(resources_path, program_path);
	path_add(resources_path, "resources");
	strcpy(streams_path, program_path);
	path_add(streams_path, "streams");
	if (getSetting("BuildMode") == "Yes")
	{
		//okay, this is totally different!
		strcpy(user_path, program_path); //RoRdev/build/bin/release/windows
		path_descend(user_path); //RoRdev/build/bin/release
		path_descend(user_path); //RoRdev/build/bin
		path_descend(user_path); //RoRdev/build
		//we are in build here, take opportunity for other paths too
		strcpy(resources_path, user_path);
		strcpy(streams_path, user_path);
		path_add(user_path, "userspace");
		path_add(resources_path, "contents");
		path_add(resources_path, "source");
		path_descend(streams_path); //RoRdev
		path_add(streams_path, "streams");
		path_add(streams_path, "release");
	}
	//setup config files names
	char plugins_fname[1024];
	strcpy(plugins_fname, program_path);
	strcat(plugins_fname, "plugins.cfg");
	char ogreconf_fname[1024];
	strcpy(ogreconf_fname, user_path);
	path_add(ogreconf_fname, "config");
	strcpy(config_root, ogreconf_fname); //setting the config root here
	strcat(ogreconf_fname, "ogre.cfg");
	char ogrelog_fname[1024];
	strcpy(ogrelog_fname, user_path);
	path_add(ogrelog_fname, "logs");
	char ogrelog_path[1024];
	strcpy(ogrelog_path, ogrelog_fname);
	strcat(ogrelog_fname, "RoR.log");

	// now update our settings with the results:
	String dsStr = "\\";
#if OGRE_PLATFORM != OGRE_PLATFORM_WIN32
	dsStr="/";
#endif
	settings["dirsep"] = dsStr;
	settings["Config Root"] = String(config_root);
	settings["Cache Path"] = String(user_path) + "cache" + dsStr;
	settings["Log Path"] = String(ogrelog_path);
	settings["Resources Path"] = String(resources_path);
	settings["Streams Path"] = String(streams_path);
	settings["User Path"] = String(user_path);
	settings["Program Path"] = String(program_path);
	settings["plugins.cfg"] = String(plugins_fname);
	settings["ogre.cfg"] = String(ogreconf_fname);
	settings["ogre.log"] = String(ogrelog_fname);

	// now enable the user to override that:
	try
	{
		String configfile = settings["configfile"];
		if(!configfile.size()) configfile = "config.cfg";
		loadSettings(configfile, true);
	} catch(...)
	{
	}

	printf(" * user path:        %s\n", settings["User Path"].c_str());
	printf(" * program path:     %s\n", settings["Program Path"].c_str());
	printf(" * used plugins.cfg: %s\n", settings["plugins.cfg"].c_str());
	printf(" * used ogre.cfg:    %s\n", settings["ogre.cfg"].c_str());
	printf(" * used ogre.log:    %s\n", settings["ogre.log"].c_str());

	return true;
}


void Settings::path_descend(char* path)
{
	char dirsep='/';
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep='\\';
#endif
	char* pt=path+strlen(path)-1;
	if (pt>=path && *pt==dirsep) pt--;
	while (pt>=path && *pt!=dirsep) pt--;
	if (pt>=path) *(pt+1)=0;
}

void Settings::path_add(char* path, char* dirname)
{
	char dirsep='/';
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep='\\';
#endif
	sprintf(path, "%s%s%c", path, dirname, dirsep);
}

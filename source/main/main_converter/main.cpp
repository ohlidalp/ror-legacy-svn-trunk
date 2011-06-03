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
#include "RoRPrerequisites.h"
#include "RigsOfRods.h"
#include "language.h"
#include "errorutils.h"
#include "utils.h"
#include "Settings.h"
#include "rornet.h"
#include "RoRVersion.h"
#include "SerializedRig.h"

using namespace Ogre;

// simpleopt by http://code.jellycan.com/simpleopt/
// license: MIT
#include "SimpleOpt.h"

#define HELPTEXT "--help (this)\n<inputvehicle> <outputfile>"

// option identifiers
enum { OPT_HELP, OPT_INPUT};

// option array
CSimpleOpt::SOption cmdline_options[] = {
	{ OPT_INPUT,          ("-input"),       SO_REQ_SEP },
	{ OPT_HELP,           ("--help"),       SO_NONE    },
	{ OPT_HELP,           ("-help"),        SO_NONE    },
SO_END_OF_OPTIONS
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX // required to stop windows.h messing up std::min
#endif //NOMINMAX
#include "windows.h"
#include "ShellAPI.h"
#endif //OGRE_PLATFORM_WIN32

#ifdef __cplusplus
extern "C" {
#endif

void showUsage()
{
	showInfo(_L("Command Line Arguments"), HELPTEXT);
}

void convert(Ogre::String input, Ogre::String output)
{
	if(!SETTINGS.setupPaths())
		return;

	String logFilename   = SSETTING("Log Path") + "RoRConverter" + Ogre::String(".log");
	String pluginsConfig = "";//SSETTING("plugins.cfg");
	String ogreConfig    = SSETTING("ogre.cfg");

	// no plugins
	Ogre::Root *mRoot = new Root(pluginsConfig, ogreConfig, logFilename);
	// and no init, since no render system or render window
	//mRoot->initialise(false);

	// init some important things
	Ogre::ResourceGroupManager * ogreRGM = new Ogre::ResourceGroupManager();
	Ogre::Math * ogreMath = new Ogre::Math();
	Ogre::MaterialManager * ogreMatMgr = new Ogre::MaterialManager();
	ogreMatMgr->initialise();
	Ogre::HighLevelGpuProgramManager * gpuman = new Ogre::HighLevelGpuProgramManager();
	Ogre::MaterialSerializer * ogreMaterialSerializer = new Ogre::MaterialSerializer();
   
	// get the path info
	Ogre::String basename, ext, path; 
	Ogre::StringUtil::splitFullFilename(input, basename, ext, path);

	// init the paths
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(path, "FileSystem", "our");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("our");

	// then load the truck
	SerializedRig r;
	r.loadTruckVirtual(basename+"."+ext, true);

	// and write the output
	FILE *fo = fopen(output.c_str(), "wb");
	fwrite(&r, sizeof(r), 1, fo);
	fclose(fo);
}

int main(int argc, char *argv[])
{
	CSimpleOpt args(argc, argv, cmdline_options);
	while (args.Next()) {
		if (args.LastError() == SO_SUCCESS)
		{
			if (args.OptionId() == OPT_HELP)
			{
				showUsage();
				return 0;
			}
		} else {
			showUsage();
			return 1;
		}
	}

	if(args.FileCount() != 2)
	{
		showUsage();
		return 1;
	}

	String input = String(args.File(0));
	String output = String(args.File(1));

	convert(input, output);

	return 0;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
	return main(__argc, __argv);
}
#endif


#ifdef __cplusplus
}
#endif

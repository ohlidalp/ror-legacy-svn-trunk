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
#include "ContentManager.h"
#include "JSON.h"

#include <OgreScriptCompiler.h>

using namespace Ogre;

// simpleopt by http://code.jellycan.com/simpleopt/
// license: MIT
#include "SimpleOpt.h"

#define HELPTEXT "--help (this)\n<inputvehicle> <outputfile>"

// option identifiers
enum { OPT_HELP, OPT_LOGPATH};

// option array
CSimpleOpt::SOption cmdline_options[] = {
	{ OPT_LOGPATH,          ("-logpath"),       SO_REQ_SEP },
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

void initOgre()
{
	String logFilename   = SSETTING("Log Path") + "RoRConverter" + Ogre::String(".log");
	String pluginsConfig = SSETTING("plugins.cfg");
	String ogreConfig    = SSETTING("ogre.cfg");

	Ogre::Root *mRoot = new Root(pluginsConfig, ogreConfig, logFilename);

	mRoot->restoreConfig();
	mRoot->initialise(true);

	mRoot->getAutoCreatedWindow()->setAutoUpdated(false);

	ContentManager::getSingleton().init();
}

void convert(Ogre::String input, Ogre::String output)
{
	// get the path info
	Ogre::String in_basename, in_ext, in_path; 
	Ogre::StringUtil::splitFullFilename(input, in_basename, in_ext, in_path);
	Ogre::String out_basename, out_ext, out_path; 
	Ogre::StringUtil::splitFullFilename(output, out_basename, out_ext, out_path);

	// add the paths
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(in_path, "FileSystem", "rorconverter");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("rorconverter");

	// then load the truck
	SerializedRig r;
	r.loadTruckVirtual(in_basename+"."+in_ext, false);

	// then save
	if(out_ext == "json")
	{
		// now save it
		JSONObject root;
		// Adding a string
		root[L"truckname"] = new JSONValue(r.realtruckname.c_str());

		// Create nodes array
		JSONArray nodes_array;
		for (int i = 0; i < r.free_node; i++)
		{
			JSONObject node;
			node[L"id"] = new JSONValue(TOSTRING(i).c_str());
			//node[L"options"] = new JSONValue(TOSTRING(r.nodes[i].).c_str());
			node[L"coord"] = new JSONValue(TOSTRING(r.nodes[i].AbsPosition).c_str());
			nodes_array.push_back(new JSONValue(node));
		}
		root[L"nodes"] = new JSONValue(nodes_array);

		// Create beams array
		JSONArray beams_array;
		for (int i = 0; i < r.free_beam; i++)
		{
			JSONObject beam;
			beam[L"id1"] = new JSONValue(TOSTRING(r.beams[i].p1->id).c_str());
			beam[L"id2"] = new JSONValue(TOSTRING(r.beams[i].p2->id).c_str());
			beams_array.push_back(new JSONValue(beam));
		}
		root[L"beams"] = new JSONValue(beams_array);

		// Create final value
		JSONValue *final_value = new JSONValue(root);

		// Print it
		FILE *fo = fopen(output.c_str(), "w");
		fwprintf(fo, L"%ls", final_value->Stringify().c_str());
		fclose(fo);
		//print_out(value->Stringify().c_str());

		// Clean up
		delete final_value;
	} else if(out_ext == "bin")
	{
		// and write the output
		FILE *fo = fopen(output.c_str(), "wb");
		fwrite(&r, sizeof(r), 1, fo);
		fclose(fo);
	} else
	{
		LOG("unsupported output file format: " + out_ext);
	}
}

int main(int argc, char *argv[])
{
	if(!SETTINGS.setupPaths())
		return 1;

	CSimpleOpt args(argc, argv, cmdline_options);
	while (args.Next()) {
		if (args.LastError() == SO_SUCCESS)
		{
			if (args.OptionId() == OPT_HELP)
			{
				showUsage();
				return 0;
			} else if(args.OptionId() == OPT_LOGPATH)
			{
				SETTINGS.setSetting("Log Path", args.OptionArg());
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

	SETTINGS.setSetting("Advanced Logging", "Yes");

	initOgre();

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

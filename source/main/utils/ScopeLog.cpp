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

#include "ScopeLog.h"
#include "Ogre.h"
#include "Settings.h"

#include <stdio.h> // for remove
#include <time.h>

// created by thomas fischer thomas{AT}thomasfischer{DOT}biz, 18th of Juli 2009

using namespace Ogre;

ScopeLog::ScopeLog(String name) : orgLog(0), logFileName(), name(name), f(0), headerWritten(false)
{
	// get original log file
	orgLog = LogManager::getSingleton().getDefaultLog();

	// determine a filename
	logFileName = SSETTING("Log Path") + SSETTING("dirsep") + "_" + name + ".html";
	
	// create a new log file
	f = fopen(logFileName.c_str(), "w");

	// add self as listener
	orgLog->addListener(this);
}

ScopeLog::~ScopeLog()
{
	// remove self as listener
	orgLog->removeListener(this);

	// destroy our log
	if(f)
	{
		fprintf(f, "</body></html>");
		fclose(f);
	}

	// if the new log file is empty, remove it.
	if(!headerWritten)
	{
		remove(logFileName.c_str());
	}
}


void ScopeLog::messageLogged(const String &message, LogMessageLevel lml, bool maskDebug, const String &logName)
{
	if(!headerWritten)
	{
		time_t t = time(NULL);
		fprintf(f, "<html><header><title>%s</title>\n", name.c_str());
		fprintf(f, "<style type=\"text/css\">\n");
		fprintf(f, ".LogMessageLevel1  { font-size:0.9em;color:DarkGreen; }\n");
		fprintf(f, ".LogMessageLevel2  { font-size:1.0em;color:Black; }\n");
		fprintf(f, ".LogMessageLevel3  { font-size:1.3em;color:DarkRed; }\n");
		fprintf(f, ".Warning           { font-size:1.1em;color:Orange; }\n");
		fprintf(f, ".WarningMeshFormat { font-size:0.8em;color:GoldenRod; }\n");
		fprintf(f, ".OgreNotice        { font-size:0.8em;color:OliveDrab; }\n");
		fprintf(f, ".RoRNotice         { font-size:0.8em;color:SeaGreen; }\n");
		fprintf(f, ".CompilerError     { font-size:1.2em;color:Red; }\n");
		fprintf(f, ".MaterialError     { font-size:1.5em;color:Red; }\n");
		fprintf(f, ".GeneralError      { font-size:1.5em;color:Red; }\n");
		fprintf(f, ".IgnoreThis        { font-size:0.8em;color:DarkGrey; }\n");
		fprintf(f, ".BeamInputOutput   { font-size:1.2em;color:OrangeRed; }\n");
		 
		fprintf(f, "</style>\n");
		fprintf(f, "</header><body>\n");
		fprintf(f, "Log for <b>%s</b> created on <b>%s</b> with <b>Rigs of Rods %s</b> ", name.c_str(), ctime(&t), ROR_VERSION_STRING);
		fprintf(f, "(built at %s on %s )<br/>\n", __DATE__, __TIME__);

		fprintf(f, "<table border='1'><tr><td>ms since start</td><td>message</td></tr>\n");
		headerWritten = true;
	}

	// use style depending on log message level
	char style[50]="";
	sprintf(style, "LogMessageLevel%d", lml);

	// reminder: this if switch is highly sorted
	if(message.find("you should upgrade it as soon as possible using the OgreMeshUpgrade tool") != String::npos)
		sprintf(style, "WarningMeshFormat");
	else if(message.find("WARNING:") != String::npos)
		sprintf(style, "Warning");
	else if(message.find("Can't assign material ") != String::npos)
		sprintf(style, "MaterialError");
	else if(message.find("Compiler error: ") != String::npos)
		sprintf(style, "CompilerError");
	else if(message.find("Invalid WAV file: ") != String::npos)
		sprintf(style, "GeneralError");
	else if(message.find("error while loading terrain: ") != String::npos)
		sprintf(style, "GeneralError");
	else if(message.find("Error loading texture ") != String::npos)
		sprintf(style, "GeneralError");
	else if(message.find("ODEF: ") != String::npos)
		sprintf(style, "BeamInputOutput");
	else if(message.find("BIO|") != String::npos)
		sprintf(style, "BeamInputOutput");
	else if(message.find("Inertia|") != String::npos)
		sprintf(style, "BeamInputOutput");
	else if(message.find("Mesh: Loading ") != String::npos)
		sprintf(style, "OgreNotice");
	else if(message.find("Loading 2D Texture") != String::npos)
		sprintf(style, "OgreNotice");
	else if(message.find("Loading 2D Texture") != String::npos)
		sprintf(style, "OgreNotice");
	else if(message.find("Texture: ") != String::npos)
		sprintf(style, "OgreNotice");
	else if(message.find("Caelum: ") != String::npos)
		sprintf(style, "OgreNotice");
	else if(message.find("Info: Freetype returned ") != String::npos)
		sprintf(style, "IgnoreThis");
	else if(message.find("static map icon not found: ") != String::npos)
		sprintf(style, "IgnoreThis");
	else if(message.find("COLL: ") != String::npos)
		sprintf(style, "RoRNotice");
	else if(message.find("Loading WAV file ") != String::npos)
		sprintf(style, "RoRNotice");
	else if(message.find("SoundScriptInstance: instance created: ") != String::npos)
		sprintf(style, "RoRNotice");
	else if(message.find("FLEXBODY ") != String::npos)
		sprintf(style, "RoRNotice");
	else if(message.find("MaterialFunctionMapper: replaced mesh material ") != String::npos)
		sprintf(style, "RoRNotice");
	else if(message.find("MaterialFunctionMapper: replaced entity material ") != String::npos)
		sprintf(style, "RoRNotice");

	
	unsigned long ms = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
	fprintf(f, "<tr class=\"%s\"><td>%d</td><td>%s</td></tr>\n", style, ms, message.c_str());
}

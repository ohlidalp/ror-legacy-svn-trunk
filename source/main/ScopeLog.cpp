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

#include "ScopeLog.h"
#include "Ogre.h"
#include "Settings.h"

#include <stdio.h> // for remove

// created by thomas fischer thomas{AT}thomasfischer{DOT}biz, 18th of Juli 2009

using namespace Ogre;

ScopeLog::ScopeLog(String name) : orgLog(0), logFileName()
{
	// get original log file
	orgLog = LogManager::getSingleton().getDefaultLog();

	// determine a filename
	logFileName = SETTINGS.getSetting("Log Path") + SETTINGS.getSetting("dirsep") + "_" + name + ".log";
	
	// create a new log file
	ourLog = LogManager::getSingleton().createLog(logFileName); // do not replace the default
	ourLog->setLogDetail(orgLog->getLogDetail());

	// add self as listener
	orgLog->addListener(this);
}

ScopeLog::~ScopeLog()
{
	// remove self as listener
	orgLog->removeListener(this);

	// destroy our log
	LogManager::getSingleton().destroyLog(ourLog);

	// if the new log file is empty, remove it.
	if(getFileSize(logFileName) == 0)
	{
		remove(logFileName.c_str());
	}
}

void ScopeLog::messageLogged(const String &message, LogMessageLevel lml, bool maskDebug, const String &logName)
{
	// passtrough to our log
	ourLog->logMessage(message, lml, maskDebug);
}

int ScopeLog::getFileSize(String filename)
{
	FILE *f = fopen(filename.c_str(), "rb");
	if(f != NULL)
	{
		fseek(f, 0, SEEK_END);
		int size = ftell(f);
		fclose(f);
		return size;
	}
	return 0;
}
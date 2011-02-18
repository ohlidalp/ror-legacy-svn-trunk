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
// created on 21th of May 2008 by Thomas

#include "CacheSystem.h"
#include "sha1.h"
#include "ImprovedConfigFile.h"
#include "Settings.h"
#include "language.h"
#include "errorutils.h"

#include "SoundScriptManager.h"
#include "BeamData.h" // for authorinfo_t
#include "ScopeLog.h"

#ifdef USE_MYGUI
#include "LoadingWindow.h"
#endif //MYGUI

using namespace RoR; //CSHA1

// singleton pattern
CacheSystem* CacheSystem::myInstance = 0;

CacheSystem &CacheSystem::Instance ()
{
	return *myInstance;
}

CacheSystem::CacheSystem(SceneManager *smgr) : smgr(smgr)
{
}

CacheSystem::~CacheSystem()
{
}

void CacheSystem::startup() 
{
	LogManager::getSingleton().logMessage("CacheSystem starting");
}

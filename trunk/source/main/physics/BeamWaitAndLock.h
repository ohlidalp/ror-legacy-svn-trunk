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
#ifndef __BeamLocker_H__
#define __BeamLocker_H__

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"

#include "BeamFactory.h"

// helper macro that logs the locking place

#ifdef FEAT_DEBUG_MUTEX
# define BEAMLOCK() BeamWaitAndLock sync(__FILE__, __FUNCTION__, __LINE__)
#else //!FEAT_DEBUG_MUTEX
// TODO: remove this later on
# define BEAMLOCK() BeamWaitAndLock sync(__FILE__, __FUNCTION__, __LINE__)
//# define BEAMLOCK()
#endif //FEAT_DEBUG_MUTEX


/**
 * @brief helper class that locks the vehicles array during its lifetime
 */
class BeamWaitAndLock
{
public:
	/**
	 *	constructor
	 */
	BeamWaitAndLock()
	{
		BeamFactory::getSingleton()._waitForSyncAndLock();
	}

	/**
	 *	constructor with logging
	 */
	BeamWaitAndLock(const char *fileName, const char *functionName, int line)
	{
		LOG("Beam data locking | " + String(functionName) + " | " + String(fileName) + ":" + TOSTRING(line));
		BeamFactory::getSingleton()._waitForSyncAndLock();
	}

	/**
	 * destructor
	 */
	~BeamWaitAndLock()
	{
		BeamFactory::getSingleton()._ReleaseLock();
	}
};

#endif //__BeamLocker_H__

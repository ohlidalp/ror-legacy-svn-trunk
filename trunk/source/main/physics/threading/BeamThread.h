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
#ifndef __BeamThread_H__
#define __BeamThread_H__

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"

#include "BeamWorkerManager.h"
#include "BeamFactory.h"
#include "Singleton.h"
#include "utils.h"

// this class should be derived by every thread that wants to access the Beam array
// it will automatically register with the thread manager upon initialization

// IMPORTANT: before the actual access to the array, the thread MUST call syncBeamThreads
// this function is able to pause the current thread to modify the array.
class BeamThread
{
public:
	BeamThread();
	~BeamThread();

	inline void syncBeamThreads()
	{
		BeamWorkerManager::getSingleton().syncThreads(this);
	}
};

#endif // __BeamThread_H__

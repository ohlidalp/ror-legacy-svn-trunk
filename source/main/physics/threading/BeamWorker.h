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

#ifndef __BEAMWORKER_H_
#define __BEAMWORKER_H_

#include "RoRPrerequisites.h"
#include "Beam.h"
#include "BeamWorkerManager.h"
#include "BeamThread.h"

// this class wraps a thread that acts as worker thread.
// it will request work and sync with the others
class BeamWorker :
	  public BeamThread
{
protected:
	bool running;

	float test1;
public:
	BeamWorker();
	~BeamWorker();

	static void createThread();
	// called from the new thread, do not execute manually
	void _startWorkerLoop();
	void killWorker() { running=false; }
protected:
	// method to work off one beam
	void _doWork();
};

#endif // __BEAMWORKER_H_

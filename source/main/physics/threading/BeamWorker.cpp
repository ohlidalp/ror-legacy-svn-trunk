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

#include "BeamWorker.h"

#include <Ogre.h>
#include "Settings.h"

#include "BeamThread.h"
#include "errorutils.h"
#include "InputEngine.h"
#include "utils.h"

using namespace Ogre;



void *threadWorkerEntry(void *beamWorkerPtr)
{
	// call worker method in class
	BeamWorker *worker = (BeamWorker*)beamWorkerPtr;
	worker->_startWorkerLoop();
	pthread_exit(NULL);
	return NULL;	
}

BeamWorker::BeamWorker()
{
	pthread_create(&thread, NULL, threadWorkerEntry, (void*)this);
}

BeamWorker::~BeamWorker()
{
	// TODO: make sure the thread is gone!
}

void BeamWorker::_startWorkerLoop()
{
#ifdef USE_CRASHRPT
	if(BSETTING("NoCrashRpt", true))
	{
		// add the crash handler for this thread
		CrThreadAutoInstallHelper cr_thread_install_helper;
		MYASSERT(cr_thread_install_helper.m_nInstallStatus==0);
	}
#endif //USE_CRASHRPT

	LOG("TR| BeamWorker created: " + TOSTRING(ThreadID::getID()));
	try
	{
		// additional exception handler required, otherwise RoR just crashes upon exception
		while (1)
		{
			// sync threads
			syncBeamThreads();

			//do work
			_doWork();
		}
	} catch(Ogre::Exception& e)
	{
		LOG("TR| BeamWorker exception: " + TOSTRING(ThreadID::getID()) + ": " + e.getFullDescription());
		// TODO: error handling
	}
	LOG("TR| BeamWorker exiting: " + TOSTRING(ThreadID::getID()));
}

void BeamWorker::_doWork()
{
	int seconds = Ogre::Math::RangeRandom(1, 10);
	LOG("TR| BeamWorker doing work: " + TOSTRING(ThreadID::getID()) + " sleeping " + TOSTRING(seconds) + " seconds");

	sleep(seconds);

#if 0
	float dtperstep=dt/(Real)steps;

	for (int i=0; i<steps; i++)
	{
		for (int t=0; t<numtrucks; t++)
		{
			if(!trucks[t]) continue;

			if (trucks[t]->state!=SLEEPING && trucks[t]->state!=NETWORKED && trucks[t]->state!=RECYCLE)
				trucks[t]->calcForcesEuler(i==0, dtperstep, i, steps);
		}
		truckTruckCollisions(dtperstep);
	}

	for (int t=0; t<numtrucks; t++)
	{
		if(!trucks[t]) continue;

		if (trucks[t]->reset_requested)
			trucks[t]->SyncReset();

		if (trucks[t]->state!=SLEEPING && trucks[t]->state!=NETWORKED && trucks[t]->state!=RECYCLE)
		{
			trucks[t]->lastlastposition=trucks[t]->lastposition;
			trucks[t]->lastposition=trucks[t]->position;
			trucks[t]->updateTruckPosition();
		}
		if (floating_origin_enable && trucks[t]->nodes[0].RelPosition.length()>100.0)
			trucks[t]->moveOrigin(trucks[t]->nodes[0].RelPosition);
	}

	ffforce=affforce/steps;
	ffhydro=affhydro/steps;
	if (free_hydro) ffhydro=ffhydro/free_hydro;
#endif // 0
}

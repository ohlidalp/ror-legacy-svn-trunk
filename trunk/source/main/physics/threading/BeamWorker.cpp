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
	// create new beam worker inside the new thread, not outside!
	BeamWorker *worker = new BeamWorker();
	worker->_startWorkerLoop();
	delete(worker);
	pthread_exit(NULL);
	return NULL;	
}

BeamWorker::BeamWorker() : running(true), test1(0)
{
}

void BeamWorker::createThread()
{
	pthread_t thread;
	int res = pthread_create(&thread, NULL, threadWorkerEntry, NULL);
	if(res)
	{
		perror("error creating new thread: ");
	}
	pthread_detach(thread);
}

BeamWorker::~BeamWorker()
{
	running = false;
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


	//LOG("TR| BeamWorker created: " + getThreadIDAsString());
	try
	{
		// additional exception handler required, otherwise RoR just crashes upon exception
		while (running)
		{
			// sync threads
			syncBeamThreads();

			//do work
			_doWork();
		}
	} catch(Ogre::Exception& e)
	{
		LOG("TR| BeamWorker exception: " + getThreadIDAsString() + ": " + e.getFullDescription());
		// TODO: error handling
	}
	//LOG("TR| BeamWorker exiting: " + getThreadIDAsString());
}

void BeamWorker::_doWork()
{
	float seconds = Ogre::Math::RangeRandom(0.1f, 2.0f);
	//LOG("TR| BeamWorker doing work: " + getThreadIDAsString() + " sleeping " + TOSTRING(seconds) + " seconds");

	test1 += seconds;

	if(test1 > 20)
		killWorker();

	sleepMilliSeconds(seconds);

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

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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 24th of August 2009

#include "BeamWorkerManager.h"
#include "BeamWorker.h"

#include <Ogre.h>
#include "Settings.h"


using namespace Ogre;

void *threadWorkerManagerEntry(void* ptr)
{
	BeamWorkerManager *btlm = (BeamWorkerManager*)ptr;
	btlm->_startWorkerLoop();
	pthread_exit(NULL);
	return NULL;	
}

BeamWorkerManager::BeamWorkerManager() :
	      threads()
		, threadsSize(0)
		, done_count(0)
		, work_mutex()
		, work_cv()
		, done_count_mutex()
		, done_count_cv()

{
	pthread_mutex_init(&work_mutex, NULL);
	pthread_cond_init(&work_cv, NULL);
	pthread_mutex_init(&done_count_mutex, NULL);
	pthread_cond_init(&done_count_cv, NULL);
	// start worker thread:
	pthread_create(&workerThread, NULL, threadWorkerManagerEntry, (void*)this);
}


BeamWorkerManager::~BeamWorkerManager()
{
}

void BeamWorkerManager::_startWorkerLoop()
{
	while (1)
	{
		MUTEX_LOCK(&done_count_mutex);
		while (done_count < threadsSize)
		{
			LOG("worker loop continued, waiting for more threads | threads waiting: " + TOSTRING(done_count) + " / " + TOSTRING(threadsSize));
			pthread_cond_wait(&done_count_cv, &done_count_mutex);
		}
		MUTEX_UNLOCK(&done_count_mutex);
		LOG("worker loop lock aquired, all threads sleeping...");

		// we got through, all threads should be stopped by now

		// TODO: make modifications on truck array in here
		Sleep(5000);

		// reset counter, continue all threads via signal
		LOG("worker loop: unpausing all threads");
		MUTEX_LOCK(&done_count_mutex);
		done_count=0;
		// send signals to the threads
		pthread_cond_signal(&work_cv);
		MUTEX_UNLOCK(&done_count_mutex);
	}
}

void BeamWorkerManager::addThread(BeamThread *bthread)
{
	threads.push_back(bthread);
	threadsSize=threads.size();
}

void BeamWorkerManager::removeThread(BeamThread *bthread)
{
	for(std::vector<BeamThread*>::iterator it = threads.begin(); it != threads.end(); ++it)
	{
		if(*it == bthread)
		{
			threads.erase(it);
			threadsSize=threads.size();
			return;
		}
	}
}

void BeamWorkerManager::syncThreads(BeamThread *bthread)
{
	LOG("Thread visiting syncing point| class: " + TOSTRING((unsigned long)bthread) + " / thread: " + TOSTRING((unsigned long)pthread_self().p));
	
	MUTEX_LOCK(&work_mutex);

	// count thread counter up:
	MUTEX_LOCK(&done_count_mutex);
	done_count++;
	pthread_cond_signal(&done_count_cv);
	MUTEX_UNLOCK(&done_count_mutex);

	// then wait for the signal that we can continue
	pthread_cond_wait(&work_cv, &work_mutex);
	MUTEX_UNLOCK(&work_mutex);
	// return to continue to do work
}

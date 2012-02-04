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
#include "utils.h"


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
		, done_count_mutex()
		, done_count_cv()

{
	pthread_mutex_init(&api_mutex, NULL);
	pthread_mutex_init(&done_count_mutex, NULL);
	pthread_cond_init(&done_count_cv, NULL);

	threads.clear();

	// start worker thread:
	pthread_create(&workerThread, NULL, threadWorkerManagerEntry, (void*)this);
}


BeamWorkerManager::~BeamWorkerManager()
{
	pthread_cond_destroy(&done_count_cv);
	pthread_mutex_destroy(&api_mutex);
	pthread_mutex_destroy(&done_count_mutex);
}

void BeamWorkerManager::_startWorkerLoop()
{
	LOG("worker manager started in thread " + TOSTRING(ThreadID::getID()));
	while (1)
	{
		MUTEX_LOCK(&done_count_mutex);
		while (done_count < threadsSize)
		{
			LOG("TR| worker loop continued, waiting for more threads | threads waiting: " + TOSTRING(done_count) + " / " + TOSTRING(threadsSize));
			pthread_cond_wait(&done_count_cv, &done_count_mutex);
		}
		MUTEX_UNLOCK(&done_count_mutex);
		LOG("TR| worker loop lock acquired, all threads sleeping...");

		// we got through, all threads should be stopped by now

		// TODO: make modifications on truck array in here
		sleep(3);

		// reset counter, continue all threads via signal
		LOG("TR| worker loop: unpausing all threads");
		MUTEX_LOCK(&done_count_mutex);
		done_count=0;
		// send signals to the threads
		threadMap::iterator it;
		for(it = threads.begin(); it != threads.end(); ++it)
		{
			pthread_cond_signal(&it->second.work_cv);
		}
		MUTEX_UNLOCK(&done_count_mutex);
	}
}

void BeamWorkerManager::addThread(BeamThread *bthread)
{
	// threadID is not valid in this context!
	MUTEX_LOCK(&api_mutex);
	workerData_t wd;
	pthread_mutex_init(&wd.work_mutex, NULL);
	pthread_cond_init(&wd.work_cv, NULL);
	wd.bthread = bthread;
	wd.threadID = -1;
	LOG("TR| add Thread: " + TOSTRING(bthread));
	threads[bthread] = wd;
	threadsSize++;
	MUTEX_UNLOCK(&api_mutex);
}

void BeamWorkerManager::removeThread(BeamThread *bthread)
{
	// threadID is not valid in this context!
	MUTEX_LOCK(&api_mutex);
	pthread_cond_destroy(&threads[bthread].work_cv);
	pthread_mutex_destroy(&threads[bthread].work_mutex);
	LOG("TR| remove Thread: " + TOSTRING(bthread));
	threads[bthread].bthread = NULL;
	threads[bthread].threadID = 0;
	threadsSize--;
	MUTEX_UNLOCK(&api_mutex);
}

void BeamWorkerManager::syncThreads(BeamThread *bthread)
{
	unsigned int tid = ThreadID::getID();
	LOG("TR| Thread visiting syncing point| class: " + TOSTRING((unsigned long)bthread) + " / thread: " + TOSTRING(tid));
	threadMap::iterator it = threads.find(bthread);
	if(it == threads.end())
	{
		LOG("unknown thread calling: " + TOSTRING(tid) + " / class " + TOSTRING(bthread));
		return;
	}
	
	workerData_t &wd = it->second;
	MUTEX_LOCK(&wd.work_mutex);

	// count thread counter up:
	MUTEX_LOCK(&done_count_mutex);
	done_count++;
	pthread_cond_signal(&done_count_cv);
	MUTEX_UNLOCK(&done_count_mutex);


	// then wait for the signal that we can continue
	pthread_cond_wait(&wd.work_cv, &wd.work_mutex);
	MUTEX_UNLOCK(&wd.work_mutex);
	// return to continue to do work
}

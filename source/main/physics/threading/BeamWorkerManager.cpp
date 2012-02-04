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
#include <unistd.h>
#include <time.h>

using namespace Ogre;

void *threadWorkerManagerEntry(void* ptr)
{
	// create instance inside the new thread, not outside
	BeamWorkerManager::getSingleton()._startWorkerLoop();
	// delete singleton maybe?
	pthread_exit(NULL);
	return NULL;	
}

BeamWorkerManager::BeamWorkerManager() :
	      threads()
		, threadsSize(0)
		, targetThreadSize(4096)
		, done_count(0)
		, done_count_mutex()
		, done_count_cv()

{
	pthread_mutex_init(&api_mutex, NULL);
	pthread_mutex_init(&done_count_mutex, NULL);
	pthread_cond_init(&done_count_cv, NULL);

	threads.clear();
}

BeamWorkerManager::~BeamWorkerManager()
{
	pthread_mutex_destroy(&api_mutex);
	pthread_mutex_destroy(&done_count_mutex);
	pthread_cond_destroy(&done_count_cv);
}

void BeamWorkerManager::createThread()
{
	// start worker thread:
	pthread_t workerThread;
	int res = pthread_create(&workerThread, NULL, threadWorkerManagerEntry, NULL);
	if(res)
	{
		perror("error starting thread: ");
	}
	pthread_detach(workerThread);
}

void BeamWorkerManager::_startWorkerLoop()
{
	LOG("worker manager started in thread " + getThreadIDAsString());
	int rc = 0;
	struct timespec timeout;
	int local_done_count = -1;
	while (1)
	{
		MUTEX_LOCK(&done_count_mutex);
		while (done_count < threadsSize)
		{
			//LOG("TR| worker loop continued, waiting for more threads | threads waiting: " + TOSTRING(done_count) + " / " + TOSTRING(threadsSize));

			// set timeout
#if 1
			pthread_cond_wait(&done_count_cv, &done_count_mutex);
#else
			timeout.tv_sec = time(NULL);
			timeout.tv_nsec = 50000;

			// then wait for signal or time
			rc = pthread_cond_timedwait(&done_count_cv, &done_count_mutex, &timeout);
#endif //0
		}
		LOG("TR| worker loop done | threads waiting: " + TOSTRING(done_count) + " / " + TOSTRING(threadsSize));
		MUTEX_UNLOCK(&done_count_mutex);
		LOG("TR| worker loop lock acquired, all threads sleeping...");

		// we got through, all threads should be stopped by now

		// create new threads?
		_checkRunThreads();

		// TODO: make modifications on truck array in here
		//	sleepMilliSeconds(100);

		// reset counter, continue all threads via signal
		LOG("TR| worker loop: un-pausing all threads");
		MUTEX_LOCK(&done_count_mutex);
		done_count=0;
		// send signals to the threads
		MUTEX_LOCK(&api_mutex);
		threadMap::iterator it;
		for(it = threads.begin(); it != threads.end(); ++it)
		{
			pthread_cond_signal(&it->second.work_cv);
		}
		MUTEX_UNLOCK(&api_mutex);

		MUTEX_UNLOCK(&done_count_mutex);
	}
}

void BeamWorkerManager::_checkRunThreads()
{
	int c = 0;
	for(int i=threadsSize; i < targetThreadSize; i++)
	{
		BeamWorker::createThread();
		c++;
	}
	if(c>0)
		LOG("TR| "+ TOSTRING(c) + " threads died, restarted them");
}

void BeamWorkerManager::addThread(BeamThread *bthread)
{
	MUTEX_LOCK(&api_mutex);
	unsigned long tid = getThreadID();
	workerData_t wd;
	pthread_mutex_init(&wd.work_mutex, NULL);
	pthread_cond_init(&wd.work_cv, NULL);
	wd.bthread = bthread;
	wd.threadID = tid;
	//LOG("TR| add Thread: " + TOSTRING((unsigned long)bthread) + " / thread " + TOSTRING(tid));
	// using insert instead of [] -> speed
	threads.insert(std::pair<BeamThread*, workerData_t>(bthread, wd));
	threadsSize = threads.size();
	MUTEX_UNLOCK(&api_mutex);
}

void BeamWorkerManager::removeThread(BeamThread *bthread)
{
	MUTEX_LOCK(&api_mutex);
	unsigned long tid = getThreadID();
	threadMap::iterator it = threads.find(bthread);
	if(it == threads.end())
	{
		LOG("TR| unknown thread calling remove: class " + TOSTRING((unsigned long)bthread) + " / thread " + TOSTRING(tid));
		return;
	}
	//LOG("TR| remove Thread: " + TOSTRING((unsigned long)bthread) + " / thread " + TOSTRING(tid));
	workerData_t &wd = it->second;
	pthread_cond_destroy(&wd.work_cv);
	pthread_mutex_destroy(&wd.work_mutex);
	threads.erase(it);
	threadsSize = threads.size();
	MUTEX_UNLOCK(&api_mutex);

	// trigger main thread so it wont wait for eternity for the now missing thread
	MUTEX_LOCK(&done_count_mutex);
	pthread_cond_signal(&done_count_cv);
	MUTEX_UNLOCK(&done_count_mutex);
}

void BeamWorkerManager::syncThreads(BeamThread *bthread)
{
	unsigned long tid = getThreadID();
	//LOG("TR| Thread visiting syncing point| class: " + TOSTRING((unsigned long)bthread) + " / thread: " + TOSTRING(tid));

	// yes, we need this lock here
	MUTEX_LOCK(&api_mutex);
	threadMap::iterator it = threads.find(bthread);
	MUTEX_UNLOCK(&api_mutex);
	// result?
	if(it == threads.end())
	{
		LOG("TR| unknown thread calling: " + TOSTRING(tid) + " / class " + TOSTRING((unsigned long)bthread));
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

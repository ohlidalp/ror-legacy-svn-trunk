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
#ifndef __BeamWorkerManager_H__
#define __BeamWorkerManager_H__

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"

#include "BeamFactory.h"
#include "Singleton.h"
#include "utils.h"

#ifdef USE_SKIA
#include <CanvasTexture.h>
#endif // USE_SKIA

class BeamThread;

// Manager that manages all threads and the beam locking
class BeamWorkerManager :
	    public RoRSingleton<BeamWorkerManager>
	  , Ogre::FrameListener
{
	friend class BeamThread;
	friend class RoRSingleton<BeamWorkerManager>;
protected:
	typedef struct workerData_t
	{
		pthread_mutex_t work_mutex;
		pthread_cond_t work_cv;
		unsigned long threadID;
		BeamThread *bthread;
	} workerData_t;

	typedef std::map <BeamThread*, workerData_t> threadMap;
	threadMap threads;
	unsigned long threadsSize;
	unsigned long targetThreadSize;

	unsigned long done_count;
	pthread_mutex_t api_mutex;
	pthread_mutex_t done_count_mutex;
	pthread_cond_t done_count_cv;

	BeamWorkerManager();
	~BeamWorkerManager();


	void addThread(BeamThread *bthread);
	void removeThread(BeamThread *bthread);
	void syncThreads(BeamThread *bthread);
	void initDebugging();
	void updateDebugging(float dt);

#ifdef USE_SKIA
	Ogre::Canvas::Texture *mCanvasTextureClock1;
	float rot;
#endif // USE_SKIA

	bool frameStarted(const FrameEvent& evt);
public:
	static void createThread();
	void _startWorkerLoop();
	void _checkRunThreads();
};

#endif //__BeamWorkerManager_H__

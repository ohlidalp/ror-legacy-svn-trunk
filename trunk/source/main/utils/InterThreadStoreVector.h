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
#ifndef INTERTHREADSTORE_H__
#define INTERTHREADSTORE_H__

#include "RoRPrerequisites.h"
#include <pthread.h>


/// this class is a helper to exchange data in a class between different threads, it can be pushed and pulled in various threads
template <class T>
class InterThreadStoreVector
{
public:
	InterThreadStoreVector()
	{
		pthread_mutex_init(&lock, NULL);
	}

	~InterThreadStoreVector()
	{
		pthread_mutex_destroy(&lock);
	}
	
	void push(T &v)
	{
		pthread_mutex_lock(&lock);
		store.push_back(v);
		pthread_mutex_unlock(&lock);	
	}
	
	int pull(std::vector < T > &res)
	{
		int results = 0;
		std::vector<msg_t> tmpWaitingMessages;
		pthread_mutex_lock(&lock);
		res = store;
		results = res.size();
		store.clear();
		pthread_mutex_unlock(&lock);
		return results;
	}

protected:
	pthread_mutex_t lock;
	std::vector < T > store;
};

#endif //INTERTHREADSTORE_H__
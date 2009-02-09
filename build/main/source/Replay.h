/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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
#ifndef __Replay_H__
#define __Replay_H__
#include "Ogre.h"
using namespace Ogre;

//typedef struct _replaynodes
//{
//} replaynodes_t;

class Replay
{
public:
	Vector3 *nodes;
	float *times;
	int writeindex;
	int numnodes;
	int numframes;
	int firstrun;

	Replay(int nnodes, int nframes);

	//dirty stuff, we use this to write the replay buffer
	Vector3 *getUpdateIndex(float dt);

	//we take negative offsets only
	Vector3 *getReplayIndex(int offset);

	~Replay();

};
#endif

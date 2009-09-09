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

#include "OgrePrerequisites.h"
#include "Beam.h"

class ExampleFrameListener;

typedef struct node_simple_ {
	Ogre::Vector3 pos;
} node_simple_t;

typedef struct beam_simple_ {
	float scale;
	bool broken;
} beam_simple_t;

class Replay
{
public:
	Replay(Beam *b, int nframes);
	~Replay();

	void *getWriteBuffer(float dt, int type);
	void *getReadBuffer(int offset, int type);
	float getReplayTime(int offset);
	void writeDone();

protected:
	Ogre::Timer *replayTimer;
	int numNodes;
	int numBeams;
	int numFrames;

	int writeIndex;
	int firstRun;
	// malloc'ed
	node_simple_t *nodes;
	beam_simple_t *beams;
	float *times;
};
#endif

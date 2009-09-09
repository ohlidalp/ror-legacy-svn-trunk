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
#include "Replay.h"

#include "Ogre.h"

using namespace Ogre;

Replay::Replay(Beam *b, int _numFrames)
{
	numNodes = b->getNodeCount();
	numBeams = b->getBeamCount();
	numFrames = _numFrames;

	replayTimer = new Timer();

	// get memory
	nodes = (node_simple_t*)calloc(numNodes * numFrames, sizeof(node_simple_t));
	beams = (beam_simple_t*)calloc(numBeams * numFrames, sizeof(beam_simple_t));	
	times = (float*)calloc(numFrames, sizeof(float));

	unsigned long bsize = (numNodes * numFrames * sizeof(node_simple_t) + numBeams * numFrames * sizeof(beam_simple_t) + numFrames * sizeof(float)) / 1024.0f;
	LogManager::getSingleton().logMessage("replay buffer: " + StringConverter::toString(bsize) + " kB");

	writeIndex = 0;
	firstRun = 1;
}

//dirty stuff, we use this to write the replay buffer
void *Replay::getWriteBuffer(float dt, int type)
{
	void *ptr = 0;
	times[writeIndex] = dt;
	if(type == 0)
	{
		// nodes
		ptr = (void *)(nodes + (writeIndex * numNodes));
	}else if(type == 1)
	{
		// beams
		ptr = (void *)(beams + (writeIndex * numBeams));
	}
	return ptr;
}

void Replay::writeDone()
{
	writeIndex++;
	if(writeIndex == numFrames)
	{
		firstRun = 0;
		writeIndex = 0;
	}
}

//we take negative offsets only
void *Replay::getReadBuffer(int offset, int type)
{
	if (offset >= 0) offset=-1;
	if (offset <= -numFrames) offset = -numFrames + 1;
	//LogManager::getSingleton().logMessage("replay time: " + StringConverter::toString(times[offset]));
	
	int delta = writeIndex + offset;
	if (delta < 0)
		if (firstRun && type == 0)
			return (void *)(nodes);
		else if (firstRun && type == 1)
			return (void *)(beams);
		else
			delta += numFrames;
	if(type == 0)
		return (void *)(nodes + delta * numNodes);
	else if(type == 1)
		return (void *)(beams + delta * numBeams);
	return 0;
}

float  Replay::getReplayTime(int offset)
{
	if (offset >= 0) offset=-1;
	if (offset <= -numFrames) offset = -numFrames + 1;
	return times[offset];
}

Replay::~Replay()
{
	free(nodes);
	free(times);
	delete replayTimer;
}


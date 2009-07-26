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

Replay::Replay(int nnodes, int nframes)
{
	nodes=(Vector3*)malloc(nnodes*nframes*sizeof(Vector3));
	times=(float*)malloc(nframes*sizeof(float));
	writeindex=0;
	numnodes=nnodes;
	numframes=nframes;
	firstrun=1;
}

//dirty stuff, we use this to write the replay buffer
Vector3 *Replay::getUpdateIndex(float dt)
{
	times[writeindex]=dt;
	Vector3* index=nodes+(writeindex*numnodes);
	writeindex++;
	if (writeindex==numframes) {firstrun=0;writeindex=0;};
	return index;
}

//we take negative offsets only
Vector3 *Replay::getReplayIndex(int offset)
{
	if (offset>=0) offset=-1;
	if (offset<=-numframes) offset=-numframes+1;
	int delta=writeindex+offset;
	if (delta<0) 
		if (firstrun) return nodes;
		else delta+=numframes;
	return nodes+(delta*numnodes);
}

Replay::~Replay()
{
	free(nodes);
	free(times);
}


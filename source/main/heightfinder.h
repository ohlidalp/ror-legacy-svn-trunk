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
#ifndef __HeightFinder_H__
#define __HeightFinder_H__

#include <OgrePrerequisites.h>
#include <OgreVector3.h>
#include "rormemory.h"

#include "OgreTerrain.h"
#include "OgreTerrainGroup.h"

using namespace Ogre;
/**
 * This is the common Interface for all Scenemanager Specific Implementations of the Heightfinder
 */
class HeightFinder
{
public:
	HeightFinder() {};
	virtual ~HeightFinder() {};

	virtual float getHeightAt(float x, float z) = 0;
	virtual void getNormalAt(float x, float y, float z, Ogre::Vector3 *result, float precision=0.1) = 0;
};

// new terrain height finder. For the new terrain from Ogre 1.7
class NTHeightFinder : public HeightFinder, public MemoryAllocatedObject
{
protected:
	TerrainGroup *mTerrainGroup;
	Vector3 mTerrainPos;
public:
	NTHeightFinder(TerrainGroup *tg, Vector3 tp) : mTerrainGroup(tg), mTerrainPos(tp)
	{
	}
	
	~NTHeightFinder()
	{
	}

	float getHeightAt(float x, float z)
	{
		Terrain *t=0;
		return mTerrainGroup->getHeightAtWorldPosition(x, 1000, z, &t);
	}

	void getNormalAt(float x, float y, float z, Ogre::Vector3 *result, float precision=0.1)
	{
		Vector3 left, down;

		left.x = -precision;
		left.y = getHeightAt( x - precision, z ) - y;
		left.z = 0;

		down.x = 0;
		down.y = getHeightAt( x, z + precision ) - y;
		down.z = precision;

		*result = left.crossProduct( down );
		result->normalise();
	}
};

// Scene-Manager Specific implementations

/**
 * Heightfinder for the standart Ogre Terrain Mnager
 */
class TSMHeightFinder : public HeightFinder, public MemoryAllocatedObject
{
protected:
	Ogre::Vector3 scale;
	Ogre::Vector3 inverse_scale;
	int size;
	int size1;
	char cfgfilename[256];
	unsigned short *data;
	float defaulth;
	float dx, dz;
	bool flipped;
	void loadSettings();

public:
	TSMHeightFinder(char *cfgfilename, char *fname, float defaultheight);
	~TSMHeightFinder();

	float getHeightAt(float x, float z);
	void getNormalAt(float x, float y, float z, Ogre::Vector3 *result, float precision=0.1);

};

#endif

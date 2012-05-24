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
#ifndef __HeightFinder_H_
#define __HeightFinder_H_

#include "RoRPrerequisites.h"

#include "OgreTerrainGroup.h"

#include "TerrainManager.h"

/**
 * This is the common interface for all Scene-Manager specific implementations of the Height-Finder
 */
class HeightFinder
{
public:

	HeightFinder() {};
	virtual ~HeightFinder() {};

	virtual float getHeightAt(float x, float z) = 0;
	virtual Ogre::Vector3 getNormalAt(float x, float y, float z, float precision = 0.1f)
	{
		Ogre::Vector3 left(-precision, getHeightAt(x - precision, z) - y, 0.0f);
		Ogre::Vector3 down(0.0f, getHeightAt(x, z + precision) - y, precision);
		down = left.crossProduct(down);
		down.normalise();
		return down;
	}
};

/**
 * New terrain Height-Finder. For the new terrain from Ogre 1.7
 */
class NTHeightFinder : public HeightFinder
{
public:

	NTHeightFinder(TerrainManager *tm, Ogre::Vector3 tp) : mTerrainGroup(0), mTerrainPos(tp)
	{
		mTerrainGroup = tm->getTerrainGroup();
	}
	
	~NTHeightFinder()
	{
	}

	float getHeightAt(float x, float z)
	{
		return mTerrainGroup->getHeightAtWorldPosition(x, 1000, z);
	}

protected:

	Ogre::TerrainGroup *mTerrainGroup;
	Ogre::Vector3 mTerrainPos;
};

/**
 * Height-Finder for the standard Ogre Terrain Manager
 */
class TSMHeightFinder : public HeightFinder
{
public:

	TSMHeightFinder(char *cfgfilename, char *fname, float defaultheight);
	~TSMHeightFinder();

	float getHeightAt(float x, float z);

protected:

	Ogre::String cfgfilename;
	Ogre::Vector3 inverse_scale;
	Ogre::Vector3 scale;
	bool flipped;
	float defaulth;
	float dx, dz;
	int size1;
	int size;
	unsigned short *data;

	void loadSettings();
};

#endif // __HeightFinder_H_

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

// forward definitions
//namespace Ogre {
//	class TerrainSceneManager;
//};

/**
 * This is the common Interface for all Scenemanager Specific Implementations of the Heightfinder
 */
class HeightFinder
{
public:
	HeightFinder() {};
	virtual ~HeightFinder() {};

	virtual float getHeightAt(float x, float z) = 0;
	virtual void getNormalAt(float x, float z, Ogre::Vector3 *result, float precision=0.1) = 0;
};

// Scene-Manager Specific implementations

/**
 * Heightfinder for the standart Ogre Terrain Mnager
 */
class TSMHeightFinder : public HeightFinder
{
protected:
	Ogre::Vector3 scale;
	int size;
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
	void getNormalAt(float x, float z, Ogre::Vector3 *result, float precision=0.1);

	inline int re(int v) {return size-v-1;} // reversed
};

#endif

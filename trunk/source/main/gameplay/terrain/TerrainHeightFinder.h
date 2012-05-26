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
#ifndef TERRAINHEIGHTFINDER_H__
#define TERRAINHEIGHTFINDER_H__

#include "RoRPrerequisites.h"

/**
 * New terrain Height-Finder. For the new terrain from Ogre 1.7
 */
class TerrainHeightFinder : public HeightFinder
{
public:
	TerrainHeightFinder(TerrainGeometryManager *tgm) : mTerrainGroup(tgm->getTerrainGroup())
	{
	}

	inline float getHeightAt(float x, float z)
	{
		return mTerrainGroup->getHeightAtWorldPosition(x, 1000, z);
	}

protected:
	Ogre::TerrainGroup *mTerrainGroup;
};

#endif // TERRAINHEIGHTFINDER_H__




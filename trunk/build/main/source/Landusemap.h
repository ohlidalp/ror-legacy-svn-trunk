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
//created by thomas fischer 23 February 2009
#ifndef __Tractionmap_H__
#define __Tractionmap_H__

#include <OgrePrerequisites.h>
#include <OgreVector3.h>
#include "collisions.h"

class Landusemap
{
protected:
	unsigned char *data;
	void loadSettings();
	void loadTerrainSettings();
	Ogre::String configFilename;
	Ogre::String TSMconfigFilename;
	Ogre::String textureFilename;
	char defaultUse;
	Collisions *coll;
	Ogre::Real mapsizex, mapsizez;
	std::map < Ogre::uint32, Ogre::String > usemap;

public:
	Landusemap(Ogre::String cfgfilename, Collisions *c, Ogre::Real mapsizex, Ogre::Real mapsizez);
	~Landusemap();

	ground_model_t *getGroundModelAt(int x, int z);
};

#endif

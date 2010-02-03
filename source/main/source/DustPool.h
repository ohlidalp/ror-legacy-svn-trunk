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
#ifndef __DustPool_H__
#define __DustPool_H__

#include <stdio.h>
#include <math.h>

#include "OgrePrerequisites.h"
#include "OgreVector3.h"
#include "OgreColourValue.h"
#include "rormemory.h"
//#include "OgreDeflectorPlaneAffector.h"

class Water;

typedef struct dustatom_t
{
	Ogre::ParticleSystem *ps;
	Ogre::SceneNode *node;
	Ogre::Vector3 position;
	Ogre::Vector3 velocity;
	Ogre::ColourValue colour;
	float rate;
} dustatom_t;

class DustPool : public MemoryAllocatedObject
{
protected:
	Water* w;
	Ogre::SceneManager *smgr;
	Ogre::SceneNode *parentNode;
	Ogre::String dname;
	int size;
	int allocated;

	std::vector<dustatom_t> dustAtomsVec;

	float minvelo, maxvelo, fadeco, timedelta, velofac, ttl;

public:
	DustPool(Ogre::String name, int dsize, float minvelo, float maxvelo, float fadeco, float timedelta, float velofac, float ttl, Ogre::SceneNode *parent, Ogre::SceneManager *smgr, Water *mw=NULL);
	~DustPool();

	void alloc(Ogre::Vector3 pos, Ogre::Vector3 vel, Ogre::ColourValue col = Ogre::ColourValue::ZERO, float time = -1);
	void update(float gspeed);

	void setWater(Water *mw) { this->w = mw; };
	void setVisible(bool s);
};

#endif



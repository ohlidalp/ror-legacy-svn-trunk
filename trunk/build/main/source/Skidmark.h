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
#ifndef SkidMark_H
#define SkidMark_H

#include "OgrePrerequisites.h"
#include "DynamicRenderable.h"
#include <vector>
#include "Beam.h"

typedef struct _skidmark
{
	Ogre::ManualObject *obj;
	std::vector<Ogre::Vector3> points;
	std::vector<int> ground_model_id;
	Ogre::Vector3 lastPoint;
	int pos;
	Ogre::ColourValue colour;
	Ogre::Vector3 face[2];
	int facecounter;
} skidmark_t;

class Skidmark
{
public:
	/// Constructor - see setOperationType() for description of argument.
	Skidmark(Ogre::SceneManager *scm, wheel_t *wheel, HeightFinder *hfinder, Ogre::SceneNode *snode, int lenght=500, int bucketCount=20);
	virtual ~Skidmark();

	void updatePoint();

	void update();

private:
	static int instancecounter;
	Ogre::SceneManager *scm;
	Ogre::SceneNode *mNode;
	HeightFinder *hfinder;
	
	std::queue<skidmark_t> objects;
	bool mDirty;
	int lenght;
	int bucketCount;
	wheel_t *wheel;
	float minDistance, maxDistance, minDistanceSquared, maxDistanceSquared;
	static Ogre::Vector2 tex_coords[4][4];
	
	void limitObjects();
	void addObject(Ogre::Vector3 start);
	void setPointInt(unsigned short index, const Ogre::Vector3 &value);
	void addPoint(const Ogre::Vector3 &value);
};


#endif


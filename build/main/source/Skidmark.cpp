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

#include "Skidmark.h"
#include <Ogre.h>
#include <cassert>
#include <cmath>

using namespace Ogre;
int Skidmark::instancecounter = 0;

Skidmark::Skidmark(SceneManager *scm, wheel_t *wheel, SceneNode *snode, int _lenght, int bucketCount) : scm(scm), mNode(snode), lenght(_lenght), wheel(wheel), bucketCount(bucketCount)
{
	minDistance = std::max(0.2f, wheel->width*0.9f);
	minDistanceSquared = pow(minDistance, 2);
	maxDistance = std::max(0.5f, wheel->width*1.1f);
	maxDistanceSquared = pow(maxDistance, 2);
	mDirty = true;
}

Skidmark::~Skidmark()
{
}

void Skidmark::addObject(Vector3 start)
{
	LogManager::getSingleton().logMessage("new skidmark section");
	skidmark_t skid;
	skid.pos=0;
	skid.lastPoint=start;
	skid.facecounter=0;
	for(int i=0;i<3;i++) skid.face[i] = Vector3::ZERO;
	skid.colour = ColourValue(Math::RangeRandom(0, 100)/100.0f, Math::RangeRandom(0, 100)/100.0f, Math::RangeRandom(0, 100)/100.0f);

	skid.points.resize(lenght);	
	skid.obj = scm->createManualObject("skidmark" + StringConverter::toString(instancecounter++));
	skid.obj->setDynamic(true);
	skid.obj->setRenderingDistance(80000);
	skid.obj->begin("tracks/transred", RenderOperation::OT_TRIANGLE_STRIP);
	for(int i = 0; i < lenght; i++)
	{
		skid.points[i] = start;
		skid.obj->position(start);
	}
    skid.obj->end();
	mNode->attachObject(skid.obj);


	objects.push(skid);

	limitObjects();
}

void Skidmark::limitObjects()
{
	if((int)objects.size() > bucketCount)
	{
		LogManager::getSingleton().logMessage("deleting first skidmarks section to keep the limits");
		objects.front().points.clear();
		scm->destroyManualObject(objects.front().obj);
		objects.pop();
	}
}

void Skidmark::setPointInt(unsigned short index, const Vector3 &value)
{
	objects.back().points[index] = value;
	mDirty = true;
}

void Skidmark::updatePoint()
{
	Vector3 lastPoint = wheel->lastContactType?wheel->lastContactOuter:wheel->lastContactInner;
	if(!objects.size())
	{
		// new bucket
		addObject(lastPoint);
	} else
	{
		// check existing buckets
		skidmark_t skid = objects.back();

		// too near to update?
		if(skid.lastPoint.squaredDistance(wheel->lastContactInner) < minDistanceSquared 
			|| skid.lastPoint.squaredDistance(wheel->lastContactOuter) < minDistanceSquared)
		{
			//LogManager::getSingleton().logMessage("E: too near for update");
			return;
		}

		// far enough for new bucket?
		float maxDist = maxDistance;
		if(wheel->speed > 0) maxDist *= wheel->speed;
		//if((skid.lastPoint != Vector3::ZERO && fabs(skid.lastPoint.distance(lastPoint)) > maxDist) || skid.pos >= (int)skid.points.size())
		if(skid.pos >= (int)skid.points.size())
		{
			// add connection to last bucket
			Vector3 lp1 = objects.back().points[objects.back().pos-1];
			Vector3 lp2 = objects.back().points[objects.back().pos-2];
			addObject(lp1);
			addPoint(lp2);

		}
	}

	skidmark_t skid = objects.back();

	// tactics: we always choose the latest oint and then create two points
	Vector3 axis = wheel->refnode1->RelPosition - wheel->refnode0->RelPosition;
	// choose node wheel by the latest added point
	if(!wheel->lastContactType)
	{
		// choose inner
		//LogManager::getSingleton().logMessage("inner");
		addPoint(wheel->lastContactInner);
		addPoint(wheel->lastContactInner - axis);
	} else
	{
		// choose outer
		//LogManager::getSingleton().logMessage("outer");
		addPoint(wheel->lastContactOuter + axis);
		addPoint(wheel->lastContactOuter);
	}

}

void Skidmark::addPoint(const Vector3 &value)
{
	setPointInt(objects.back().pos, value);
	objects.back().pos++;
	objects.back().lastPoint = value;
}

void Skidmark::update()
{
	skidmark_t skid = objects.back();
	Vector3 vaabMin = skid.points[0];
	Vector3 vaabMax = skid.points[0];
	skid.obj->beginUpdate(0);
	bool behindEnd = false;
	Vector3 lastValid = Vector3::ZERO;
	for(int i = 0; i < lenght; i++)
	{
		if(i>=skid.pos) behindEnd=true;

		if(behindEnd)
		{
			skid.obj->position(lastValid);
			skid.obj->colour(ColourValue::Green);
		} else
		{
			skid.obj->position(skid.points[i]);
			skid.obj->colour(skid.colour);
			lastValid = skid.points[i];
		}
		
		if(skid.points[i].x < vaabMin.x) vaabMin.x = skid.points[i].x;
		if(skid.points[i].y < vaabMin.y) vaabMin.y = skid.points[i].y;
		if(skid.points[i].z < vaabMin.z) vaabMin.z = skid.points[i].z;
		if(skid.points[i].x > vaabMax.x) vaabMax.x = skid.points[i].x;
		if(skid.points[i].y > vaabMax.y) vaabMax.y = skid.points[i].y;
		if(skid.points[i].z > vaabMax.z) vaabMax.z = skid.points[i].z;
	}
    skid.obj->end();

	//manual->setBoundingBox(AxisAlignedBox(vaabMin, vaabMax));
	
	// Use infinite AAB to always stay visible
	AxisAlignedBox aabInf;
	aabInf.setInfinite();
	skid.obj->setBoundingBox(aabInf);

	mDirty = false;
}


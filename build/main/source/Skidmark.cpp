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

Skidmark::Skidmark(SceneManager *scm, SceneNode *snode, int _lenght) : scm(scm), mNode(snode), lenght(_lenght)
{
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
	skid.colour = ColourValue(Math::RangeRandom(0, 100)/100.0f, Math::RangeRandom(0, 100)/100.0f, Math::RangeRandom(0, 100)/100.0f);

	skid.points.resize(lenght);	
	skid.obj = scm->createManualObject("skidmark" + StringConverter::toString(instancecounter++));
	skid.obj->setDynamic(true);
	skid.obj->setRenderingDistance(800);
	skid.obj->begin("tracks/skidmark", RenderOperation::OT_LINE_STRIP); //OT_LINE_STRIP);
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
	if(objects.size()>5)
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

void Skidmark::setPoint(const Vector3 &value)
{
	float minDistance = 0.05f;
	float maxDistance = 0.5f;


	// far enough for new section?
	if(!objects.size())
	{
		addObject(value);
	} else
	{
		skidmark_t skid = objects.back();
		// too near to update?
		if(fabs(skid.lastPoint.distance(value)) < minDistance) return;
		
		// far enough for new section?
		if((skid.lastPoint != Vector3::ZERO && fabs(skid.lastPoint.distance(value)) > maxDistance) || skid.pos >= skid.points.size())
			addObject(value);
	}

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
	for(int i = 0; i < lenght; i++)
	{
		skid.obj->position(skid.points[i]);
		skid.obj->colour(skid.colour);
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


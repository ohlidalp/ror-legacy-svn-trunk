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
	if(lenght%2) lenght -= lenght%2; // round it!

	minDistance = 0.3f; //std::max(0.2f, wheel->width*0.9f);
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
	skid.colour = ColourValue(Math::RangeRandom(0, 100)/100.0f, Math::RangeRandom(0, 100)/100.0f, Math::RangeRandom(0, 100)/100.0f, 0.8f);


	// new material
	char bname[255]="";
	sprintf(bname, "mat-skidmark-%d", instancecounter);
	MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().create(bname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));
	Pass *p = mat->getTechnique(0)->getPass(0);
	TextureUnitState *tus = p->createTextureUnitState("tracks.png");
	//tus->setColourOperationEx(LBX_MODULATE, LBS_MANUAL, LBS_CURRENT, skid.colour);
	//tus->setTextureName();
	p->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
	p->setLightingEnabled(false);
	p->setDepthWriteEnabled(false);
	p->setDepthBias(3, 3);
	p->setCullingMode(Ogre::CULL_NONE);

	skid.points.resize(lenght);	
	skid.obj = scm->createManualObject("skidmark" + StringConverter::toString(instancecounter++));
	skid.obj->setDynamic(true);
	skid.obj->setRenderingDistance(800);
	skid.obj->begin(bname, RenderOperation::OT_TRIANGLE_STRIP);
	for(int i = 0; i < lenght; i++)
	{
		skid.points[i] = start;
		skid.obj->position(start);
		skid.obj->textureCoord(0,0);
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
		if(skid.pos >= (int)skid.points.size())
		{
			// add connection to last bucket
			Vector3 lp1 = objects.back().points[objects.back().pos-1];
			Vector3 lp2 = objects.back().points[objects.back().pos-2];
			addObject(lp1);
			addPoint(lp2);
			addPoint(lp1);
		}
		else if((skid.lastPoint != Vector3::ZERO && fabs(skid.lastPoint.distance(lastPoint)) > maxDist))
		{
			// just new bucket, no connection to last bucket
			addObject(lastPoint);
		}
	}

	skidmark_t skid = objects.back();

	float overaxis = 0.2f;
	// tactics: we always choose the latest oint and then create two points
	Vector3 axis = wheel->refnode1->RelPosition - wheel->refnode0->RelPosition;
	// choose node wheel by the latest added point
	if(!wheel->lastContactType)
	{
		// choose inner
		//LogManager::getSingleton().logMessage("inner");
		addPoint(wheel->lastContactInner + (axis * overaxis));
		addPoint(wheel->lastContactInner - axis - (axis * overaxis));
	} else
	{
		// choose outer
		//LogManager::getSingleton().logMessage("outer");
		addPoint(wheel->lastContactOuter + axis + (axis * overaxis));
		addPoint(wheel->lastContactOuter - (axis * overaxis));
	}

}

void Skidmark::addPoint(const Vector3 &value)
{
	if(objects.back().pos >= lenght)
	{
		LogManager::getSingleton().logMessage("E: boundary protection hit");
		return;
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
	bool behindEnd = false;
	Vector3 lastValid = Vector3::ZERO;
	int to_counter=0;
	Vector2 tex_coords[4] = {Vector2(0,0), Vector2(0,1), Vector2(1,0), Vector2(1,1)};
	for(int i = 0; i < lenght; i++, to_counter++)
	{
		if(i>=skid.pos) behindEnd=true;
		if(to_counter>3) to_counter=0;

		float len = 1.0f;
		if(!behindEnd && i>0 && skid.points[i] != Vector3::ZERO && skid.points[i-1] != Vector3::ZERO)
		{
			len = minDistance / (skid.points[i].distance(skid.points[i-1]));
		}

		if(behindEnd)
		{
			skid.obj->position(lastValid);
			skid.obj->textureCoord(0,0);
		} else
		{
			skid.obj->position(skid.points[i]);
			Vector2 tco = tex_coords[to_counter];

			tco.x *= 1 - len;

			skid.obj->textureCoord(tco);


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

	skid.obj->setBoundingBox(AxisAlignedBox(vaabMin, vaabMax));
	
	// Use infinite AAB to always stay visible
	/*
	// for debugging only
	AxisAlignedBox aabInf;
	aabInf.setInfinite();
	skid.obj->setBoundingBox(aabInf);
	*/

	mDirty = false;
}


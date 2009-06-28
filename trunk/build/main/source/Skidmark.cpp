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
#include "heightfinder.h"
#include <Ogre.h>
#include <cassert>
#include <cmath>

using namespace Ogre;
int Skidmark::instancecounter = 0;

// this is a hardcoded array which we use to map ground types to a certain texture with UV/ coords
Vector2 Skidmark::tex_coords[4][4] = {
	{Vector2(0,0.00f), Vector2(0,0.25f), Vector2(1,0.00f), Vector2(1,0.25f)},
	{Vector2(0,0.25f), Vector2(0,0.50f), Vector2(1,0.25f), Vector2(1,0.50f)},
	{Vector2(0,0.50f), Vector2(0,0.75f), Vector2(1,0.50f), Vector2(1,0.75f)},
	{Vector2(0,0.75f), Vector2(0,1.00f), Vector2(1,0.75f), Vector2(1,1.00f)},
};

Skidmark::Skidmark(SceneManager *scm, wheel_t *wheel, HeightFinder *hfinder, SceneNode *snode, int _lenght, int bucketCount) : scm(scm), hfinder(hfinder), mNode(snode), lenght(_lenght), wheel(wheel), bucketCount(bucketCount)
{
	if(lenght%2) lenght -= lenght%2; // round it!

	minDistance = 0.1f;
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
	//LogManager::getSingleton().logMessage("new skidmark section");
	skidmark_t skid;
	skid.pos=0;
	skid.lastPointAv=start;
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
	skid.facesizes.resize(lenght);
	skid.ground_model_id.resize(lenght);
	skid.obj = scm->createManualObject("skidmark" + StringConverter::toString(instancecounter++));
	skid.obj->setDynamic(true);
	skid.obj->setRenderingDistance(2000); //2km sight range
	skid.obj->begin(bname, RenderOperation::OT_TRIANGLE_STRIP);
	for(int i = 0; i < lenght; i++)
	{
		skid.points[i] = start;
		skid.facesizes[i] = 0;
		skid.ground_model_id[i] = 0;
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
		//LogManager::getSingleton().logMessage("deleting first skidmarks section to keep the limits");
		objects.front().points.clear();
		objects.front().facesizes.clear();
		scm->destroyManualObject(objects.front().obj);
		objects.pop();
	}
}

void Skidmark::setPointInt(unsigned short index, const Vector3 &value, Real fsize)
{
	objects.back().points[index] = value;
	objects.back().facesizes[index] = fsize;
	
	int model_id = -1; // nothing

	// normal road texture
	if(wheel->lastSlip > 2)
		model_id = 0; // normal tire tracks

	// grass
	if(model_id == 0 && (!strncmp(wheel->lastGroundModel->name, "grass", 255) || !strncmp(wheel->lastGroundModel->name, "gravel", 255)))
		model_id = 3;

	// rough textures
	if(wheel->lastSlip > 5 && model_id == 0) model_id = 1; // rough street
	if(wheel->lastSlip > 5 && model_id == 3) model_id = 2; // rough grass/gravel

	objects.back().ground_model_id[index] = model_id;

	mDirty = true;
}

void Skidmark::updatePoint()
{
	Vector3 thisPoint = wheel->lastContactType?wheel->lastContactOuter:wheel->lastContactInner;
	Vector3 axis = wheel->lastContactType?(wheel->refnode1->RelPosition - wheel->refnode0->RelPosition):(wheel->refnode0->RelPosition - wheel->refnode1->RelPosition);
	Vector3 thisPointAV = thisPoint + axis * 0.5f;
	Real distance = 0;

	if(!objects.size())
	{
		// add first bucket
		addObject(thisPoint);
	} else
	{
		// check existing buckets
		skidmark_t skid = objects.back();

		distance = skid.lastPointAv.distance(thisPointAV);
		// too near to update?
		if(distance < minDistance)
		{
			//LogManager::getSingleton().logMessage("E: too near for update");
			return;
		}

		// far enough for new bucket?
		float maxDist = maxDistance;
		if(wheel->speed > 0) maxDist *= wheel->speed;
		if(skid.pos >= (int)skid.points.size())
		{
			// add new bucket with connection to last bucket
			Vector3 lp1 = objects.back().points[objects.back().pos-1];
			Vector3 lp2 = objects.back().points[objects.back().pos-2];
			addObject(lp1);
			addPoint(lp2, distance);
			addPoint(lp1, distance);
		}
		else if(distance > maxDistance)
		{
			// just new bucket, no connection to last bucket
			addObject(thisPoint);
		}
	}

	skidmark_t skid = objects.back();

	float overaxis = 0.2f;
	// tactics: we always choose the latest oint and then create two points
	
	// XXX: TO BE IMPROVED
	//Vector3 groundNormal = Vector3::ZERO;
	//hfinder->getNormalAt(thisPoint.x, thisPoint.y, thisPoint.z, &groundNormal);

	//LogManager::getSingleton().logMessage("ground normal: "+StringConverter::toString(wheel->refnode1->RelPosition.dotProduct(groundNormal)));

	// choose node wheel by the latest added point
	if(!wheel->lastContactType)
	{
		// choose inner
		//LogManager::getSingleton().logMessage("inner");
		addPoint(wheel->lastContactInner - (axis * overaxis), distance);
		addPoint(wheel->lastContactInner + axis + (axis * overaxis), distance);
	} else
	{
		// choose outer
		//LogManager::getSingleton().logMessage("outer");
		addPoint(wheel->lastContactOuter + axis + (axis * overaxis), distance);
		addPoint(wheel->lastContactOuter - (axis * overaxis), distance);
	}

	// save as last point (in the middle of the wheel)
	objects.back().lastPointAv = thisPointAV;

	/*
	// debug code: adds boxes to the average point
	SceneNode *sn = mNode->getParentSceneNode()->createChildSceneNode();
	sn->attachObject(scm->createEntity("addPointTRACK"+StringConverter::toString(objects.back().lastPointAv) +StringConverter::toString(axis), "beam.mesh"));
	sn->setPosition(thisPointAV);
	sn->setScale(0.1f, 0.01f, 0.1f);
	*/

}

void Skidmark::addPoint(const Vector3 &value, Real fsize)
{
	if(objects.back().pos >= lenght)
	{
		//LogManager::getSingleton().logMessage("E: boundary protection hit");
		return;
	}
	setPointInt(objects.back().pos, value, fsize);
	objects.back().pos++;
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
	int current_gmodel=skid.ground_model_id[0];
	float tcox_counter=0;

	for(int i = 0; i < lenght; i++, to_counter++)
	{
		if(i>=skid.pos-1) behindEnd=true;
		if(to_counter>3)
		{
			current_gmodel=skid.ground_model_id[i];
			to_counter=0;
		}

		if(!behindEnd)
			tcox_counter += skid.facesizes[i] * 0.001f;

		while(tcox_counter>1)
			tcox_counter--;

		if(behindEnd)
		{
			skid.obj->position(lastValid);
			skid.obj->textureCoord(0,0);
		} else
		{
			skid.obj->position(skid.points[i]);

			Vector2 tco = tex_coords[current_gmodel][to_counter];
			tco.x += tcox_counter; // scale texture according face size
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


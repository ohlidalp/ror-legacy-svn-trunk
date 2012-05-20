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
#ifdef USE_MYGUI

#include "MapControl.h"

#include "BeamData.h"
#include "MapEntity.h"
#include "Ogre.h"

using namespace Ogre;

MapControl::MapControl(int mapsizex, int mapsizez) :
	  mMapWidth(mapsizex)
	, mMapHeight(mapsizez)
	, mAlpha(1.0f)
	, mScale(1.0f)
{
	initialiseByAttributes(this);
	setVisibility(false);
}

void MapControl::setMapTexture(String name)
{
	mMapTexture->setImageTexture(name);
}

void MapControl::setWorldSize(int x, int z)
{
	mMapWidth = x;
	mMapHeight = z;
}

MapEntity *MapControl::createMapEntity(String type)
{
	MapEntity *entity = new MapEntity(this, type, mMapTexture);
	mMapEntities.insert(entity);
	return entity;
}

MapEntity *MapControl::createNamedMapEntity(String name, String type)
{
	MapEntity *entity = createMapEntity(type);
	mNamedEntities[name] = entity;
	return entity;
}

MapEntity *MapControl::getEntityByName(String name)
{
	if (mNamedEntities.find(name) != mNamedEntities.end())
	{
		return mNamedEntities[name];
	}
	return NULL;
}

String MapControl::getTypeByDriveable(int driveable)
{
	switch (driveable)
	{
	case NOT_DRIVEABLE:
		return "load";
	case TRUCK:
		return "truck";
	case AIRPLANE:
		return "airplane";
	case BOAT:
		return "boat";
	case MACHINE:
		return "machine";
	default:
		return "unknown";
	}
}

void MapControl::deleteMapEntity(MapEntity *entity)
{
	mMapEntities.erase(entity);
}

bool MapControl::getVisibility()
{
	return mMainWidget->getVisible();
}

void MapControl::setVisibility(bool value)
{
	mMainWidget->setVisible(value);
}

void MapControl::setAlpha(float value)
{
	mAlpha = value;
	mMainWidget->setAlpha(value);
}

void MapControl::setPosition(float x, float y, float w, float h, RenderWindow* rw)
{
	updateRenderMetrics(rw);

	mScale = w;
	mMainWidget->setCoord(x * rWinWidth, y * rWinHeight, w * rWinWidth, h * rWinHeight);

	updateEntityPositions();
}

void MapControl::updateEntityPositions()
{
	for (std::set<MapEntity *>::iterator it = mMapEntities.begin(); it != mMapEntities.end(); it++)
	{
		(*it)->update();
	}
}

void MapControl::setEntitiesVisibility(bool value)
{
	for (std::set<MapEntity *>::iterator it = mMapEntities.begin(); it != mMapEntities.end(); it++)
	{
		(*it)->setVisibility(value);
	}
}

void MapControl::updateRenderMetrics(RenderWindow* win)
{
	win->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
}

#endif // USE_MYGUI

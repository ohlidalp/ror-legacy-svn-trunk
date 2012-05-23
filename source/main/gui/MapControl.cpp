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

MapControl::MapControl(float mapsizex, float mapsizey, float mapsizez) :
	  mMapSize(Vector3(mapsizex, mapsizey, mapsizez))
	, mAlpha(1.0f)
	, mScale(1.0f)
	, mX(0)
	, mY(0)
{
	initialiseByAttributes(this);
	setVisibility(false);
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

void MapControl::deleteMapEntity(MapEntity *entity)
{
	mMapEntities.erase(entity);
}

MapEntity *MapControl::getEntityByName(String name)
{
	if (mNamedEntities.find(name) != mNamedEntities.end())
	{
		return mNamedEntities[name];
	}
	return NULL;
}

bool MapControl::getVisibility()
{
	return mMainWidget->getVisible();
}

void MapControl::setAlpha(float value)
{
	mAlpha = value;
	mMainWidget->setAlpha(value);
}

void MapControl::setEntitiesVisibility(bool value)
{
	for (std::set<MapEntity *>::iterator it = mMapEntities.begin(); it != mMapEntities.end(); it++)
	{
		(*it)->setVisibility(value);
	}
}

void MapControl::setMapTexture(String name)
{
	mMapTexture->setImageTexture(name);
}

void MapControl::setPosition(int x, int y, float size, Ogre::RenderWindow *rw)
{
	int realx, realy, realw, realh;

	mScale = size;
	mX = x;
	mY = y;

	updateRenderMetrics(rw);
	
	realw = realh = size * std::min(rWinWidth, rWinHeight);

	if (x == -1)
	{
		realx = 0;
	} else if (x == 0)
	{
		realx = (rWinWidth - realw) / 2;
	} else if (x == 1)
	{
		realx = rWinWidth - realw;
	}

	if (y == -1)
	{
		realy = 0;
	} else if (y == 0)
	{
		realy = (rWinHeight - realh) / 2;
	} else if (y == 1)
	{
		realy = rWinHeight - realh;
	}

	mMainWidget->setCoord(realx, realy, realw, realh);

	updateEntityPositions();
}

void MapControl::setVisibility(bool value)
{
	mMainWidget->setVisible(value);
}

void MapControl::setWorldSize(float width, float length, float height)
{
	mMapSize = Vector3(width, length, height);
}

void MapControl::windowResized(Ogre::RenderWindow *rw)
{
	setPosition(mX, mY, mScale, rw);
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

void MapControl::updateEntityPositions()
{
	for (std::set<MapEntity *>::iterator it = mMapEntities.begin(); it != mMapEntities.end(); it++)
	{
		(*it)->update();
	}
}

void MapControl::updateRenderMetrics(RenderWindow* win)
{
	win->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
}

#endif // USE_MYGUI

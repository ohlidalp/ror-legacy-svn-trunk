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

using namespace Ogre;

MapControl::MapControl(int mapsizex, int mapsizez) :
	  h(100.0f)
	, mAlpha(1.0f)
	, mapsizex(mapsizex)
	, mapsizez(mapsizez)
	, rWinHeight(1)
	, rWinWidth(1)
	, w(100.0f)
	, x(0.0f)
	, y(0.0f)
{
	initialiseByAttributes(this);
}

MapControl::~MapControl()
{
}

void MapControl::setMapTexture(String _name)
{
	mMapTexture->setImageTexture(_name);
}

void MapControl::setWorldSize(int x, int z)
{
	this->mapsizex = x;
	this->mapsizez = z;
}

MapEntity *MapControl::createMapEntity(String type)
{
	MapEntity *m = new MapEntity(this, type, mMapTexture);
	mMapEntities.push_back(m);
	return m;
}

MapEntity *MapControl::createNamedMapEntity(String name, String type)
{
	MapEntity *e = createMapEntity(type);
	mNamedEntities[name] = e;
	return e;
}

MapEntity *MapControl::getEntityByName(String name)
{
	if (mNamedEntities.find(name) != mNamedEntities.end())
		return mNamedEntities[name];

	return nullptr;
}

String MapControl::getTypeByDriveable(int driveable)
{
	if(driveable == NOT_DRIVEABLE)
		return "load";
	else if(driveable == TRUCK)
		return "truck";
	else if(driveable == AIRPLANE)
		return "airplane";
	else if(driveable == BOAT)
		return "boat";
	else if(driveable == MACHINE)
		return "machine";
	return "unknown";
}

void MapControl::deleteMapEntity(MapEntity *ent)
{
	std::vector<MapEntity *>::iterator it;
	for(it=mMapEntities.begin(); it!= mMapEntities.end(); it++)
	{
		if((*it) == ent)
		{
			// found it, erase!
			delete *it;
			mMapEntities.erase(it);
			return;
		}
	}
}

bool MapControl::getVisibility()
{
	return mMainWidget->getVisible();
}

void MapControl::setVisibility(bool value)
{
	//if(!value) GUIManager::getSingleton().unfocus();
	mMainWidget->setVisible(value);
}

void MapControl::setAlpha(float value)
{
	mAlpha = value;
	mMainWidget->setAlpha(value);
}

void MapControl::setPosition(float _x, float _y, float _w, float _h, RenderWindow* rw)
{
	mMainWidget->setCoord(_x*rWinWidth, _y*rWinHeight, _w*rWinWidth, _h*rWinHeight);
	myScale = _w;
	updateEntityPositions();
}


void MapControl::updateEntityPositions()
{
	std::vector<MapEntity *>::iterator it;
	for(it=mMapEntities.begin(); it!=mMapEntities.end(); it++)
	{
		(*it)->update();
	}
}

void MapControl::setEntitiesVisibility(bool value)
{
	std::vector<MapEntity *>::iterator it;
	for(it=mMapEntities.begin(); it!=mMapEntities.end(); it++)
	{
		(*it)->setVisibility(value);
	}
}

void MapControl::resizeToScreenRatio(RenderWindow* rw)
{
	//win->setRealPosition(
	// TODO
}

void MapControl::updateRenderMetrics(RenderWindow* win)
{
	win->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
}

#endif // USE_MYGUI

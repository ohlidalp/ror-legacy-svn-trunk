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

#ifndef __MAP_CONTROL_H_
#define __MAP_CONTROL_H_

#include "RoRPrerequisites.h"

#include "mygui/BaseLayout.h"

ATTRIBUTE_CLASS_LAYOUT(MapControl, "MapControl.layout");

class MapControl : public wraps::BaseLayout
{
public:

	MapControl(float mapsizex, float mapsizey, float mapsizez);

	MapEntity *createMapEntity(Ogre::String type);
	MapEntity *createNamedMapEntity(Ogre::String name, Ogre::String type);
	
	void deleteMapEntity(MapEntity *ent);

	MapEntity *getEntityByName(Ogre::String name);
	Ogre::Vector3 getMapSize() { return mMapSize; };
	bool getVisibility();
	float getAlpha() { return mAlpha; }
	float getWindowScale() { return mScale; }

	void setAlpha(float value);
	void setEntitiesVisibility(bool value);
	void setMapTexture(Ogre::String name);
	void setPosition(int x, int y, float size, Ogre::RenderWindow *rw);
	void setVisibility(bool value);
	void setWorldSize(float width, float length, float height);

	void windowResized(Ogre::RenderWindow *rw);

	static Ogre::String getTypeByDriveable(int driveable);

protected:

	float mAlpha, mScale;
	int mX, mY;

	Ogre::Vector3 mMapSize;

	ATTRIBUTE_FIELD_WIDGET_NAME(MapControl, mMapTexture, "mMapTexture");
	MyGUI::StaticImage* mMapTexture;

	std::map<Ogre::String, MapEntity *> mNamedEntities;
	std::set<MapEntity *> mMapEntities;

	void updateEntityPositions();

	int rWinLeft, rWinTop;
	unsigned int rWinWidth, rWinHeight, rWinDepth;

	void updateRenderMetrics(Ogre::RenderWindow* win);
};

#endif // __MAP_CONTROL_H_

#endif // USE_MYGUI

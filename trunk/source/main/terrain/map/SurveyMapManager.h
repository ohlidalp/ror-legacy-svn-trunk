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

ATTRIBUTE_CLASS_LAYOUT(SurveyMapManager, "MapControl.layout");

class SurveyMapManager : public wraps::BaseLayout, public ZeroedMemoryAllocator
{
public:

	SurveyMapManager(Ogre::Vector3 worldSize);

	SurveyMapEntity *createMapEntity(Ogre::String type);
	SurveyMapEntity *createNamedMapEntity(Ogre::String name, Ogre::String type);
	
	void deleteMapEntity(SurveyMapEntity *ent);

	SurveyMapEntity *getEntityByName(Ogre::String name);
	Ogre::Vector3 getMapSize() { return mMapSize; };
	bool getVisibility();
	float getAlpha() { return mAlpha; }
	float getWindowScale() { return mScale; }

	void setAlpha(float value);
	void setEntitiesVisibility(bool value);
	void setMapTexture(Ogre::String name);
	void setPosition(int x, int y, float size);
	void setVisibility(bool value);

	void windowResized();

	static Ogre::String getTypeByDriveable(int driveable);

protected:

	float mAlpha, mScale;
	int mX, mY;

	Ogre::Vector3 mMapSize;
	Ogre::Vector3 mWorldSize;

	ATTRIBUTE_FIELD_WIDGET_NAME(SurveyMapManager, mMapTexture, "mMapTexture");
	MyGUI::StaticImage* mMapTexture;

	std::map<Ogre::String, SurveyMapEntity *> mNamedEntities;
	std::set<SurveyMapEntity *> mMapEntities;

	void updateEntityPositions();

	int rWinLeft, rWinTop;
	unsigned int rWinWidth, rWinHeight, rWinDepth;

	void updateRenderMetrics();
};

#endif // __MAP_CONTROL_H_

#endif // USE_MYGUI

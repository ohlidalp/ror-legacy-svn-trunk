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

#ifndef __IngameEditor_H__
#define __IngameEditor_H__

#include "Ogre.h"

// some shortcut
#define INGAMEEDITOR IngameEditor::Instance()

class ExampleFrameListener;

#include "MovableText.h"
#include "Beam.h"

enum {IE_GEOMETRY, IE_flares};

class IngameEditor
{
public:
	static IngameEditor & Instance();
	bool show(bool show);
	void frameStep(const Ogre::FrameEvent& evt);
	void setup(ExampleFrameListener *efl);
	bool wasSetup() { return initiated; };


protected:
	IngameEditor();
	IngameEditor(const IngameEditor&);
	IngameEditor& operator= (const IngameEditor&);

private:
	static IngameEditor* myInstance;
	ExampleFrameListener *mefl;
	float mTimeUntilNextToggle;
	
	Ogre::Vector3 virtualPosition, virtualPosition2;
	void positionChanged(bool rotationchanged=false);
	
	Ogre::Plane gridPlane;
	Ogre::SceneNode *gridNode;
	Ogre::Vector3 basePosition;
	Ogre::Quaternion baseRotation;
	
	Ogre::Vector3 tmpPosition;
	Ogre::SceneNode *selectionNode;
	Ogre::SceneNode *selectionNode2;
	Ogre::SceneNode *selectionNode3;
	Ogre::SceneNode *lineNode, *lineNode2;
	Ogre::ManualObject *lineManualObject, *lineManualObject2;
	Ogre::Vector2 gridsize;
	Ogre::Vector2 gridTranslation;
	Beam *truck;
	int truckNumber;
	int attachNodeNumber;
	int tmpNodeNumber;
	int placeMode;
	int editorMode;
	int savedSkeletonMode;
	float savedNearClip;
	Ogre::Vector3 offPos;
	bool hideFrontBeams;
	bool initiated;
	int beamType;
	Ogre::MovableText *beamTypeText;
	Ogre::MovableText *selectorText;

	void applyBeamType(beam_t *b);
	void updateInput();
	void switchmode(int newmode);
	void getTruckDirection();
	float truckangle;
};


#endif

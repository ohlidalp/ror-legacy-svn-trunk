/*
This source file is part of Rigs of Rods
Copyright 2005-2011 Pierre-Michel Ricordel
Copyright 2007-2011 Thomas Fischer

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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 11th of March 2011

#ifndef SCENEMOUSE_H__
#define SCENEMOUSE_H__

#include "RoRPrerequisites.h"
#include <OgrePrerequisites.h>
#include <OgreSingleton.h>
#include <OIS.h>


class SceneMouse : public Ogre::Singleton < SceneMouse >
{
public:
	SceneMouse(Ogre::SceneManager *scm, RoRFrameListener *rfl);
	~SceneMouse();

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool keyPressed(const OIS::KeyEvent& _arg);
    bool keyReleased(const OIS::KeyEvent& _arg);


	void update(float dt);
	bool isMouseGrabbed() { return mouseGrabState != 0; };

protected:
	Ogre::SceneManager *scm;
	RoRFrameListener *rfl;
	Ogre::ManualObject *pickLine;
	Ogre::SceneNode *pickLineNode;
	float mouseGrabForce;
	int mouseGrabState;


	int minnode;
	float mindist;
	Beam *grab_truck;
	Ogre::Vector3 lastgrabpos;
	int lastMouseX, lastMouseY;

	void releaseMousePick();
	Ogre::Ray getMouseRay();
};



#endif //SCENEMOUSE_H__

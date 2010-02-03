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

#ifndef __Mirrors_H__
#define __Mirrors_H__

#include "Ogre.h"
#include "rormemory.h"

using namespace Ogre;

class Mirrors : public MemoryAllocatedObject
{
private:
	Camera *mCamera;
	Camera *mMirrorCam;
	RenderTexture* rttTex;
	//Plane mirrorPlane;
	SceneNode *tnodeo;
	SceneNode *tnoden;
	MaterialPtr mat;

public:

	Mirrors(SceneManager *mSceneMgr, RenderWindow *mWindow, Camera *camera);
void setActive(bool state);

void update(Vector3 normal, Vector3 center, Ogre::Radian rollangle);
void prepareShutdown();

};



#endif

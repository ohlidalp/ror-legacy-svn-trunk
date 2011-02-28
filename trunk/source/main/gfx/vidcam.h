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
#ifndef VIDCAM_H__
#define VIDCAM_H__

#include "RoRPrerequisites.h"

#include <OgreMaterial.h>

#include "Beam.h"

class VideoCamera
{
public:
	VideoCamera(Ogre::SceneManager *mSceneMgr, Ogre::Camera *camera, Beam *truck);

	void init();

	void update();

	void setActive(bool state);
	
	static VideoCamera *parseLine(Ogre::SceneManager *mSceneMgr, Ogre::Camera *camera, Beam *truck, const char *fname, char *line, int linecounter);

	float fov, minclip, maxclip;
	int nx, ny, nref, offx, offy, offz, texx, texy, cammode;
	Ogre::String materialName, disabledTexture;

protected:
	Ogre::SceneManager *mSceneMgr;
	Ogre::Camera *camera;
	Beam *truck;
	static int counter;
	Ogre::Camera *mCamera;
	Ogre::Camera *mVidCam;
	Ogre::RenderTexture* rttTex;
	Ogre::MaterialPtr mat;

};



#endif // VIDCAM_H__

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
#include "SerializedRig.h"

class VideoCamera
{
public:
	VideoCamera(Ogre::SceneManager *mSceneMgr, Ogre::Camera *camera, rig_t *truck);

	void init();

	void update(float dt);

	void setActive(bool state);
	//static VideoCamera *setActive(bool state);
	
	static VideoCamera *parseLine(Ogre::SceneManager *mSceneMgr, Ogre::Camera *camera, SerializedRig *truck, parsecontext_t &c);

	int camNode, lookat, switchoff;
	float fov, minclip, maxclip;
	Ogre::Vector3 offset;
	Ogre::Vector2 mirrorSize;
	Ogre::Quaternion rotation;
	
	int nz, ny, nref, camRole;
	Ogre::String materialName, disabledTexture, vidCamName;

protected:
	Ogre::SceneManager *mSceneMgr;
	rig_t *truck;
	static int counter;
	Ogre::Camera *mCamera;
	Ogre::Camera *mVidCam;
	Ogre::RenderTexture* rttTex;
	Ogre::MaterialPtr mat;
	bool debugMode;
	Ogre::SceneNode *debugNode;
	MaterialReplacer *mr;
	Ogre::RenderWindow *rwMirror;
};



#endif // VIDCAM_H__

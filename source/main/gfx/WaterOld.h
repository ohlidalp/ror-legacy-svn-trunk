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
#ifndef __WaterOld_H__
#define __WaterOld_H__

#include "RoRPrerequisites.h"

#include "Ogre.h"
#include "water.h"

extern float mrtime;

class WaterOld : public Water
{
public:
	WaterOld(int type, Ogre::Camera *camera, Ogre::SceneManager *mSceneMgr, Ogre::RenderWindow *mWindow, float wheight, float *mapsizex, float *mapsizez, bool usewaves);

	float getHeight();
	float getHeightWaves(Ogre::Vector3 pos);
	Ogre::Vector3 getVelocity(Ogre::Vector3 pos);

	void setFadeColour(Ogre::ColourValue ambient);
	void setHeight(float value);
	void setSunPosition(Ogre::Vector3);
	void setVisible(bool value);

	bool allowUnderWater();
	void framestep(float dt);
	void moveTo(Ogre::Camera *cam, float centerheight);
	void prepareShutdown();
	void showWave(Ogre::Vector3 refpos);
	void update();
	void updateReflectionPlane(float h);

	enum water_quality {WATER_FULL_QUALITY, WATER_FULL_SPEED, WATER_REFLECT, WATER_BASIC};

private:

	typedef struct
	{
		float amplitude;
		float maxheight;
		float wavelength;
		float wavespeed;
		float direction;
	} wavetrain_t;

	static const int WAVEREZ = 100;
	static const int MAX_WAVETRAINS = 10;

	bool haswaves;
	bool visible;
	Ogre::Camera *mCamera;
	Ogre::Camera *mReflectCam;
	Ogre::Camera *mRefractCam;
	float *mapsizex, *mapsizez;
	float *wbuffer;
	float height, orgheight;
	float maxampl;
	float mScale;
	Ogre::HardwareVertexBufferSharedPtr wbuf;
	int framecounter;
	int free_wavetrain;
	int mType;
	Ogre::Viewport *vRtt1, *vRtt2;
	Ogre::RenderTexture* rttTex1;
	Ogre::RenderTexture* rttTex2;
	Ogre::SceneNode *pBottomNode;
	Ogre::SceneNode *pTestNode;
	wavetrain_t wavetrains[MAX_WAVETRAINS];
};

#endif // __WaterOld_H__

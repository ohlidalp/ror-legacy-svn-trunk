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
#ifndef __MAP_TEXTURE_CREATOR_H__
#define __MAP_TEXTURE_CREATOR_H__

#include "RoRPrerequisites.h"
#include <Ogre.h>

class MapTextureCreator : public Ogre::RenderTargetListener
{
public:
	MapTextureCreator(Ogre::SceneManager *smgr, Ogre::Camera *mainCam, RoRFrameListener *efl);
	Ogre::String getMaterialName();
	Ogre::String getRTName();
	void update();
	void setStaticGeometry(Ogre::StaticGeometry *geo);
protected:
	void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
	void setFogVisible(bool value);

	static int mCounter;
	Ogre::Vector3 mCampos;
	Ogre::Quaternion mCamdir;
	Ogre::Viewport *mViewport;
	Ogre::StaticGeometry *mStatics;
	Ogre::Camera *mMainCam;
	Ogre::SceneManager *mSceneManager;
	Ogre::Camera *mCamera;
	Ogre::RenderTarget *mRttTex;
	Ogre::MaterialPtr mMaterial;
	Ogre::TextureUnitState* mTextureUnitState;
	RoRFrameListener *mEfl;

	void init();
};

#endif // __MAP_TEXTURE_CREATOR_H__

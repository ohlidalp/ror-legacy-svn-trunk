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
#ifndef __MapTextureCreator_H_
#define __MapTextureCreator_H_

#include "RoRPrerequisites.h"

#include "Ogre.h"

class MapTextureCreator : public Ogre::RenderTargetListener
{
public:

	MapTextureCreator(Ogre::SceneManager *smgr, Ogre::Camera *mainCam, RoRFrameListener *efl);

	Ogre::String getMaterialName();
	Ogre::String getRTName();
	
	void setCamera(Ogre::Vector3 lookAt, Ogre::Quaternion orientation);
	void setCameraMode(Ogre::PolygonMode polygonMode);
	void setCameraZoom(Ogre::Real zoom);
	void setStaticGeometry(Ogre::StaticGeometry *staticGeometry);

	void update();

protected:

	bool init();

	void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

	Ogre::Camera *mCamera;
	Ogre::Camera *mMainCam;
	Ogre::MaterialPtr mMaterial;
	Ogre::Quaternion mCamOrientation;
	Ogre::Real mCamZoom;
	Ogre::RenderTarget *mRttTex;
	Ogre::SceneManager *mSceneManager;
	Ogre::StaticGeometry *mStatics;
	Ogre::TextureUnitState* mTextureUnitState;
	Ogre::Vector3 mCamLookAt;
	Ogre::Viewport *mViewport;
	RoRFrameListener *mEfl;

	static int mCounter;
};

#endif // __MapTextureCreator_H_

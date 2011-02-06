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
#ifndef SHADOWLISTENER_H__
#define SHADOWLISTENER_H__

#include "Ogre.h"


#if OGRE_VERSION>0x010602
class SoftShadowListener : public Ogre::SceneManager::Listener
#else
class SoftShadowListener : public Ogre::ShadowListener
#endif //OGRE_VERSION
{
    // this is a callback we'll be using to set up our shadow camera
#if OGRE_VERSION>0x010602
	void shadowTextureCasterPreViewProj(Ogre::Light* light, Ogre::Camera* camera, size_t iteration);
#else
    void shadowTextureCasterPreViewProj(Ogre::Light *light, Ogre::Camera *cam);
#endif //OGRE_VERSION
    
	virtual void shadowTexturesUpdated(size_t);
	virtual void shadowTextureReceiverPreViewProj(Ogre::Light*, Ogre::Frustum*);
    virtual void preFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*);
    virtual void postFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*);
#if OGRE_VERSION>0x010602
	virtual bool sortLightsAffectingFrustum(Ogre::LightList& lightList);
#endif
};

#endif //SHADOWLISTENER_H__

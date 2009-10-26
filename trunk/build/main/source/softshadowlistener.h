#ifndef SHADOWLISTENER_H__
#define SHADOWLISTENER_H__

#include "Ogre.h"

class SoftShadowListener : public Ogre::ShadowListener
{
    // this is a callback we'll be using to set up our shadow camera
    void shadowTextureCasterPreViewProj(Ogre::Light *light, Ogre::Camera *cam);

    // these are pure virtual but we don't need them...  so just make them empty
    // otherwise we get "cannot declare of type Mgr due to missing abstract
    // functions" and so on
    void shadowTexturesUpdated(size_t);
    void shadowTextureReceiverPreViewProj(Ogre::Light*, Ogre::Frustum*);
    void preFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*);
    void postFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*);
};

#endif //SHADOWLISTENER_H__

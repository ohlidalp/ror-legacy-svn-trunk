#include "softshadowlistener.h"
#include "Ogre.h"

// this is a callback we'll be using to set up our shadow camera
void SoftShadowListener::shadowTextureCasterPreViewProj(Ogre::Light *light, Ogre::Camera *cam)
{
	// basically, here we do some forceful camera near/far clip attenuation
	// yeah.  simplistic, but it works nicely.  this is the function I was talking
	// about you ignoring above in the Mgr declaration.
	float range = light->getAttenuationRange();
	cam->setNearClipDistance(0.01);
	cam->setFarClipDistance(range);
	// we just use a small near clip so that the light doesn't "miss" anything
	// that can shadow stuff.  and the far clip is equal to the lights' range.
	// (thus, if the light only covers 15 units of objects, it can only
	// shadow 15 units - the rest of it should be attenuated away, and not rendered)
}

// these are pure virtual but we don't need them...  so just make them empty
// otherwise we get "cannot declare of type Mgr due to missing abstract
// functions" and so on
void SoftShadowListener::shadowTexturesUpdated(size_t) {}
void SoftShadowListener::shadowTextureReceiverPreViewProj(Ogre::Light*, Ogre::Frustum*) {}
void SoftShadowListener::preFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*) {}
void SoftShadowListener::postFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*) {}

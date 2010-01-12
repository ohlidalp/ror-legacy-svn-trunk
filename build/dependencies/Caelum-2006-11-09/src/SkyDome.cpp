#include "SkyDome.h"
#include "CaelumSystem.h"
#include "GeometryFactory.h"

namespace Caelum {

const Ogre::String SkyDome::mSphericDomeResourceName = "CaelumSphericDome";

SkyDome::SkyDome (Ogre::SceneManager *sceneMgr) {
	GeometryFactory::generateSphericDome (mSphericDomeResourceName, 32);
	mNode = sceneMgr->getRootSceneNode ()->createChildSceneNode ();
	Ogre::Entity *ent = sceneMgr->createEntity ("Dome", mSphericDomeResourceName);
	ent->setMaterialName (CaelumSystem::SKY_DOME_MATERIAL_NAME);
	ent->setRenderQueueGroup (Ogre::RENDER_QUEUE_SKIES_EARLY + 2);
	ent->setCastShadows (false);
	mNode->attachObject (ent);
	autoRadius=true;
}

SkyDome::~SkyDome () {
	// TODO: Detach and destroy all attached objects.
	static_cast<Ogre::SceneNode *>(mNode->getParent ())->removeAndDestroyChild (mNode->getName ());
}

void SkyDome::setSize(float size) {
	if (size>0)
	{
		mNode->setScale(Ogre::Vector3::UNIT_SCALE*size);
		autoRadius=false;
	}
	else autoRadius=true; //size will be updated at the next viewport update
}

void SkyDome::preViewportUpdate (const Ogre::RenderTargetViewportEvent &e) {
	Ogre::Camera *cam = e.source->getCamera ();
	mNode->setPosition (cam->getRealPosition ());
	if (autoRadius)
	{
		if (cam->getFarClipDistance () > 0)
			mNode->setScale (Ogre::Vector3::UNIT_SCALE * (cam->getFarClipDistance () - CAMERA_DISTANCE_MODIFIER));
		else
			mNode->setScale (Ogre::Vector3::UNIT_SCALE * (cam->getNearClipDistance () + CAMERA_DISTANCE_MODIFIER));
	}
}

} // namespace Caelum

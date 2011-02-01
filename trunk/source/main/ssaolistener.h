#ifndef SSAOLISTENER_H__
#define SSAOLISTENER_H__

#include "Ogre.h"
#include "OgreCompositorInstance.h"


class SSAOListener : public Ogre::CompositorInstance::Listener
{
protected:
	Ogre::SceneManager *mgr;
	Ogre::Camera *cam;
public:
	SSAOListener(Ogre::SceneManager *mgr, Ogre::Camera *cam);
    void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
};

#endif //SSAOLISTENER_H__

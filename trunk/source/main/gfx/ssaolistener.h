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

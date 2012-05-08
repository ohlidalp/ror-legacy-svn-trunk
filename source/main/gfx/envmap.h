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
#ifndef __EnvironmentMap_H_
#define __EnvironmentMap_H_

#include "RoRPrerequisites.h"
#include "Ogre.h"

class Envmap
{
private:
	Ogre::Camera *camera[6];
	Ogre::RenderTarget *rt[6];
	Ogre::TexturePtr texture;
	Ogre::RenderTexture* rttTex;
	Ogre::SceneNode *snode;
	int round;
	bool isDynamic;
	Ogre::SceneNode *debug_sn;

public:

	bool inited;
	Envmap(Ogre::SceneManager *mSceneMgr, Ogre::RenderWindow *mWindow, Ogre::Camera *incam, bool dynamic);
	void removeEnvMapFromTruckMaterial(Ogre::String truckMaterial);
	void addEnvMapToTruckMaterial(Ogre::String truckMaterial);
	void update(Ogre::Vector3 center, Beam *beam=0);
	void forceUpdate(Ogre::Vector3 center);
	void prepareShutdown();

};

#endif // __EnvironmentMap_H_

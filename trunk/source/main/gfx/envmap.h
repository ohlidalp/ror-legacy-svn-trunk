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
#ifndef __Envmap_H__
#define __Envmap_H__

#include "RoRPrerequisites.h"
#include "Ogre.h"


class Beam;
using namespace Ogre;

class Envmap
{
private:
	Camera *camera[6];
	RenderTarget *rt[6];
	TexturePtr texture;
	RenderTexture* rttTex;
	SceneNode *snode;
	int round;
	bool isDynamic;
	SceneNode *debug_sn;

public:

	bool inited;
	Envmap(SceneManager *mSceneMgr, RenderWindow *mWindow, Camera *incam, bool dynamic);
	void removeEnvMapFromTruckMaterial(Ogre::String truckMaterial);
	void addEnvMapToTruckMaterial(Ogre::String truckMaterial);
	void update(Vector3 center, Beam *beam=0);
	void forceUpdate(Vector3 center);
	void prepareShutdown();

};



#endif

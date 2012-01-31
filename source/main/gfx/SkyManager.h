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

#ifdef USE_CAELUM

#ifndef SKYMANAGER_H__
#define SKYMANAGER_H__

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"
#include "CaelumPrerequisites.h"
#include "Singleton.h"

class SkyManager : public RoRSingleton< SkyManager >
{
public:
	SkyManager();
	~SkyManager();

	void init(Ogre::SceneManager *mScene, Ogre::RenderWindow *mWindow, Ogre::Camera *mCamera);

	void loadScript(Ogre::String script);
	
	/// change the time scale
	void setTimeFactor(double f);
	Ogre::Light *getMainLight();
	/// gets the current time scale
	double getTimeFactor();
	
	/// prints the current time of the simulation inthe format of HH:MM:SS
	Ogre::String getPrettyTime();
	
	void forceUpdate(float dt);

	void notifyCameraChanged(Ogre::Camera *cam);
protected:
	Ogre::Camera *mCamera;
    Caelum::CaelumSystem *mCaelumSystem;
	Caelum::CaelumSystem *getCaelumSystem() { return mCaelumSystem; };
};

#endif //SKYMANAGER_H__

#endif //USE_CAELUM

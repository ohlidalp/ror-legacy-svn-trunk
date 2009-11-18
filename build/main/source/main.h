/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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

/*! @mainpage

	have fun coding :)

	please note that the documentation is work in progress
*/

#include "Ogre.h"
#include "OgreConfigFile.h"
#include "ExampleFrameListener.h"
//#include "OgrePlatformManager.h"
#include "OgreConfigDialog.h"
#ifdef HAS_EDITOR
#include "spincontrol.h"
#endif
#include "Skidmark.h"
#include "ColoredTextAreaOverlayElementFactory.h"
#include "CacheSystem.h"
#include "skinmanager.h"
#include "Settings.h"

using namespace Ogre;

//#include "FlexMesh.h"
#include "Beam.h"
#include "OgreSceneManager.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//#include <CFUserNotification.h>
#endif

// Event handler to add ability to alter curvature
class RoRFrameListener : public ExampleFrameListener
{
public:
	RoRFrameListener(RenderWindow* win, Camera* cam, SceneManager* scm, Root* root) : 
		ExampleFrameListener(win, cam, scm, root)
	{
	}

	bool frameStarted(const FrameEvent& evt)
	{
		bool ret = ExampleFrameListener::frameStarted(evt);
		return ret;
	}

};



class RigsOfRods
{
public:
	RigsOfRods();
	~RigsOfRods();
	//startup game
	void go(void);
	void shutdown(void);

	bool useogreconfig;
	bool buildmode;

protected:
	Root *mRoot;
	Camera* mCamera;
	SceneManager* mSceneMgr;
	ExampleFrameListener* mFrameListener;
	RenderWindow* mWindow;
	SoundScriptManager* ssm;
	static RigsOfRods *myinstance;

#ifdef HAS_EDITOR
	SpinControlOverlayElementFactory* spinfact;
#endif

	void loadMainResource(String name, String group);

	//creates Ogre Root
	bool setup(void);

	// create scene
	void createScene(void);

	/** Configures the application - returns false if the user chooses to abandon configuration. */
	bool configure(void);

	//resource utilities
	void exploreStreams();
	void exploreTerrains();
	void exploreVehicles();
};

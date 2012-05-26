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
#ifndef GEnv_H__
#define GEnv_H__

class RoRFrameListener;
class TerrainManager;
class Collisions;
class IHeightFinder;
class Water;
class Network;
class MapControl;
class Character;
class SkyManager;

class GlobalEnvironment
{
public:
	GlobalEnvironment() :
	      ogreRoot(0)
		, ogreViewPort(0)
		, ogreRenderWindow(0)
		, ogreCamera(0)
		, ogreSceneManager(0)
		, frameListener(0)
		, terrainManager(0)
		, collisions(0)
		, heightFinder(0)
		, water(0)
		, network(0)
		, surveyMap(0)
		, player(0)
		, sky(0)
		, embeddedMode(false)
	{
	}
	Ogre::Root *ogreRoot;
	Ogre::Viewport *ogreViewPort;
	Ogre::RenderWindow *ogreRenderWindow;
	Ogre::Camera *ogreCamera;
	Ogre::SceneManager *ogreSceneManager;

	RoRFrameListener *frameListener;
	TerrainManager *terrainManager;
	Collisions *collisions;
	IHeightFinder *heightFinder;
	Water *water;
	Network *network;
	MapControl *surveyMap;
	Character *player;
	SkyManager *sky;

	bool embeddedMode;
};

#endif // GEnv_H__

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
#ifndef TERRAINOBJECTMANAGER_H__
#define TERRAINOBJECTMANAGER_H__

#include "RoRPrerequisites.h"

class TerrainObjectManager
{
public:
	TerrainObjectManager(Ogre::SceneManager *smgr, TerrainManager *terrainManager);
	~TerrainObjectManager();


	void loadObjectConfigFile(Ogre::String filename);

protected:
	Ogre::SceneManager *mSceneMgr;
	TerrainManager *terrainManager;

	void loadObject(const char* name, float px, float py, float pz, float rx, float ry, float rz, Ogre::SceneNode * bakeNode, const char* instancename, bool enable_collisions=true, int scripthandler=-1, const char *type=0, bool uniquifyMaterial=false);
	void unloadObject(const char* name);

};
#endif // TERRAINOBJECTMANAGER_H__



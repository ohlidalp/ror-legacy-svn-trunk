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
#ifndef __TerrainObjectManager_H_
#define __TerrainObjectManager_H_

#include "RoRPrerequisites.h"


#ifdef USE_PAGED
#include "BatchPage.h"
#include "GrassLoader.h"
#include "ImpostorPage.h"
#include "PagedGeometry.h"
#include "TreeLoader2D.h"
#include "TreeLoader3D.h"
#endif //USE_PAGED
#include "IManager.h"


class TerrainObjectManager : public IManager
{
	friend class TerrainManager;
public:

	TerrainObjectManager(TerrainManager *terrainManager);
	~TerrainObjectManager();

	void loadObjectConfigFile(Ogre::String filename);


	typedef struct localizer_t
	{
		int type;
		Ogre::Vector3 position;
		Ogre::Quaternion rotation;
	} localizer_t;

	bool update(float dt);

protected:

	TerrainManager *terrainManager;

	typedef struct
	{
		Ogre::Entity *ent;
		Ogre::SceneNode *node;
		Ogre::AnimationState *anim;
		float speedfactor;
	} animated_object_t;

	Ogre::StaticGeometry *bakesg;
	ProceduralManager *proceduralManager;

	Road *road;
	Ogre::SceneNode *bakeNode;

	typedef struct
	{
		float px;
		float py;
		float pz;
		//float ry;
		Ogre::Quaternion rotation;
		char name[256];
		bool ismachine;
		bool freePosition;
	} truck_prepare_t;


	int truck_preload_num;
	truck_prepare_t truck_preload[100];


#ifdef USE_PAGED
	typedef struct
	{
		Forests::PagedGeometry *geom;
		void *loader;
	} paged_geometry_t;

	std::vector<paged_geometry_t> pagedGeometry;
	Forests::TreeLoader2D *treeLoader;
#endif //USE_PAGED

	localizer_t localizers[64];

	int objcounter;
	int free_localizer;

	std::vector<animated_object_t> animatedObjects;

	typedef struct loadedObject_t
	{
		bool enabled;
		int loadType;
		Ogre::String instanceName;
		Ogre::SceneNode *sceneNode;
		std::vector <int> collTris;
		std::vector <int> collBoxes;
	} loadedObject_t;
	std::map< std::string, loadedObject_t> loadedObjects;

	void loadObject(const char* name, float px, float py, float pz, float rx, float ry, float rz, Ogre::SceneNode * bakeNode, const char* instancename, bool enable_collisions=true, int scripthandler=-1, const char *type=0, bool uniquifyMaterial=false);
	void unloadObject(const char* name);

	void loadPreloadedTrucks();
	bool updateAnimatedObjects(float dt);
	void postLoad();

	virtual size_t getMemoryUsage();

	virtual void freeResources();

};

#endif // __TerrainObjectManager_H_

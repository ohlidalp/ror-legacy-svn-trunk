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
#ifndef __Collisions_H__
#define __Collisions_H__

#include "Ogre.h"
using namespace Ogre;
#include "OgreTextAreaOverlayElement.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif
#include <math.h>
//these are absolute maximums per terrain
#define MAX_COLLISION_BOXES 5000
#define MAX_COLLISION_TRIS 100000
//this is a power of two! Change with caution
#define HASH_SIZE 20
//how many elements per cell? power of 2 minus 2 is better
#define CELL_BLOCKSIZE 126
//how many cells in the pool? Increase in case of sparce distribution of objects
#define MAX_CELLS 10000
#define UNUSED_CELLID 0xFFFFFFFF
#define UNUSED_CELLELEMENT 0xFFFFFFFF
//terrain size is limited to 327km x 327km:
#define CELL_SIZE 10.0
#define inverse_CELL_SIZE 0.1
#define MAXIMUM_CELL 0x7FFF

#define MAX_EVENTSOURCE 500

#define LATEST_GROUND_MODEL_VERSION 3
#define LATEST_LANDUSE_VERSION 1

#define FX_NONE 0
#define FX_PARTICLE 1

typedef struct _eventsource
{
	char instancename[256];
	char boxname[256];
	SceneNode *snode;
	Quaternion direction;
	int luahandler;
	int cbox;
} eventsource_t;

#ifdef LUASCRIPT
# include "luasystem.h"
class LuaSystem;
#endif

#include "Beam.h"
#include "ExampleFrameListener.h"
#include "heightfinder.h"

typedef struct _collision_box
{
	//absolute collision box
	float lo_x;
	float hi_x;
	float lo_y;
	float hi_y;
	float lo_z;
	float hi_z;
	bool refined;
	//rotation
	Quaternion rot;
	Quaternion unrot;
	//center of rotation
	Vector3 center;
	//relative collision box
	float relo_x;
	float rehi_x;
	float relo_y;
	float rehi_y;
	float relo_z;
	float rehi_z;
	//self rotation
	bool selfrotated;
	Vector3 selfcenter;
	Quaternion selfrot;
	Quaternion selfunrot;
	int eventsourcenum;
	bool virt;
	bool camforced;
	Vector3 campos;
	int event_filter;
} collision_box_t;

typedef struct _collision_tri
{
	Vector3 a;
	Vector3 b;
	Vector3 c;
	Matrix3 forward;
	Matrix3 reverse;
	ground_model_t* gm;
	bool enabled;
} collision_tri_t;

typedef struct _cell
{
	int free;
	int element[CELL_BLOCKSIZE];
	void *next;
} cell_t;

typedef struct _hash
{
	unsigned int cellid;
	cell_t *cell;
} hash_t;

class Landusemap;

class Collisions : public MemoryAllocatedObject
{
private:
	//collision boxes pool
	collision_box_t collision_boxes[MAX_COLLISION_BOXES];
	int free_collision_box;
	//collision tris pool;
	collision_tri_t collision_tris[MAX_COLLISION_TRIS];
	int free_collision_tri;
	//collision hashtable
	hash_t hashtable[1<<HASH_SIZE];
	//cell pool
	cell_t cells[MAX_CELLS];
	int free_cell;

	eventsource_t eventsources[MAX_EVENTSOURCE];
	int free_eventsource;

	bool permitEvent(int filter);

#ifdef LUASCRIPT
	LuaSystem *lua;
#endif
	ExampleFrameListener *mefl;
	unsigned int hashmask;
	HeightFinder *hfinder;
	int collision_count;
	int largest_cellcount;
	bool debugMode;
	Landusemap *landuse;

private:
	void hash_add(int cell_x, int cell_z, int value);
	void hash_free(int cell_x, int cell_z, int value);
	cell_t *hash_find(int cell_x, int cell_z);
	unsigned int hashfunc(unsigned int cellid);
	int collisionVersion;
	std::map<Ogre::String, ground_model_t> ground_models;
	void parseGroundConfig(Ogre::ConfigFile *cfg, Ogre::String groundModel=Ogre::String());

public:
	bool forcecam;
	Vector3 forcecampos;
	ground_model_t *defaultgm, *defaultgroundgm;

	Collisions() {}; // for wrapper, DO NOT USE!

  Collisions(
#ifdef LUASCRIPT
    LuaSystem *mlua,
#endif
    ExampleFrameListener *efl, bool debugMode);

	void addCollisionBox(SceneNode *tenode, bool rotating, bool virt, float px, float py, float pz, float rx, float ry, float rz, float lx,float hx,float ly,float hy,float lz,float hz,float srx,float sry,float srz, const char* eventname, const char* instancename, bool forcecam, Vector3 campos, float scx=1.0, float scy=1.0, float scz=1.0, float drx=0.0, float dry=0.0, float drz=0.0, int event_filter=EVENT_ALL, int luahandler=-1);
	int addCollisionTri(Vector3 p1, Vector3 p2, Vector3 p3, ground_model_t* gm);
	bool collisionCorrect(Vector3 *refpos);
	bool nodeCollision(node_t *node, bool iscinecam, int contacted, float dt, float* nso, ground_model_t** ogm);
	Vector3 getPosition(char* instance, char* box);
	Quaternion getDirection(char* instance, char* box);
	collision_box_t *getBox(char* instance, char* box);
	bool isInside(Vector3 pos, char* instance, char* box, float border=0);
	bool isInside(Vector3 pos, collision_box_t *cbox, float border=0);
	bool groundCollision(node_t *node, float dt, ground_model_t** gm, float *nso=0);
	void primitiveCollision(node_t *node, Vector3 &normal, Vector3 &force, Vector3 &velocity, float dt, ground_model_t* gm, float* nso, float penetration=0, float reaction=-1.0f);
	void setHfinder(HeightFinder *hf);
	void printStats();

	int enableCollisionTri(int number, bool enable);
	int removeCollisionTri(int number);

	int createCollisionDebugVisualization();


	// ground models things
	int loadDefaultModels();
	int loadGroundModelsConfigFile(Ogre::String filename);
	std::map<Ogre::String, ground_model_t> *getGroundModels() { return &ground_models; };
	void setupLandUse(const char *configfile);
	ground_model_t *getGroundModelByString(const Ogre::String name);
	ground_model_t *last_used_ground_model;
};


#endif

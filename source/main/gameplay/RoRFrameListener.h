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
#ifndef __RoRFrameListener_H__
#define __RoRFrameListener_H__

#include "RoRPrerequisites.h"
#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif

#ifdef USE_MPLATFORM
#include "mplatform_base.h"
#endif

#include "OgreStringConverter.h"
#include "OgreException.h"
//#include "OgreTerrainSceneManager.h"
#include "OgrePixelFormat.h"
#include "CollisionTools.h"
#include <string.h>
#include <stdio.h>
#include "SkyManager.h"
#include "Beam.h"
#include "InputEngine.h"
#include "turbojet.h"
#include "Character.h"
#include "CacheSystem.h"
#include "ChatSystem.h"
#include "envmap.h"
#include "ForceFeedback.h"
#include "AppState.h"

#ifdef USE_PAGED
# include "PagedGeometry.h"
# include "GrassLoader.h"
# include "TreeLoader2D.h"
#endif

#include "OgreTerrainGroup.h"


#define CAMERA_EXT 0
#define CAMERA_FIX 1
#define CAMERA_INT 2
#define CAMERA_END 3
// the free modes are on purpose behind end, so they require a special key to be triggered
#define CAMERA_FREE 4
#define CAMERA_FREE_FIXED 5

#define CAMERA_EXTERNALCONTROL 9999


#define NONE_LOADED 0
#define TERRAIN_LOADED 1
#define ALL_LOADED 2
#define EXITING 3
#define EDITING 4
#define RELOADING 5
#define EDITOR_PAUSE 6

#define EVENT_NONE 0
#define EVENT_ALL 1
#define EVENT_AVATAR 2
#define EVENT_TRUCK 3
#define EVENT_AIRPLANE 4
#define EVENT_DELETE 5

#define DEFAULT_INTERNAL_CAM_PITCH Degree(-15)

#define MAX_PLAYLIST_ENTRIES 20

#ifdef USE_PAGED
using namespace Forests;
#endif

enum QueryFlags
{
   OBJECTS_MASK = 1<<7,
   TRUCKS_MASK  = 1<<8,
};

typedef struct
{
	float px;
	float py;
	float pz;
//	float ry;
	Quaternion rotation;
	char name[256];
	bool ismachine;
	bool freePosition;
} truck_prepare_t;

typedef struct
{
	Vector3 pos;
	Quaternion rot;
} spawn_location_t;

#ifdef USE_PAGED
typedef struct
{
	PagedGeometry *geom;
	void *loader;
} paged_geometry_t;
#endif //PAGED

typedef struct
{
	Ogre::Entity *ent;
	Ogre::SceneNode *node;
	Ogre::AnimationState *anim;
	float speedfactor;
} animated_object_t;

class RoRFrameListener: public FrameListener, public Ogre::WindowEventListener
{
	friend class BeamFactory;
	friend class Savegame;
protected:
	void initializeCompontents();
	OverlayWrapper *ow;
	int setupBenchmark();
	truck_prepare_t truck_preload[100];
	int truck_preload_num;
	localizer_t localizers[64];
	int free_localizer;
	int mSceneDetailIndex;
	Real mMoveSpeed;
	bool benchmarking;
	//    Real dirSpeed;
	Degree mRotateSpeed;

	MapControl *bigMap;
	StaticGeometry *bakesg;
	int interactivemap;
	int externalCameraMode;
	int netPointToUID;


	typedef struct loadedObject_t {
		bool enabled;
		int loadType;
		Ogre::String instanceName;
		Ogre::SceneNode *sceneNode;
		std::vector <int> collTris;
		std::vector <int> collBoxes;
	} loadedObject_t;

	std::map< std::string, loadedObject_t> loadedObjects;
	DOFManager *mDOF;
	int shaderSchemeMode;
	int inputGrabMode;
	int mouseGrabState;
	int screenWidth;
	int screenHeight;
	int truckgrabbed;
	int nodegrabbed;
	Real distgrabbed;
	bool enablePosStor;

	//GUI_Progress *gui_progress;
	TerrainGroup* mTerrainGroup;

	std::map<String, spawn_location_t> netSpawnPos;
	float truckx, trucky, truckz;
	//	SceneNode *speed_node;
	//	SceneNode *tach_node;
	//	SceneNode *roll_node;
	//	SceneNode *pitch_node;
	//	SceneNode *rollcorr_node;

	SceneManager *mSceneMgr;
	Root *mRoot;
	//	bool usejoy;
	//bool useforce;
	//BeamJoystick *joy;
	ForceFeedback *forcefeedback;

	bool flipflop;
	HDRListener *hdrListener;

	bool dirvisible;
	SceneNode *dirArrowNode;
	Vector3 dirArrowPointed;
	int objectCounter;
	float mouseGrabForce;

	SceneNode *pointerDestination;

	//SceneNode *personode;
	//AnimationState *persoanim;
	//float persoangle;
	//float persospeed;
	//float persovspeed;
	//bool perso_canjump;
	//Vector3 lastpersopos;

	void updateStats(void);
	//update engine panel
	void updateGUI(float dt);
	void updateIO(float dt);

#ifdef USE_PAGED
	std::vector<paged_geometry_t> pagedGeometry;
#endif
	String grassdensityTextureFilename;
	void initSoftShadows();
	void initHDR();

	// WindowEventListener
	void windowMoved(Ogre::RenderWindow* rw);
	void windowClosed(Ogre::RenderWindow* rw);
	void windowFocusChange(Ogre::RenderWindow* rw);
	int flaresMode;
	MapTextureCreator *mtc;
	bool updateTruckMirrors(float dt);
	void gridScreenshots(Ogre::RenderWindow* pRenderWindow, Ogre::Camera* pCamera, const int& pGridSize, const Ogre::String& path, const Ogre::String& pFileName, const Ogre::String& pFileExtention, const bool& pStitchGridImages);
	void initDust();
	Ogre::String inputhwnd;
	AppState *parentState;
public:
	// Constructor takes a RenderWindow because it uses that to determine input context
	RoRFrameListener(AppState *parent, RenderWindow* win, Camera* cam, SceneManager* scm, Root* root, bool isEmbedded=false, Ogre::String inputhwnd=0);
	virtual ~RoRFrameListener();

	int loading_state;

	Ogre::String terrainName;
	Ogre::String terrainFileName;
	Ogre::String terrainFileHash;
	Ogre::String terrainModHash;

	static RoRFrameListener *eflsingleton;
	void removeBeam(Beam *);
	
	void loadObject(const char* name, float px, float py, float pz, float rx, float ry, float rz, SceneNode * bakeNode, const char* instancename, bool enable_collisions=true, int scripthandler=-1, const char *type=0, bool uniquifyMaterial=false);
	void unloadObject(const char* name);
	bool updateEvents(float dt);
	void initTrucks(bool loadmanual, Ogre::String selected, Ogre::String selectedExtension = Ogre::String(), std::vector<Ogre::String> *truckconfig=0, bool enterTruck=false, Skin *skin=NULL);
	

	// this needs to be public so we can call it manually in embedded mode
	void windowResized(Ogre::RenderWindow* rw);

	void moveCamera(float dt);
	void hideGUI(bool visible);
	Ogre::String saveTerrainMesh();

	// Override frameStarted event to process that (don't care about frameEnded)
	bool frameStarted(const FrameEvent& evt);
	bool setCameraPositionWithCollision(Ogre::Vector3 newPos);
	bool checkForActive(int j, bool *sleepyList);
	bool frameEnded(const FrameEvent& evt);
	void showLoad(int type, char* instance, char* box);
	void showspray(bool s);
	int getNetPointToUID() { return netPointToUID; };
	void setNetPointToUID(int uid);
	void checkRemoteStreamResultsChanged();
	float netcheckGUITimer;

	Character *person;
	ChatSystem *netChat;
	static HeightFinder *hfinder;
	double getTime() {return rtime;};
	SceneManager *getSceneMgr() { return mSceneMgr; };
	RenderWindow *getRenderWindow() { return mWindow; };
	Collisions *getCollisions() { return collisions; };

	Water *getWater() { return w; };
	Camera *getCamera() { return mCamera; };
	int getLoadingState() { return loading_state; };
	void setLoadingState(int value);
	void pauseSim(bool value);
	void loadNetTerrain(char *preselected_map);
	float mapsizex, mapsizez;
	bool terrainHasTruckShop;

	void changedCurrentTruck(Beam *previousTruck, Beam *currentTruck);

	Network *getNetwork() { return net; };

	float stopTimer();
	void startTimer();
	void updateRacingGUI();

	int cameramode, lastcameramode;
	void setCameraRotation(Ogre::Radian x, Ogre::Radian y, Ogre::Real distance) { camRotX=x; camRotY=y; camDist=distance;};

	void RTSSgenerateShaders(Ogre::Entity *entity, Ogre::String normalTextureName);
	bool RTSSgenerateShadersForMaterial(Ogre::String curMaterialName, Ogre::String normalTextureName);


	void hideMap();
	void setDirectionArrow(char *text, Vector3 position);
	Ogre::Radian getCameraRotationX() { return camRotX; };
	void netDisconnectTruck(int number);

	static float getGravity() { return gravity; };
	static void setGravity(float value);

	void loadTerrain(Ogre::String terrainfile);
	void loadClassicTerrain(Ogre::String terrainfile);

	OverlayWrapper *getOverlayWrapper() { return ow; };

	Water *w;
	Ogre::String loadedTerrain;

#ifdef USE_OIS_G27
	OIS::Win32LogitechLEDs *leds;
#endif // USE_OIS_G27

public:

	void shutdown_final();

	// mutex'ed data
	void setNetQuality(int q);
	int getNetQuality(bool ack=false);
	bool getNetQualityChanged();
	bool enforceCameraFOVUpdate;
	pthread_mutex_t mutex_data;
	Radian camRotX, camRotY;
	Real camDist;

	bool freeTruckPosition;
	Vector3 reload_pos;

	void reloadCurrentTruck();

	SoundScriptManager* getSSM() { return ssm; };
private:
	int net_quality; 
	bool net_quality_changed; 


protected:

	HeatHaze *heathaze;
	bool chatlock;
	double rtime;
	float terrainxsize;
	float terrainzsize;

	//DotSceneLoader* mLoader;

	Camera* mCamera;
	Vector3 cdoppler;
	collision_box_t *reload_box;
	Quaternion reload_dir;

	RenderWindow* mWindow;
	int mStatsOn;
	bool mTruckInfoOn;
	int mapMode;
	unsigned int mNumScreenShots;
	// just to stop toggles flipping too fast
	Real mTimeUntilNextToggle ;
	bool pressure_pressed;
	SoundScriptManager* ssm;

	//camera
	Vector3 camIdealPosition;
	bool camCollided;
	Vector3 camPosColl;
	Radian pushcamRotX, pushcamRotY;
	float mMoveScale;
	Degree mRotScale;
	Vector3 lastPosition;
	//	float lastangle;
	float clutch;
	Editor *editor;

	static float gravity;

	bool netmode;
	bool initialized;
	bool isEmbedded;

	bool hidegui;
	bool debugCollisions;

	Collisions *collisions;
	MOC::CollisionTools* mCollisionTools;

	int raceStartTime;

	int objcounter;
	char terrainmap[256];

	Ogre::String terrainUID;
	Road *road;
	Dashboard *dashboard;
	FILE *editorfd;
	//	Dirt *dirt;
#ifdef HAS_EDITOR
	TruckEditor *trucked;
#endif
	int thread_mode;

	/** adds a truck to the main array
	  */
	

	Vector3 persostart;
	int joyshiftlock;

	Envmap *envmap;

	Network *net;
	ProceduralManager *proceduralManager;
	int gameStartTime;

#ifdef USE_PAGED
	Forests::TreeLoader2D *treeLoader;
#endif

//	char *preselected_map;
//	char *preselected_truck;

#ifdef USE_MPLATFORM
	MPlatform_Base *mplatform;
#endif


	RenderWindow* renderwin;

	char screenshotformat[256];
	bool useCaelumSky;
	float farclip;
	ColourValue fadeColour;

//	GUI_TruckTool *truckToolGUI;
//	GUI_MainMenu *mainmenu;

	std::vector<animated_object_t> animatedObjects;
	bool updateAnimatedObjects(float dt);
};


#endif

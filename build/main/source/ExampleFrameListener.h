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
#ifndef __ExampleFrameListener_H__
#define __ExampleFrameListener_H__

#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif
//#include "OgreKeyEvent.h"
//#include "OgreEventListeners.h"
//#include "OISEvents.h"
//#include "OISInputManager.h"
//#include "OISMouse.h"
//#include "OISKeyboard.h"
//#include "OISJoyStick.h"

#include "OgreStringConverter.h"
#include "OgreException.h"
#include "OgreTextAreaOverlayElement.h"
#include <OgrePanelOverlayElement.h>
//#include "OgreTerrainSceneManager.h"
#include "OgrePixelFormat.h"
#include <string.h>
#include <stdio.h>
#include "Caelum.h"
#include "Beam.h"
#include "InputEngine.h"
#include "turbojet.h"
#include "Character.h"
#include "envmap.h"

#ifdef PAGED
# include "PagedGeometry.h"
# include "GrassLoader.h"
# include "TreeLoader2D.h"
#endif

class DustPool;

#ifdef CLOUDS
class OgreCloudObject;
namespace simul{ namespace clouds { class CellularCloudNode; }};
#endif

class Caelum;
class MapTextureCreator;

#ifdef LUASCRIPT
class LuaSystem;
#endif

class Road;
class ProceduralManager;
class Editor;
class Water;
class HeatHaze;
class Dashboard;
#ifdef HAS_EDITOR
class TruckEditor;
#endif
class NetworkBase;
//class BeamJoystick;
class Collisions;
class Mirrors;
class Beam;
class MapControl;

//#include "dirt.h"


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

#define EVENT_ALL 0
#define EVENT_AVATAR 1
#define EVENT_TRUCK 2
#define EVENT_AIRPLANE 3

#define DEFAULT_INTERNAL_CAM_PITCH Degree(-15)

#define MAX_PLAYLIST_ENTRIES 20

// this defines the version that is used for the scripts and console stuff.
#define ROR_VERSION_STRING "0.36"

using namespace Ogre;

#ifdef PAGED
using namespace Forests;
#endif

#define LOCALIZER_VERTICAL 0
#define LOCALIZER_HORIZONTAL 1
#define LOCALIZER_NDB 2
#define LOCALIZER_VOR 3

typedef struct
{
	int type;
	Vector3 position;
	Quaternion rotation;
} localizer_t;

typedef struct
{
	float px;
	float py;
	float pz;
//	float ry;
	Quaternion rotation;
	char name[256];
	bool ismachine;
} truck_prepare_t;

typedef struct
{
	Vector3 pos;
	Quaternion rot;
} spawn_location_t;


typedef struct
{
	Ogre::Entity *ent;
	Ogre::SceneNode *node;
	Ogre::AnimationState *anim;
	float speedfactor;
} animated_object_t;

class ExampleFrameListener: public FrameListener, public Ogre::WindowEventListener
{
protected:
	truck_prepare_t truck_preload[100];
	int truck_preload_num;
	localizer_t localizers[64];
	int free_localizer;
	int mSceneDetailIndex;
	Real mMoveSpeed;
	//    Real dirSpeed;
	Degree mRotateSpeed;
	Overlay* directionOverlay;
	Overlay* mDebugOverlay;
	Overlay* mTimingDebugOverlay;
	Overlay* dashboardOverlay;
	Overlay* machinedashboardOverlay;
	Overlay* airdashboardOverlay;
	Overlay* boatdashboardOverlay;
	Overlay* needlesOverlay;
	Overlay* airneedlesOverlay;
	Overlay* boatneedlesOverlay;
	Overlay* needlesMaskOverlay;
	//Overlay* bigMapOverlay;

	MapControl *bigMap;
	StaticGeometry *bakesg;
	bool interactivemap;
    bool xfire_enabled;
	int externalCameraMode;

	Overlay* playerListOverlay;
	//Overlay* bigMapOverlayText;
	//OverlayElement *bigmapotext;
	Overlay* pressureOverlay;
	Overlay* pressureNeedleOverlay;
	Overlay* editorOverlay;
#ifdef HAS_EDITOR
	Overlay* truckeditorOverlay;
#endif
	Overlay* mouseOverlay;
	Overlay* flashOverlay;
	//    Beam *beam;
	int loading_state;
	OverlayElement* guiGear;
	OverlayElement* guiGear3D;
	OverlayElement* guiRoll;
	TextAreaOverlayElement* guiAuto[5];
	TextAreaOverlayElement* guiAuto3D[5];
	OverlayElement* guipedclutch;
	OverlayElement* guipedbrake;
	OverlayElement* guipedacc;
	TextAreaOverlayElement* laptimemin;
	TextAreaOverlayElement* laptimes;
	TextAreaOverlayElement* laptimems;
	TextAreaOverlayElement* lasttime;
	TextAreaOverlayElement* flashMessageTE;

	TextAreaOverlayElement* directionArrowText;
	TextAreaOverlayElement* directionArrowDistance;

	TextAreaOverlayElement* alt_value_taoe;
	TextAreaOverlayElement* boat_depth_value_taoe;

	OverlayElement* mouseElement;
	int mouseX;
	int mouseY;
	int inputGrabMode;
	int mouseGrabState;
	bool switchMouseButtons;
	int screenWidth;
	int screenHeight;
	bool isnodegrabbed;
	int truckgrabbed;
	int nodegrabbed;
	Real distgrabbed;

	//GUI_Progress *gui_progress;

	float thrtop;
	float thrheight;
	float throffset;

	std::map<String, spawn_location_t> netSpawnPos;
	float truckx, trucky, truckz;
	//	SceneNode *speed_node;
	//	SceneNode *tach_node;
	//	SceneNode *roll_node;
	//	SceneNode *pitch_node;
	//	SceneNode *rollcorr_node;
	/*
	OverlayElement *mapdot[MAX_ARROW];
	OverlayElement *mapo;
	OverlayElement *airmapdot[MAX_ARROW];
	OverlayElement *airmapo;
	OverlayElement *bigmapdot[MAX_ARROW];
	OverlayElement *bigmapo;
	OverlayElement *boatmapdot;
	OverlayElement *boatmapo;
	*/

	OverlayElement *pbrakeo;
	OverlayElement *lockedo;
	OverlayElement *securedo;
	OverlayElement *lopresso;
	OverlayElement *clutcho;
	OverlayElement *lightso;
	OverlayElement *batto;
	OverlayElement *igno;

	OverlayElement *thro1;
	OverlayElement *thro2;
	OverlayElement *thro3;
	OverlayElement *thro4;
	OverlayElement *engfireo1;
	OverlayElement *engfireo2;
	OverlayElement *engfireo3;
	OverlayElement *engfireo4;
	OverlayElement *engstarto1;
	OverlayElement *engstarto2;
	OverlayElement *engstarto3;
	OverlayElement *engstarto4;

	OverlayElement *bthro1;
	OverlayElement *bthro2;

	TextureUnitState *adibugstexture;
	TextureUnitState *aditapetexture;

	TextureUnitState *hsirosetexture;
	TextureUnitState *hsibugtexture;
	TextureUnitState *hsivtexture;
	TextureUnitState *hsihtexture;

	//TextureUnitState *arrowtexture[MAX_ARROW];
	TextureUnitState *speedotexture;
	TextureUnitState *tachotexture;
	TextureUnitState *rolltexture;
	TextureUnitState *pitchtexture;
	TextureUnitState *rollcortexture;
	TextureUnitState *turbotexture;

	TextureUnitState *airspeedtexture;
	TextureUnitState *altimetertexture;
	TextureUnitState *vvitexture;
	TextureUnitState *aoatexture;

	TextureUnitState *boatspeedtexture;
	TextureUnitState *boatsteertexture;

	TextureUnitState *pressuretexture;

	TextureUnitState *airrpm1texture;
	TextureUnitState *airrpm2texture;
	TextureUnitState *airrpm3texture;
	TextureUnitState *airrpm4texture;

	TextureUnitState *airpitch1texture;
	TextureUnitState *airpitch2texture;
	TextureUnitState *airpitch3texture;
	TextureUnitState *airpitch4texture;

	TextureUnitState *airtorque1texture;
	TextureUnitState *airtorque2texture;
	TextureUnitState *airtorque3texture;
	TextureUnitState *airtorque4texture;

	SceneManager *mSceneMgr;
	Root *mRoot;
	//	bool usejoy;
	//bool useforce;
	//BeamJoystick *joy;
	bool flipflop;

	bool dirvisible;
	SceneNode *dirArrowNode;
	Vector3 dirArrowPointed;

	SceneNode *pointerDestination;

	Character *person;
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
	//void moveMapDot(int n, float x, float y);
	//void moveAirMapDot(int n, float x, float y);
	//void moveBoatMapDot(float x, float y);
	//void moveBigMapDot(int n, float x, float y, int trucknum, int truckstate=0);
	void resizePanel(OverlayElement *oe, RenderWindow *win);
	void reposPanel(OverlayElement *oe, RenderWindow *win);
	void placeNeedle(RenderWindow* win, SceneNode *node, float x, float y, float len);

#ifdef PAGED
	PagedGeometry *grass;
	PagedGeometry *trees;
	GrassLoader *grassLoader;
#endif
	String grassdensityTextureFilename;
	void setGrassDensity(float x, float y, int density, bool relative=false);
	void saveGrassDensity();
	int changeGrassBuffer(unsigned char *data, int relchange);
	void updateGrass(Vector3 pos);

	// WindowEventListener
	void windowMoved(Ogre::RenderWindow* rw);
	void windowResized(Ogre::RenderWindow* rw);
	void windowClosed(Ogre::RenderWindow* rw);
	void windowFocusChange(Ogre::RenderWindow* rw);
	int flaresMode;
	MapTextureCreator *mtc;
	int fogmode;
	float fogdensity;
	ManualObject *pickLine;
	SceneNode *pickLineNode;
	bool updateTruckMirrors(float dt);
public:
	// Constructor takes a RenderWindow because it uses that to determine input context
	ExampleFrameListener(RenderWindow* win, Camera* cam, SceneManager* scm, Root* root);
	virtual ~ExampleFrameListener();

	void loadObject(char* name, float px, float py, float pz, float rx, float ry, float rz, SceneNode * bakeNode, char* instancename, bool enable_collisions=true, int luahandler=-1, char *type=0);
	void repairTruck(char* inst, char* box);
	bool updateEvents(float dt);
	void initTrucks(bool loadmanual, Ogre::String selected, Ogre::String selectedExtension = Ogre::String(), std::vector<Ogre::String> *truckconfig=0);
	void setCurrentTruck(int v);
	//bool processUnbufferedMouseInput(const FrameEvent& evt);
	void moveCamera(float dt);
	void showDebugOverlay(bool show, bool showmat=false);
#ifdef HAS_EDITOR
	void showTruckEditorOverlay(bool show);
#endif
	//void showBigMap(bool show);
	void showPressureOverlay(bool show);
	void showEditorOverlay(bool show);
	void showDashboardOverlays(bool show, int mode);
	// Override frameStarted event to process that (don't care about frameEnded)
	bool frameStarted(const FrameEvent& evt);
	void recursiveActivation(int j);
	bool checkForActive(int j, bool *sleepyList);
	void flashMessage(Ogre::String txt, float time=1, float charHeight=-1);
	void flashMessage(char* txt, float time=1, float charHeight=-1);
	void flashMessage();
	int getFogMode() { return fogmode; };
	float getFogDensity() { return fogdensity; };
	bool frameEnded(const FrameEvent& evt);
	void showLoad(int type, char* instance, char* box);
	void getMeshInformation(Mesh* mesh,size_t &vertex_count,Vector3* &vertices,
			size_t &index_count, unsigned* &indices,
			const Vector3 &position = Vector3::ZERO,
			const Quaternion &orient = Quaternion::IDENTITY,const Vector3 &scale = Vector3::UNIT_SCALE);

	bool getSwitchButtons() { return switchMouseButtons; };
	static HeightFinder *hfinder;
	double getTime() {return rtime;};
	SceneManager *getSceneMgr() { return mSceneMgr; };
	RenderWindow *getRenderWindow() { return mWindow; };
	Collisions *getCollisions() { return collisions; };
	int addTruck(char* fname, Vector3 pos);
	Beam *getTruck(int number) { return trucks[number]; };
	int getCurrentTruckNumber() { return current_truck; };
	int getTruckCount() { return free_truck; };
	Beam *getCurrentTruck() { return trucks[current_truck]; };
	Water *getWater() { return w; };
	Camera *getCamera() { return mCamera; };
	//SceneNode *getPersonNode() { return personode; };
	//OIS::Mouse *getMouse() { return  mMouse; };
	int getLoadingState() { return loading_state; };
	void pauseSim(bool value);
	void loadNetTerrain(char *preselected_map);

	float mapsizex, mapsizez;

	float stopTimer();
	void startTimer();
	void updateRacingGUI();

	TextAreaOverlayElement *playerlistOverlay[MAX_PLAYLIST_ENTRIES];

	int cameramode;
	void setCameraRotation(Ogre::Radian x, Ogre::Radian y, Ogre::Real distance) { camRotX=x; camRotY=y; camDist=distance;};
	void hideMap();
	void setDirectionArrow(char *text, Vector3 position);
	Ogre::Radian getCameraRotationX() { return camRotX; };
	void netDisconnectTruck(int number);

	caelum::SkyColourModel *getCaelumModel() { return mCaelumModel; };
	caelum::CaelumSystem *getCaelumSystem() { return mCaelumSystem; };

	static float getGravity() { return gravity; };
	
	Ogre::Ray getMouseRay();
	void shutdown_pre();
	void shutdown_final();
protected:

	HeatHaze *heathaze;
	bool chatting;
	bool chatlock;
	double rtime;
	float terrainxsize;
	float terrainzsize;


	// Caelum system
	caelum::CaelumSystem *mCaelumSystem;
	// Caelum model
	caelum::SkyColourModel *mCaelumModel;
	//caelum maps?

	//OIS::InputManager* mInputManager;
	//OIS::Mouse* mMouse;
	Camera* mCamera;
	Vector3 cdoppler;
	Vector3 reload_pos;
	collision_box_t *reload_box;
	Quaternion reload_dir;

	RenderWindow* mWindow;
	bool mStatsOn;
	bool mTruckInfoOn;
	int mapMode;
	unsigned int mNumScreenShots;

	DustPool *dustp;
	DustPool *dripp;
	DustPool *clumpp;
	DustPool *sparksp;
	DustPool *splashp;
	DustPool *ripplep;
	// just to stop toggles flipping too fast
	Real mTimeUntilNextToggle ;
	bool pressure_pressed;
	SoundScriptManager* ssm;

	//camera
	Vector3 camIdealPosition;
	Radian camRotX, camRotY;
	Radian pushcamRotX, pushcamRotY;
	Real camDist;
	float mMoveScale;
	Degree mRotScale;
	Vector3 lastPosition;
	//	float lastangle;
	float clutch;
	Editor *editor;
	int showcredits;
	float creditsviewtime;
	static float gravity;

	bool netmode;
	bool initialized;

	bool hidegui;
	bool debugCollisions;

#ifdef LUASCRIPT
	LuaSystem *lua;
#endif

	Collisions *collisions;

	int raceStartTime;

	int objcounter;
	char terrainmap[256];
	Road *road;
	Water *w;
	Mirrors *mirror;
	Dashboard *dashboard;
	FILE *editorfd;
	//	Dirt *dirt;
#ifdef HAS_EDITOR
	TruckEditor *trucked;
#endif
	Beam *trucks[MAX_TRUCKS];
	int free_truck;
	int current_truck;

	Vector3 persostart;
	float timeUntilUnflash;
	int joyshiftlock;

	Envmap *envmap;

	NetworkBase *net;
	ProceduralManager *proceduralManager;

	
	void updateXFire();
	int gameStartTime;

#ifdef PAGED
	Forests::TreeLoader2D *treeLoader;
#endif

//	char *preselected_map;
//	char *preselected_truck;

	void loadTerrain(Ogre::String terrainfile);
	static bool fileExists(char* filename);
	void processConsoleInput();

	RenderWindow* renderwin;

	char screenshotformat[255];
	bool useIngameEditor;
	float globalFogDensity;
	bool useCaelumSky;
	float farclip;
	ColourValue fadeColour;

//	GUI_TruckTool *truckToolGUI;
//	GUI_MainMenu *mainmenu;

	Ogre::String loadedTerrain;
	
	std::vector<animated_object_t> animatedObjects;
	bool updateAnimatedObjects(float dt);
#ifdef CLOUDS
	OgreCloudObject *cloud;
	simul::clouds::CellularCloudNode *ci;
#endif

};


#endif

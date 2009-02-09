/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#ifndef __TruckEditorElement_H__
#define __TruckEditorElement_H__

#include <stdio.h>
#include <math.h>

#include "Ogre.h"
using namespace Ogre;

#include "DynamicLines.h"

#define SELECT_NEW 0
#define SELECT_ADD 1
#define SELECT_DEL 2

#define STAB_NONE 0
#define STAB_DIRECT 1
#define STAB_REVERSE 2

#define SEL_FALSE 0
#define SEL_TRUE 1
#define SEL_MIXED 2

#define SELECTMODE_NODE 0
#define SELECTMODE_BEAM 1

#define DEFAULT_TIE_LENGTH 2.0
#define DEFAULT_TIE_RATE 0.5
#define DEFAULT_TIE_STOP 0.1

#define EBEAM_STRUCTURAL 0
#define EBEAM_ROPE 1
#define EBEAM_HYDRO 2
#define EBEAM_SHOCK 3
#define EBEAM_COMMAND 4
#define EBEAM_MIXED 5

typedef struct _editortri
{
	int id1;
	int id2;
	int id3;
	Vector2 tcoord1;
	Vector2 tcoord2;
	Vector2 tcoord3;
	char type;
	bool backfaced;
	bool exported;
} editortri_t;


typedef struct _editornode
{
    Vector3 Position;
	char mode;
	bool fixed;
	bool contacter;
	bool ropable;

	bool hasTie;
	float tieMaxl;
	float tieRate;
	float tieShort;
	float tieLong;
	char tieMode;

	bool hasVisual;
	BillboardSet *bboard;
	BillboardSet *selectedbboard;
	SceneNode *snode;
	int guid;

} editornode_t;

typedef struct _editorbeam
{
    int id1;
	int id2;
	char mode;

	bool isShock;
	//this is protected by isShock
	float s;
	float d;
	float sbound;
	float lbound;
	float precomp;
	
	bool isHydro;
	float hydroRatio;

	bool isCommand;
	float commandRate;
	float commandShort;
	float commandLong;
	int commandKeyS;
	int commandKeyL;

	bool isRope;

	bool hasVisual;
	DynamicLines *dline;
	SceneNode *snode;
	int guid;
} editorbeam_t;

typedef struct _editorwheel
{
	float radius;
	float width;
	int numrays;
	int id1;
	int id2;
	int idstab;
	int stabmode;
	bool braked;
	bool propulsed;
	int idtorque;
	float mass;
	float spring;
	float damp;
	char facemat[256];
	char bandmat[256];
} editorwheel_t;

typedef struct _editorprop
{
	int refid;
	int xid;
	int yid;
	float offsetx;
	float offsety;

	bool isProp;
	float offsetz;
	float rotx;
	float roty;
	float rotz;
	char meshname[256];
} editorprop_t;

class TruckEditorElement
{
public:
	SceneManager *sceneman;
	float snapgrid;
	int guid;

	char description[1024];
	bool forwardcommands;
	bool importcommands;
	bool rollon;
	bool rescuer;
	editornode_t nodes[1000];
	int free_node;
	editorbeam_t beams[4000];
	int free_beam;
	editorwheel_t wheels[16];
	int free_wheel;
	editortri_t tris[4000];
	int free_tri;
	float totalmass;
	float cargomass;
	char cabmat[256];
	char helpmat[256];

	int free_selected_node;
	int selected_nodes[1000];

	int free_selected_beam;
	int selected_beams[1000];

	bool has_cam;
	int cam_posid;
	int cam_dirid;
	int cam_refid;

	bool has_engine;
	float minrpm, maxrpm, torque, dratio, rear;
	float gears[16];

	bool has_cinecam;
	Vector3 cinecamPos;
	int cinecamIds[8];

	editorprop_t props[40];
	int free_prop;

	int selectionmode;

	bool has_globeam;
	float globeam_deform;
	float globeam_break;
	float globeam_beam_diameter;
	char globam_beam_material[256];

public:
	bool dirty;

	TruckEditorElement(SceneManager *scm, float snap);
	int genGUID();
	void clear(bool clearVisuals);
	void deleteTri(int id);
	void deleteBeam(int id);
	void deleteNode(int id);
	int load(char* truckname);
	int save(char* truckname);
	void updateVisuals();
	bool isSelected(int id);
	bool isSelectedBeam(int id);
	void node2beamSelection();
	void beam2nodeSelection();
	void unselect(int id);
	void unselectBeam(int id);
	void switchSelectionMode(int mode);
	//start and end must be ordered and returns how many nodes affected by this operation
	int selectNodes(Vector3 start, Vector3 stop, int selectmode);
	//start and end must be ordered and returns how many beams affected by this operation
	int selectBeams(Vector3 start, Vector3 stop, int selectmode);
	int createNode(Vector3 position);
	int create_beam(int id1, int id2);
	void deleteSelectedNodes();
	void deleteSelectedBeams();
	//classic click-happend
	void appendNode(Vector3 position, int context);
	bool isDraggable(Vector3 pos, int c, float tolerance);
	void moveSelected(Vector3 disp);
	void setSelectedNodesMode(char mode);
	void setSelectedNodesFixed(int mode);
	int  getSelectedNodesFixed();
	void setSelectedNodesContacter(int mode);
	int  getSelectedNodesContacter();
	void setSelectedNodesRopable(int mode);
	int  getSelectedNodesRopable();
	void setSelectedNodesTie(int mode);
	int  getSelectedNodesTie();
	int  getSelectedNodesMode(char mode);
	float getSelectedNodesTieLength();
	void setSelectedNodesTieLength(float v);
	float getSelectedNodesTieRate();
	void setSelectedNodesTieRate(float v);
	float getSelectedNodesTieStop();
	void setSelectedNodesTieStop(float v);

	void setSelectedBeamsHydroRate(float v);
	float getSelectedBeamsHydroRate();
	void setSelectedBeamsShockK(float v);
	float getSelectedBeamsShockK();
	void setSelectedBeamsShockD(float v);
	float getSelectedBeamsShockD();
	void setSelectedBeamsShockShort(float v);
	float getSelectedBeamsShockShort();
	void setSelectedBeamsShockLong(float v) ;
	float getSelectedBeamsShockLong() ;
	void setSelectedBeamsShockPrecomp(float v);
	float getSelectedBeamsShockPrecomp();
	void setSelectedBeamsCommandRate(float v);
	float getSelectedBeamsCommandRate();
	void setSelectedBeamsCommandShortBound(float v) ;
	float getSelectedBeamsCommandShortBound() ;
	void setSelectedBeamsCommandShortKey(float v) ;
	float getSelectedBeamsCommandShortKey() ;
	void setSelectedBeamsCommandLongBound(float v);
	float getSelectedBeamsCommandLongBound();
	void setSelectedBeamsCommandLongKey(float v);
	float getSelectedBeamsCommandLongKey();

	int getSelectedBeamsStructuralVisible();
	void setSelectedBeamsStructuralVisible(bool v);
	void setSelectedBeamsMode(int mode);
	int getBeamMode(int id);
	int getSelectedBeamsMode();
	int numSelected();
	int numBeamSelected();

	void dedupe();
	~TruckEditorElement();
};

#endif

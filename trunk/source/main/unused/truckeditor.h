/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#ifndef __TruckEditor_H__
#define __TruckEditor_H__

#include <stdio.h>
#include <math.h>

#include "Ogre.h"
#include "OgreBorderPanelOverlayElement.h"
using namespace Ogre;
#include "OISEvents.h"
#include "OISInputManager.h"
#include "OISMouse.h"
#include "OISKeyboard.h"

#include "spincontrol.h"
#include "truckeditorelement.h"

//WTF?
#define round(x) ((x)>=0?(int)((x)+0.5):(int)((x)-0.5))

#define CAMERA_X  0
#define CAMERA_Y  1
#define CAMERA_Z  2
#define CAMERA_3D 3

#define DRAG_NONE 0
#define DRAG_VIEW 1
#define DRAG_SELECT 2
#define DRAG_CURSOR_X 3
#define DRAG_CURSOR_Y 4
#define DRAG_CURSOR_Z 5
#define DRAG_SELNODES 7
#define DRAG_SELECTBEAM 8

#define TOOL_MOVE 0
#define TOOL_SELECT 1
#define TOOL_SELECTBEAM 2
#define TOOL_BUILD 3

#define CTX_NONE 0
#define CTX_NODE 1
#define CTX_BEAM 2


class TruckEditor
{
protected:
	SceneNode *baseNode;
	int toolMode;
	Vector3 selectstart;
	Vector3 dragstart;

	SceneManager *sceneman;
	Camera *cams[4];
	RenderTexture* rttTexes[4];
	float posx[4];
	float posy[4];
	float screenscale[4];
	float screencenterx[4];
	float screencentery[4];
	OverlayElement* mouseElement;
	int mouseX;
	int mouseY;
	int lastmX;
	int lastmY;
	int lastclick;
	int dragAction;
	int dragContext;
	int screenWidth;
	int screenHeight;
	Overlay *screenOverlay;
	Overlay *openOverlay;
	Overlay *modalOverlay;
	Overlay *nodeContextOverlay;
	Overlay *beamContextOverlay;
	Overlay *contextOverlay;

	Overlay *selectionOverlay;
	OverlayElement* selectionElement;
	int selectionBoxX;
	int selectionBoxY;

	OverlayElement *lastActive;
	TextAreaOverlayElement* helpTextOE;
	char currentName[256];
	TruckEditorElement *truck;
	float snapgrid;
//	BillboardChain *beamsChain;

	char openfiles[64][64];
	int free_openfile;
	int openfile_offset;

public:
	bool dirty;

	TruckEditor(Overlay *screen, int width, int height);
	void createGrid();
int updateOpenFiles();

	void updateOpenFilesVisu();
	void showOpen(bool s);
	void selectScroll(int v);
	void selectOpen(char *ename);
	char *getLoadPointedTruck();
	void showSave();
	void showEngine();
	bool mouseClick(int x, int y, int button, OIS::Keyboard *modifier);
	void updateContextOverlay();
	void setRadioBitmap(char *name, int value);
	void setCheckBitmap(char *name, int value);
	void setTool(int ttype);
	void setContext(int ctx);
	void mouseZoom(int x, int y, int zoom);
	void mouseHover(int x, int y);
	void setHelpMessage(const char* msg);
	bool processInput(OIS::Mouse *mInputReader, OIS::Keyboard *modifier);
	void clearAll();
	void loadTruck(char* truckname);
	void refresh();
	void zoomView(int c, int zoom);
	void clickView(int c, int x, int y, int button, OIS::Keyboard *modifier);
	Vector2 convertMouse2World(int c, Vector2 mousepos);
	Vector3 convertMouse3World(int c, Vector2 mousepos, bool snap);
	void showInfos(int c, int x, int y);
	void mouseDrag(int dx, int dy, int button, int x, int y);
	void mouseDragFinish(int x,int y, int button, OIS::Keyboard *modifier);
	void selectionBoxBegin(int x, int y);
	void selectionBoxUpdate(int x, int y);
	void selectionBoxEnd();
	~TruckEditor();
};

#endif

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

#ifndef MAPCONTROL_H_
#define MAPCONTROL_H_

//#define OGREPLUGIN

#include <Ogre.h>
#include "OgreTextAreaOverlayElement.h"
#include <vector>

#define MAX_MAP_ENTITYSTATES 4

enum {MAP_DYNAMIC_ACTIVE, MAP_DYNAMIC_INACTIVE, MAP_DYNAMIC_NETWORKED};

class MapControl;
class MapEntity;
class MapTextureCreator;
class ExampleFrameListener;

class MapTextureCreator : public Ogre::RenderTargetListener
{
public:
	MapTextureCreator(Ogre::SceneManager *smgr, Ogre::Camera *mainCam, ExampleFrameListener *efl);
	Ogre::String getMaterialName();
	Ogre::String getRTName();
	void setAutoUpdated(bool value);
	void update();
	void setCamPosition(Ogre::Vector3 pos, Ogre::Quaternion direction);
	void setCameraMode(Ogre::PolygonMode pm);
	void setStaticGeometry(Ogre::StaticGeometry *geo);
	void setCamZoom(float newzoom);
	void setCamZoomRel(float zoomdelta);
	void setTranlucency(float amount);
protected:
	void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
	void setFogVisible(bool value);
	static int counter;
	float zoom;
	Ogre::Vector3 campos;
	Ogre::Quaternion camdir;
	Ogre::Viewport *v;
	Ogre::StaticGeometry *statics;
	Ogre::Camera *mainCam;
	Ogre::SceneManager *smgr;
	Ogre::Camera *mCamera;
	Ogre::RenderTarget *rttTex;
	Ogre::MaterialPtr mat;
	Ogre::TextureUnitState* tex;
	ExampleFrameListener *mefl;
	void init();
};

class MapEntity
{
public:
	MapEntity(MapControl *ctrl, int uid, Ogre::String type, Ogre::OverlayContainer* parent);
	~MapEntity();
	void setPosition(Ogre::Vector3 pos);
	void setPosition(float x, float z);
	void setRotation(Ogre::Quaternion q);
	void setRotation(float r);
	void setRotation(Ogre::Radian r);
	bool getVisibility();
	void setVisibility(bool value);
	void onTop();

	void setState(int state);
	int getState();

	float getScale() { return scale; };
	void setScale(float value) { scale=value; };

	float getMinSize() { return minsize; };
	void setMinSize(float value) { minsize=value; };

	static Ogre::String entityStates[MAX_MAP_ENTITYSTATES];
	void update();
	void setDescription(Ogre::String s);
	Ogre::String getDescription();

protected:
	Ogre::OverlayContainer *container;
	Ogre::TextAreaOverlayElement *taoe;
	Ogre::OverlayContainer *parent;
	Ogre::String myType;
	Ogre::String myDescription;
	Ogre::MaterialPtr myMaterials[MAX_MAP_ENTITYSTATES];
	Ogre::Real x, z, r;
	MapControl *mapCtrl;
	int uid;
	int myState;
	float scale;
	float minsize;
	bool isStatic;
	void init();
};

class MapControl
{
public:
	MapControl(int mapsizex, int mapsizez);
	~MapControl();
	MapEntity *createNamedMapEntity(Ogre::String name, Ogre::String type);
	MapEntity *createMapEntity(Ogre::String type);
	MapEntity *getEntityByName(Ogre::String name);
	void deleteMapEntity(MapEntity *);
	bool getVisibility();
	void setVisibility(bool value);
	void setBackground(Ogre::String texName);
	void setBackgroundMaterial(Ogre::String matName);

	void setPosition(float x, float y, float w, float h, Ogre::RenderWindow* rw=0);
	void setPosition(float x, float y, float w, Ogre::RenderWindow* rw=0);
	void resizeToScreenRatio(Ogre::RenderWindow* win);

	void updateRenderMetrics(Ogre::RenderWindow* win);
	// DO NOT USE THIS DIRECTLY!
	void setEntityPosition(float x, float y, float w, float h, float minsize, Ogre::OverlayContainer *e);
	static Ogre::String getTypeByDriveable(int driveable);
	float getAlpha() { return alpha; };
	void setAlpha(float value);
	void setEntityVisibility(bool value);
	void setWorldSize(int x, int z);
protected:
	int uniqueCounter;
	bool bgInitiated;
	Ogre::MaterialPtr bgMat;
	static int mapcounter;
	Ogre::Overlay *mainOverlay;
	Ogre::OverlayContainer *container;
	std::vector<MapEntity *> mapEntities;
	std::map<Ogre::String, MapEntity *> namedEntities;
	int mapsizex, mapsizez;
	float x,y,w,h;
	float alpha;
	unsigned int rWinWidth, rWinHeight, rWinDepth;
	int rWinLeft, rWinTop;

	void updateEntityPositions();
};


#endif

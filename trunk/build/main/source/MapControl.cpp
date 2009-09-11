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

#include "MapControl.h"
#include "Beam.h"

#ifdef OGREPLUGIN
#include "OgreTerrainSceneManager.h"
#endif

#include "ExampleFrameListener.h"
#include "water.h"
#include "ResourceBuffer.h"

using namespace Ogre;

int MapTextureCreator::counter=0;
MapTextureCreator::MapTextureCreator(SceneManager *mgr, Ogre::Camera *maincam, ExampleFrameListener *efl)
{
	mefl=efl;
	mainCam = maincam;
	counter++;
	smgr=mgr;
	statics=0;
	camdir=Quaternion::ZERO;
	init();
}

void MapTextureCreator::init()
{
	TexturePtr texture = TextureManager::getSingleton().createManual("MapRttTex"+StringConverter::toString(counter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 1024, 1024, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
	rttTex = texture->getBuffer()->getRenderTarget();
	rttTex->setAutoUpdated(true);
	mCamera = smgr->createCamera("MapRenderCam");

	v = rttTex->addViewport(mCamera);
	v->setBackgroundColour(ColourValue::Black);
	v->setOverlaysEnabled(false);

	mat = MaterialManager::getSingleton().create("MapRttMat"+StringConverter::toString(counter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	tex = mat->getTechnique(0)->getPass(0)->createTextureUnitState("MapRttTex"+StringConverter::toString(counter));

	rttTex->addListener(this);
	//mCamera->setProjectionType(PT_ORTHOGRAPHIC);

	zoom = 3;

	mCamera->setPosition(0, 4500, 0.0001);
	mCamera->lookAt(0,0,0);

	mCamera->setFarClipDistance(0);
	mCamera->setAspectRatio(1.0);
	mCamera->setFixedYawAxis(false);
	mCamera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
	mCamera->setFOVy(Radian(Math::HALF_PI));
	mCamera->setNearClipDistance(zoom);
}

void MapTextureCreator::setTranlucency(float amount)
{
	//tex->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_MANUAL, 1.0, amount);
}

void MapTextureCreator::setCameraMode(Ogre::PolygonMode pm)
{
	mCamera->setPolygonMode(pm);
}

void MapTextureCreator::setStaticGeometry(Ogre::StaticGeometry *geo)
{
	statics = geo;
}


void MapTextureCreator::setCamZoomRel(float zoomdelta)
{
	zoom += zoomdelta * zoom/100;
}

void MapTextureCreator::setCamZoom(float newzoom)
{
	zoom = newzoom;
}

void MapTextureCreator::setCamPosition(Vector3 pos, Quaternion direction)
{
	campos = pos;
	camdir = direction;
}


void MapTextureCreator::update()
{
	// 1 = max out = total overview
	if(zoom<=0.3)
		zoom=0.3;

	float width = mefl->mapsizex;
	float height = mefl->mapsizez;
	float zoomfactor = zoom * ((width+height)/2) * 0.002;

	//LogManager::getSingleton().logMessage(StringConverter::toString(zoom));
	mCamera->setNearClipDistance(zoom);
	mCamera->setPosition(campos + Vector3(0, zoomfactor, 0));
	if(camdir != Quaternion::ZERO) mCamera->setOrientation(camdir);
	mCamera->lookAt(campos - Vector3(0, zoomfactor, 0));

	// output the zoom factor for debug purposes
	//LogManager::getSingleton().logMessage(StringConverter::toString(zoom));

	/*
	// this is bugged, so deactivated for now
	float f = 50-zoom;
	if(f<20)
		f=20;
	for (int i=0; i<mefl->getTruckCount(); i++)
		mefl->getTruck(i)->preMapLabelRenderUpdate(true, f);
	*/

	if(statics)
		statics->setRenderingDistance(0);
	// thats a huge workaround to be able to not use the normal LOD

	setFogVisible(false);

	rttTex->update();

	setFogVisible(true);
	if(mefl->getWater())
		mefl->getWater()->setVisible(false);
	if(statics)
		statics->setRenderingDistance(1000);
	if(mefl->getWater())
		mefl->getWater()->setVisible(true);

	/*
	// deactivated for the moment
	for (int i=0; i<mefl->getTruckCount(); i++)
		mefl->getTruck(i)->preMapLabelRenderUpdate(false);
	*/

}

void MapTextureCreator::setFogVisible(bool value)
{
	int fogmode = mefl->getFogMode();
	//LogManager::getSingleton().logMessage("fogswitch: "+StringConverter::toString(fogmode)+" / "+StringConverter::toString(value));
	if(!fogmode || fogmode == 2)
		return;

	// this refuses to work, somehow:
	if(fogmode == 1)
	{
		if(value)
			static_cast<caelum::StoredImageSkyColourModel *>(mefl->getCaelumModel())->setFogDensity(mefl->getFogDensity());
		else
			static_cast<caelum::StoredImageSkyColourModel *>(mefl->getCaelumModel())->setFogDensity(0);

		// force caelum to update
		if(value)
			mefl->getCaelumSystem()->setLocalTime(mefl->getCaelumSystem()->getLocalTime()+1);
		else
			mefl->getCaelumSystem()->setLocalTime(mefl->getCaelumSystem()->getLocalTime()-1);
	}
	else if(fogmode == 3)
	{
		if(value)
			smgr->setFog(FOG_LINEAR, smgr->getFogColour(), mefl->getFogDensity(), smgr->getFogStart(), smgr->getFogEnd());
		else
			smgr->setFog(FOG_NONE, smgr->getFogColour(), mefl->getFogDensity(), smgr->getFogStart(), smgr->getFogEnd());
	}

}

void MapTextureCreator::setAutoUpdated(bool value)
{
	rttTex->setAutoUpdated(value);
}

Ogre::String MapTextureCreator::getMaterialName()
{
	return "MapRttMat"+StringConverter::toString(counter);
}

Ogre::String MapTextureCreator::getRTName()
{
	return "MapRttTex"+StringConverter::toString(counter);
}

void MapTextureCreator::preRenderTargetUpdate(const RenderTargetEvent& evt)
{
	Water *w = mefl->getWater();
	if(w)
		w->setVisible(false);
}
void MapTextureCreator::postRenderTargetUpdate(const RenderTargetEvent& evt)
{
	Water *w = mefl->getWater();
	if(w)
		w->setVisible(true);
}



Ogre::String MapEntity::entityStates[MAX_MAP_ENTITYSTATES] = {"activated", "deactivated", "sleeping", "networked"};

MapEntity::MapEntity(MapControl *ctrl, int _uid, String type, MyGUI::RenderBoxPtr _parent)
{
	mapCtrl=ctrl;
	parent=_parent;
	myType=type;
	uid=_uid;
	x=0;
	z=0;
	r=0;
	tw=0;
	th=0;
	myState=0;
	init();
}

MapEntity::~MapEntity()
{
}

void MapEntity::init()
{
	icon = parent->createWidget<MyGUI::StaticImage>("StaticImage", 10, 10, 200, 10, MyGUI::Align::Default);
	txt = parent->createWidget<MyGUI::StaticText>("StaticText", 10, 10, 200, 40, MyGUI::Align::Default);
	txt->setFontHeight(12);

	// check if static only icon
	String staticImgFile = "icon_"+myType+".dds";
	String group=ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
	if(ResourceGroupManager::getSingleton().resourceExists(group, staticImgFile))
	{
		LogManager::getSingleton().logMessage("static map icon found: "+ staticImgFile);
		// this icon is static!
		icon->setImageTexture(staticImgFile);
		Ogre::TexturePtr t = (Ogre::TexturePtr)(TextureManager::getSingleton().getByName(staticImgFile));
		tw = t->getWidth();
		th = t->getHeight();
		isStatic=true;
	} else
	{
		LogManager::getSingleton().logMessage("static map icon not found: "+ staticImgFile);
		// not static, now check the states!
		for(int i=0;i<MAX_MAP_ENTITYSTATES;i++)
		{
			String imgFile = "icon_"+myType+"_"+entityStates[i]+".dds";
			String group2 = "";
			if(group2 == "")
			{
				try
				{
					group2 = ResourceGroupManager::getSingleton().findGroupContainingResource(imgFile);
				}catch(...)
				{
				}
			}
			if(group2 != "")
			{
				icon->setImageTexture(imgFile);
				Ogre::TexturePtr t = (Ogre::TexturePtr)(TextureManager::getSingleton().getByName(imgFile));
				tw = t->getWidth();
				th = t->getHeight();
			} else
			{
				icon->setImageTexture("icon_missing.dds");
				Ogre::TexturePtr t = (Ogre::TexturePtr)(TextureManager::getSingleton().getByName("icon_missing.dds"));
				tw = t->getWidth();
				th = t->getHeight();
			}
		}
		isStatic=false;
	}

	//hard wire texture sizes:
	//tw = 32;
	//th = 32;

	scale = 1;
	minsize = 16;
	setVisibility(false);

	update();
}

void MapEntity::setPosition(Vector3 pos)
{
	bool needUpdate=false;
	if(fabs(x - pos.x) > 0.00001f || fabs(z - pos.z) > 0.00001f)
		needUpdate=true;
	x=pos.x;
	z=pos.z;
	if(needUpdate)
		update();
}

void MapEntity::setPosition(float _x, float _z)
{
	bool needUpdate=false;
	if(fabs(_x - x) > 0.00001f || fabs(_z - z) > 0.00001f)
		needUpdate=true;
	x=_x;
	z=_z;
	if(needUpdate)
		update();
}

void MapEntity::setRotation(float _r)
{
	bool needUpdate=false;
	//if(fabs(_r - r) > 2)
		needUpdate=true;
	r=_r;
	if(needUpdate)
		update();
}

void MapEntity::setRotation(Ogre::Quaternion q)
{
	bool needUpdate=false;
	float newr = q.getYaw().valueDegrees() - 90;
	//if(fabs(newr - r) > 2)
		needUpdate=true;
	r=newr;
	if(needUpdate)
		update();
}

void MapEntity::setRotation(Ogre::Radian _r)
{
	bool needUpdate=false;
	float newr = _r.valueDegrees();
	//if(fabs(newr - r) > 2)
		needUpdate=true;
	r=newr;
	if(needUpdate)
		update();
}

void MapEntity::onTop()
{
//	container->_notifyZOrder(container->getZOrder()+10);
}

bool MapEntity::getVisibility()
{
	return icon->isVisible();
}

void MapEntity::setVisibility(bool value)
{
	icon->setVisible(value);
}

void MapEntity::setState(int truckstate)
{
	int mapstate=2;
	//transform the state to the correct map value!!!
	switch (truckstate)
	{
	case ACTIVATED: mapstate=0; break;
	case DESACTIVATED:
	case MAYSLEEP:
	case GOSLEEP: mapstate=1; break;
	case SLEEPING: mapstate=2; break;
	case NETWORKED: mapstate=3; break;
	default: mapstate=2;
	}

	if(isStatic)
		return;

	bool needUpdate=false;
	if(myState!=mapstate)
		needUpdate=true;
	myState=mapstate;
	if(needUpdate)
		update();
}

int MapEntity::getState()
{
	return myState;
}

void MapEntity::update()
{
	float wscale = mapCtrl->getWindowScale();

	txt->setCaption(myDescription);
	txt->setVisible(wscale > 0.5f);

	Vector2 s = mapCtrl->getMapSize();
	txt->setPosition((x/((float)s.x))*parent->getWidth() + (tw * wscale) * 0.6f,
					 (z/((float)s.y))*parent->getHeight() - (th * wscale) * 0.3f);
	icon->setCoord(
			(x/((float)s.x))*parent->getWidth() - (tw * wscale) * 0.5f,
			(z/((float)s.y))*parent->getHeight() - (th * wscale) * 0.5f, 
			tw * wscale,
			th * wscale);
	icon->setVisible(true);
}

void MapEntity::setDescription(Ogre::String s)
{
	myDescription = s;
}

Ogre::String MapEntity::getDescription()
{
	return myDescription;
}





int MapControl::mapcounter=0;

MapControl::MapControl(int _mapsizex, int _mapsizez) : bgInitiated(false), uniqueCounter(0),x(0),y(0),w(100),h(100)
{
	mapsizex=_mapsizex;
	mapsizez=_mapsizez;
	mapcounter++;
	rWinWidth=1;
	rWinHeight=1;
	alpha=1;
	mtc=0;

	// create window
	rb = MyGUI::Gui::getInstance().createWidget<MyGUI::RenderBox>("RenderBox", 10, 10, 300, 300, MyGUI::Align::Default, "Modal", "render_map");
}

MapControl::~MapControl()
{
}

void MapControl::setMTC(MapTextureCreator *mtc)
{
	this->mtc = mtc;
	rb->_setTextureName(mtc->getRTName());

}


void MapControl::setWorldSize(int x, int z)
{
	this->mapsizex = x;
	this->mapsizez = z;
}

MapEntity *MapControl::createMapEntity(String type)
{
	uniqueCounter++;
	MapEntity *m = new MapEntity(this, uniqueCounter, type, rb);
	if(m!=0)
		mapEntities.push_back(m);
	return m;
}

MapEntity *MapControl::createNamedMapEntity(Ogre::String name, Ogre::String type)
{
	MapEntity *e = createMapEntity(type);
	namedEntities[name] = e;
	return e;
}

MapEntity *MapControl::getEntityByName(Ogre::String name)
{
	try
	{
		return namedEntities[name];
	}catch(...)
	{
		return 0;
	}
}

Ogre::String MapControl::getTypeByDriveable(int driveable)
{
	if(driveable == NOT_DRIVEABLE)
		return "load";
	else if(driveable == TRUCK)
		return "truck";
	else if(driveable == AIRPLANE)
		return "airplane";
	else if(driveable == BOAT)
		return "boat";
	else if(driveable == MACHINE)
		return "machine";
	return "unkown";
}

void MapControl::deleteMapEntity(MapEntity *ent)
{
	std::vector<MapEntity *>::iterator it;
	for(it=mapEntities.begin(); it!= mapEntities.end(); it++)
	{
		if((*it)->getUID() == ent->getUID())
		{
			// found it, erase!
			delete *it;
			mapEntities.erase(it);
			return;
		}
	}
}

bool MapControl::getVisibility()
{
	return rb->isVisible();
}

void MapControl::setVisibility(bool value)
{
	rb->setVisible(value);
}

void MapControl::setAlpha(float value)
{
	rb->setAlpha(value);
}

void MapControl::setPosition(float _x, float _y, float _w, float _h, Ogre::RenderWindow* rw)
{
	rb->setCoord(_x*rWinWidth, _y*rWinHeight, _w*rWinWidth, _h*rWinHeight);
	myScale = _w;
	updateEntityPositions();
}


void MapControl::updateEntityPositions()
{
	std::vector<MapEntity *>::iterator it;
	for(it=mapEntities.begin(); it!=mapEntities.end(); it++)
	{
		(*it)->update();
	}
}

void MapControl::setEntityVisibility(bool value)
{
	std::vector<MapEntity *>::iterator it;
	for(it=mapEntities.begin(); it!=mapEntities.end(); it++)
	{
		(*it)->setVisibility(value);
	}
}

void MapControl::resizeToScreenRatio(Ogre::RenderWindow* rw)
{
	//win->setRealPosition(
	// TODO
}

void MapControl::updateRenderMetrics(Ogre::RenderWindow* win)
{
	win->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
}

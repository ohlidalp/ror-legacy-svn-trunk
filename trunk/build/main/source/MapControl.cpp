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
	init();
}

void MapTextureCreator::init()
{
	TexturePtr texture = TextureManager::getSingleton().createManual("MapRttTex"+StringConverter::toString(counter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 2048, 2048, 0, PF_R8G8B8, TU_RENDERTARGET,new ResourceBuffer());
	rttTex = texture->getBuffer()->getRenderTarget();
	rttTex->setAutoUpdated(true);
	mCamera = smgr->createCamera("MapRenderCam");
	mCamera->setAutoAspectRatio(true);

	v = rttTex->addViewport(mCamera);
	v->setBackgroundColour(ColourValue::White);
	v->setOverlaysEnabled(false);

	mat = MaterialManager::getSingleton().create("MapRttMat"+StringConverter::toString(counter), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	mat->getTechnique(0)->setLightingEnabled(false);
	mat->getTechnique(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
	mat->getTechnique(0)->setTextureFiltering(TFO_ANISOTROPIC);
	mat->getTechnique(0)->setDepthCheckEnabled(true);
	mat->getTechnique(0)->setDepthWriteEnabled(true);
	mat->getTechnique(0)->setCullingMode(Ogre::CULL_NONE);

	tex = mat->getTechnique(0)->getPass(0)->createTextureUnitState("MapRttTex"+StringConverter::toString(counter));
	rttTex->addListener(this);
	//mCamera->setProjectionType(PT_ORTHOGRAPHIC);

	zoom = 1;

	mCamera->setPosition(0, 4500, 0.0001);
	mCamera->lookAt(0,0,0);

	//setCamPosition(1500, 1500, 0);
	mCamera->setNearClipDistance(1);
	mCamera->setFarClipDistance(10000);
	rttTex->update();
}

Ogre::Matrix4 MapTextureCreator::BuildScaledOrthoMatrix(float left, float right, float bottom, float top, float fnear, float ffar)
{
	float invw = 1 / (right - left);
	float invh = 1 / (top - bottom);
	float invd = 1 / (ffar - fnear);

	Matrix4 proj = Matrix4::ZERO;
	proj[0][0] = 2 * invw;
	proj[0][3] = -(right + left) * invw;
	proj[1][1] = 2 * invh;
	proj[1][3] = -(top + bottom) * invh;
	proj[2][2] = -2 * invd;
	proj[2][3] = -(ffar + fnear) * invd;
	proj[3][3] = 1;

	return proj;
}

void MapTextureCreator::setTranlucency(float amount)
{
	tex->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_MANUAL, 1.0, amount);
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
	zoom += zoomdelta;
}

void MapTextureCreator::setCamZoom(float newzoom)
{
	zoom = newzoom;
}

void MapTextureCreator::setCamPosition(float x, float z)
{
	camx = x;
	camz = -z;
}


void MapTextureCreator::update()
{
	// 1 = max out = total overview
	if(zoom<=0.1)
		zoom=0.1;

	float width = mefl->mapsizex;
	float height = mefl->mapsizez;

	//LogManager::getSingleton().logMessage(StringConverter::toString(zoom));
	Matrix4 p = BuildScaledOrthoMatrix(width  / zoom / -2.0f + camx,
	                                   width  / zoom /  2.0f + camx,
	                                   height / zoom / -2.0f + camz,
	                                   height / zoom /  2.0f + camz, 0, 5000);
	// apply zoom matrix to camera
	mCamera->setCustomProjectionMatrix(true, p);

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
		statics->setRenderingDistance(10000);
	// thats a huge workaround to be able to not use the normal LOD
#ifdef OGREPLUGIN
	TerrainOptions to;
	if(!mefl->usingETM())
	{
		to = ((TerrainSceneManager*)smgr)->getOptions();
		((TerrainSceneManager*)smgr)->setMaxPixelError(0);
		((TerrainSceneManager*)smgr)->setLODMorphStart(100000);
		((TerrainSceneManager*)smgr)->setUseLODMorph(false);
		((TerrainSceneManager*)smgr)->setDetailTextureRepeat(0);
		((TerrainSceneManager*)smgr)->setMaxGeoMipMapLevel(2);
		((TerrainSceneManager*)smgr)->setPrimaryCamera(mCamera);
		((TerrainSceneManager*)smgr)->setUseTriStrips(false);
		//((TerrainSceneManager*)smgr)->setUseVertexNormals(false);
	}
#endif
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

#ifdef OGREPLUGIN
	if(!mefl->usingETM())
	{
		((TerrainSceneManager*)smgr)->setMaxPixelError((int)to.maxPixelError);
		((TerrainSceneManager*)smgr)->setLODMorphStart(to.lodMorphStart);
		((TerrainSceneManager*)smgr)->setUseLODMorph(to.lodMorph);
		((TerrainSceneManager*)smgr)->setDetailTextureRepeat((int)to.detailTile);
		((TerrainSceneManager*)smgr)->setMaxGeoMipMapLevel((int)to.maxGeoMipMapLevel);
		((TerrainSceneManager*)smgr)->setPrimaryCamera(mainCam);
		((TerrainSceneManager*)smgr)->setUseTriStrips(to.useTriStrips);
	}
#endif
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

MapEntity::MapEntity(MapControl *ctrl, int _uid, String type, OverlayContainer* _parent)
{
	mapCtrl=ctrl;
	container=0;
	parent=_parent;
	myType=type;
	uid=_uid;
	x=0;
	z=0;
	r=0;
	myState=0;
	init();
}

MapEntity::~MapEntity()
{
}

void MapEntity::init()
{
	//add the container first
	String ovName = "tracks/mapEntity"+StringConverter::toString(uid);
	container = (OverlayContainer*)OverlayManager::getSingleton().createOverlayElement("Panel", ovName+"Container");
	container->setMetricsMode(Ogre::GMM_PIXELS);
	parent->addChild(container);

	// add description container first
	taoe=new TextAreaOverlayElement(ovName+"Text");
	taoe->setColourTop(ColourValue(0.1, 0.1, 0.1, 1.0));
	taoe->setColourBottom(ColourValue(0.0, 0.0, 0.0, 1.0));
	taoe->setFontName("highcontrast_black");
	taoe->setCharHeight(0.02);
	taoe->setCaption("");
	taoe->setTop(0.001);
	taoe->setLeft(30);

	// check if static only icon
	String staticImgFile = "icon_"+myType+".dds";
	String staticMatName = "tracks/map/icons/"+myType;
	String group="General";
	/*
	// we use a different method here to remove all the error warnings in the log files
	if(group == "")
	{
		try
		{
			group = ResourceGroupManager::getSingleton().findGroupContainingResource(staticImgFile);
		}catch(...)
		{
		}
	}
	if(group != "")
	*/
	if(ResourceGroupManager::getSingleton().resourceExists(group, staticImgFile))
	{
		LogManager::getSingleton().logMessage("static map icon found: "+ staticImgFile);
		// this icon is static!
		MaterialPtr m;
		if(MaterialManager::getSingleton().resourceExists(staticMatName))
		{
			m = MaterialManager::getSingleton().getByName(staticMatName);
		} else
		{
			m = MaterialManager::getSingleton().create(staticMatName, "General");
			m->getTechnique(0)->getPass(0)->createTextureUnitState(staticImgFile);
			m->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		}
		// we need to clone them, so we can rotate them individuelly
		myMaterials[0] = m->clone(staticMatName+"_|_"+StringConverter::toString(uid));
		isStatic=true;
	} else
	{
		LogManager::getSingleton().logMessage("static map icon not found: "+ staticImgFile);
		// not static, now check the states!
		for(int i=0;i<MAX_MAP_ENTITYSTATES;i++)
		{
			String imgFile = "icon_"+myType+"_"+entityStates[i]+".dds";
			String matName = "tracks/map/icons/"+myType+"/"+entityStates[i];
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
				MaterialPtr m;
				if(MaterialManager::getSingleton().resourceExists(matName))
				{
					m = MaterialManager::getSingleton().getByName(matName);
				} else
				{
					m = MaterialManager::getSingleton().create(matName, "General");
					m->getTechnique(0)->getPass(0)->createTextureUnitState(imgFile);
					m->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
				}
				// we need to clone them, so we can rotate them individuelly
				myMaterials[i] = m->clone(matName+"_|_"+StringConverter::toString(uid));
			} else
			{
				MaterialPtr m = MaterialManager::getSingleton().getByName("tracks/map/icons/missing");
				if(m.getPointer() != 0)
					myMaterials[i] = m->clone(matName+"_|_"+StringConverter::toString(uid));

			}
		}
		isStatic=false;
	}
	scale = 1;
	minsize = 16;
	setVisibility(false);
	update();
}

void MapEntity::setPosition(Vector3 pos)
{
	bool needUpdate=false;
	if(x != pos.x || z != pos.z)
		needUpdate=true;
	x=pos.x;
	z=pos.z;
	if(needUpdate)
		update();
}

void MapEntity::setPosition(float _x, float _z)
{
	bool needUpdate=false;
	if(_x != x || _z != z)
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
	if(container)
		return container->isVisible();
	return false;
}

void MapEntity::setVisibility(bool value)
{
	if(container && value)
		return container->show();
	if(container && !value)
		return container->hide();
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
	try
	{
		taoe->setCaption(myDescription);
	} catch(...)
	{
	}
	// some extensive error checking to be more more modding friendly
	MaterialPtr mat = myMaterials[myState];
	if(!mat.isNull())
	{
		if(mat->getNumTechniques() == 0)
			return;
		Technique *t = mat->getTechnique(0);
		if(!t)
			return;
		if(t->getNumPasses() == 0)
			return;
		Pass *p = t->getPass(0);
		if(!p)
			return;
		if(p->getNumTextureUnitStates() == 0)
			return;
		TextureUnitState *tu = p->getTextureUnitState(0);
		if(!tu)
			return;
		std::pair<size_t,size_t> d = tu->getTextureDimensions();
		mapCtrl->setEntityPosition(x*scale, z*scale, (float)d.first*scale, (float)d.second*scale, minsize, container);
		taoe->setLeft(0);

		container->setMaterialName(mat->getName());
		tu->setTextureRotate(Ogre::Degree(r));
	} else
	{
		container->setMaterialName("tracks/map/icons/missing");
	}
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

MapControl::MapControl(int _mapsizex, int _mapsizez) : container(0), bgInitiated(false), uniqueCounter(0),x(0),y(0),w(100),h(100)
{
	mapsizex=_mapsizex;
	mapsizez=_mapsizez;
	mapcounter++;
	rWinWidth=1;
	rWinHeight=1;
	alpha=1;
	// create overlay
	String ovName = "tracks/map"+StringConverter::toString(mapcounter);
	mainOverlay = OverlayManager::getSingleton().create(ovName+"Overlay");

	container = (OverlayContainer*)OverlayManager::getSingleton().createOverlayElement("Panel", ovName+"Container");
	container->setMetricsMode(Ogre::GMM_PIXELS);
	mainOverlay->add2D(container);

	String matName = ovName+"_bgmat";
	bgMat=MaterialManager::getSingleton().create(matName, "General");
	container->setMaterialName(matName);
}

MapControl::~MapControl()
{
}

void MapControl::setWorldSize(int x, int z)
{
	this->mapsizex = x;
	this->mapsizez = z;
}

void MapControl::setBackgroundMaterial(Ogre::String matName)
{
	container->setMaterialName(matName);
}

MapEntity *MapControl::createMapEntity(String type)
{
	uniqueCounter++;
	MapEntity *m = new MapEntity(this, uniqueCounter, type, container);
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

void MapControl::deleteMapEntity(MapEntity *)
{
}

bool MapControl::getVisibility()
{
	if(mainOverlay)
		return mainOverlay->isVisible();
	return false;
}

void MapControl::setVisibility(bool value)
{
	if(mainOverlay && value)
		return mainOverlay->show();
	if(mainOverlay && !value)
		return mainOverlay->hide();
}

void MapControl::setBackground(String texName)
{
	if(!mainOverlay)
		return;
	if(!bgInitiated)
	{
		bgMat->getTechnique(0)->getPass(0)->createTextureUnitState(texName);
		bgInitiated=true;
	}
	else
	{
		bgMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(texName);
	}
	bgMat->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
	bgMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_MANUAL, 1.0, alpha);
}

void MapControl::setAlpha(float value)
{
	alpha = value;
	if(bgMat.isNull())
		return;
	if(bgMat->getNumTechniques() == 0)
		return;
	Technique *t = bgMat->getTechnique(0);
	if(t->getNumPasses() == 0)
		return;
	Pass *p = t->getPass(0);
	if(p->getNumTextureUnitStates() == 0)
		return;
	p->getTextureUnitState(0)->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_MANUAL, 1.0, alpha);
}

void MapControl::setPosition(float _x, float _y, float _w, float _h, Ogre::RenderWindow* rw)
{
	float nw=0, nh=0;
	bool changedSize=false;
	if(!container)
		return;
	if(!rw)
	{
		x=_x;
		y=_y;
		nw=_w;
		nh=_h;
	} else
	{
		x=_x*rWinWidth;
		y=_y*rWinHeight;
		nw=_w*rWinWidth;
		nh=_h*rWinHeight;
	}
	if(nw != w || nh != h)
		changedSize=true;

	h=nh;
	w=nw;
	container->setLeft(x);
	container->setTop(y);
	container->setWidth(w);
	container->setHeight(h);
	if(changedSize)
		updateEntityPositions();
}

void MapControl::setPosition(float _x, float _y, float _w, Ogre::RenderWindow* rw)
{
	float nw=0, nh=0;
	bool changedSize=false;
	if(!container||!rw)
		return;
	x=_x;
	y=_y;
	nw=_w*rWinWidth;
	nh=nw;
	if(nw != w || nh != h)
		changedSize=true;

	h=nh;
	w=nw;
	container->setLeft(x);
	container->setTop(y);
	container->setWidth(w);
	container->setHeight(h);
	if(changedSize)
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

void MapControl::resizeToScreenRatio(Ogre::RenderWindow* win)
{
	if(!container)
		return;
	container->setHeight(container->getHeight()*(Real)win->getWidth()/(Real)win->getHeight());
	container->setTop(container->getTop()*(Real)win->getWidth()/(Real)win->getHeight());
}

void MapControl::setEntityPosition(float xE, float yE, float wE, float hE, float minsize, Ogre::OverlayContainer *e)
{
	if(!e)
		return;

	float newWidth = (w/(rWinWidth*0.8)) * wE;
	float newHeight = (h/(rWinHeight*0.8)) * hE;
	// enforce minimum size
	if(newWidth < minsize)
		newWidth = minsize;
	if(newHeight < minsize)
		newHeight = minsize;
	e->setWidth(newWidth);
	e->setHeight(newHeight);
	float x = xE/float(MapControl::mapsizex);
	float y = yE/float(MapControl::mapsizez);

	e->setLeft( x * w - newWidth/2);
	e->setTop( y * h - newHeight/2);
}

void MapControl::updateRenderMetrics(Ogre::RenderWindow* win)
{
	win->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
}

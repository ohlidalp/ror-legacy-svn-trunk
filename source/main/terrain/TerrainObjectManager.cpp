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
#include "TerrainObjectManager.h"

#include "BeamFactory.h"
#include "CameraManager.h"
#include "RoRFrameListener.h"
#include "SkyManager.h"

using namespace Ogre;

TerrainObjectManager::TerrainObjectManager(TerrainManager *terrainManager) :
terrainManager(terrainManager)
{
}

TerrainObjectManager::~TerrainObjectManager()
{

}

void TerrainObjectManager::loadObjectConfigFile( Ogre::String filename )
{
	//prepare for baking
	SceneNode *bakeNode=mSceneMgr->getRootSceneNode()->createChildSceneNode();

#ifdef USE_PAGED
	treeLoader = 0;
	Entity *curTree = 0;
	String treename = "";
#endif

	ProceduralObject po;
	po.loadingState = -1;
	int r2oldmode = 0;
	int lastprogress = -1;
	bool proroad = false;

	while (!ds->eof())
	{
		int progress = ((float)(ds->tell()) / (float)(ds->size())) * 100.0f;
		if (progress-lastprogress > 20)
		{
#ifdef USE_MYGUI
			LoadingWindow::getSingleton().setProgress(progress, _L("Loading Terrain"));
#endif //MYGUI
			lastprogress = progress;
		}

		char oname[1024];
		char type[256];
		char name[256];
		int r;
		float ox, oy, oz;
		float rx, ry, rz;
		//fscanf(fd," %[^\n\r]",line);
		size_t ll=ds->readLine(line, 1023);
		if (line[0]=='/' || line[0]==';' || ll==0) continue; //comments
		if (!strcmp("end",line)) break;

		if (!strncmp(line,"collision-tris", 14))
		{
			long amount = Collisions::MAX_COLLISION_TRIS;
			sscanf(line, "collision-tris %ld", &amount);
			collisions->resizeMemory(amount);
		}

		if (!strncmp(line,"grid", 4))
		{
			float px=0,py=0,pz=0;
			sscanf(line, "grid %f, %f, %f", &px, &py, &pz);
			Vector3 pos = Vector3(px,py,pz);

			Ogre::ColourValue BackgroundColour = Ogre::ColourValue::White;//Ogre::ColourValue(0.1337f, 0.1337f, 0.1337f, 1.0f);
			Ogre::ColourValue GridColour = Ogre::ColourValue(0.2f, 0.2f, 0.2f, 1.0f);

			Ogre::ManualObject *mReferenceObject = new Ogre::ManualObject("ReferenceGrid");

			mReferenceObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST);

			Ogre::Real step = 1.0f;
			unsigned int count = 50;
			unsigned int halfCount = count / 2;
			Ogre::Real full = (step * count);
			Ogre::Real half = full / 2;
			Ogre::Real y = 0;
			Ogre::ColourValue c;
			for (unsigned i=0;i < count+1;i++)
			{
				if (i == halfCount)
					c = Ogre::ColourValue(1,0,0,1.0f);
				else
					c = GridColour;

				mReferenceObject->position(-half,y,-half+(step*i));
				mReferenceObject->colour(BackgroundColour);
				mReferenceObject->position(0,y,-half+(step*i));
				mReferenceObject->colour(c);
				mReferenceObject->position(0,y,-half+(step*i));
				mReferenceObject->colour(c);
				mReferenceObject->position(half,y,-half+(step*i));
				mReferenceObject->colour(BackgroundColour);

				if (i == halfCount)
					c = Ogre::ColourValue(0,0,1,1.0f);
				else
					c = GridColour;

				mReferenceObject->position(-half+(step*i),y,-half);
				mReferenceObject->colour(BackgroundColour);
				mReferenceObject->position(-half+(step*i),y,0);
				mReferenceObject->colour(c);
				mReferenceObject->position(-half+(step*i),y,0);
				mReferenceObject->colour(c);
				mReferenceObject->position(-half+(step*i),y, half);
				mReferenceObject->colour(BackgroundColour);
			}

			mReferenceObject->end();
			mReferenceObject->setCastShadows(false);
			SceneNode *n = mSceneMgr->getRootSceneNode()->createChildSceneNode();
			n->setPosition(pos);
			n->attachObject(mReferenceObject);
			n->setVisible(true);
		}

		if (!strncmp("mpspawn", line, 7))
		{
			spawn_location_t spl;
			memset(&spl, 0, sizeof(spawn_location_t));

			char tmp[256]="";
			float x=0,y=0,z=0, rx=0, ry=0, rz=0;
			int res = sscanf(line, "mpspawn %s %f %f %f %f %f %f", tmp, &x, &y, &z, &rx, &ry, &rz);
			if (res < 7)
			{
				LOG("error reading mpspawn command!");
				continue;
			}
			spl.pos = Vector3(x, y, z);
			spl.rot = Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);
			netSpawnPos[String(tmp)] = spl;
			continue;
		}

		//sandstorm cube texture
		if (!strncmp(line,"sandstormcubemap", 16))
		{
			sscanf(line, "sandstormcubemap %s", sandstormcubemap);
		}
		if (!strncmp("landuse-config", line, 14))
		{
			collisions->setupLandUse(line+15);
			continue;
		}
		if (!strncmp("gravity", line, 7))
		{
			int res = sscanf(line, "gravity %f", &gravity);
			if (res < 1)
			{
				LOG("error reading gravity command!");
			}
			continue;
		}
		//ugly stuff to parse map size
		if (!strncmp("mapsize", line, 7))
		{
			// this is deprecated!!! (replaced by direct .cfg reads)
			//sscanf(line, "mapsize %f, %f", &mapsizex, &mapsizez);
			continue;
		}
#ifdef USE_PAGED
		//ugly stuff to parse trees :)
		if (!strncmp("trees", line, 5))
		{
			if (pagedMode == 0) continue;
			char ColorMap[256] = {};
			char DensityMap[256] = {};
			char treemesh[256] = {};
			char treeCollmesh[256] = {};
			float gridspacing = 0.0f;
			float yawfrom = 0.0f, yawto = 0.0f;
			float scalefrom = 0.0f, scaleto = 0.0f;
			float highdens = 1.0f;
			int minDist = 90, maxDist = 700;
			sscanf(line, "trees %f, %f, %f, %f, %f, %d, %d, %s %s %s %f %s", &yawfrom, &yawto, &scalefrom, &scaleto, &highdens, &minDist, &maxDist, treemesh, ColorMap, DensityMap, &gridspacing, treeCollmesh);
			if (strnlen(ColorMap, 3) == 0)
			{
				LOG("tree ColorMap map zero!");
				continue;
			}
			if (strnlen(DensityMap, 3) == 0)
			{
				LOG("tree DensityMap zero!");
				continue;
			}
			Forests::DensityMap *densityMap = Forests::DensityMap::load(DensityMap, CHANNEL_COLOR);
			if (!densityMap)
			{
				LOG("could not load densityMap: "+String(DensityMap));
				continue;
			}
			densityMap->setFilter(Forests::MAPFILTER_BILINEAR);
			//densityMap->setMapBounds(TRect(0, 0, mapsizex, mapsizez));

			paged_geometry_t paged;
			paged.geom = new PagedGeometry();
			paged.geom->setTempDir(SSETTING("User Path", "") + "cache" + SSETTING("dirsep", "\\"));
			paged.geom->setCamera(mCamera);
			paged.geom->setPageSize(50);
			paged.geom->setInfinite();
			Ogre::TRect<Ogre::Real> bounds = TBounds(0, 0, mapsizex, mapsizez);
			//trees->setBounds(bounds);

			//Set up LODs
			//trees->addDetailLevel<EntityPage>(50);
			float min = minDist * pagedDetailFactor;
			if (min<10) min = 10;
			paged.geom->addDetailLevel<BatchPage>(min, min/2);
			float max = maxDist * pagedDetailFactor;
			if (max<10) max = 10;
			paged.geom->addDetailLevel<ImpostorPage>(max, max/10);
			TreeLoader2D *treeLoader = new TreeLoader2D(paged.geom, TBounds(0, 0, mapsizex, mapsizez));
			paged.geom->setPageLoader(treeLoader);
			treeLoader->setHeightFunction(&getTerrainHeight);
			if (String(ColorMap) != "none")
			{
				treeLoader->setColorMap(ColorMap);
			}

			curTree = mSceneMgr->createEntity(String("paged_")+treemesh+TOSTRING(pagedGeometry.size()), treemesh);

			if (gridspacing > 0)
			{
				// grid style
				for(float x=0; x < mapsizex; x += gridspacing)
				{
					for(float z=0; z < mapsizez; z += gridspacing)
					{
						float density = densityMap->_getDensityAt_Unfiltered(x, z, bounds);
						if (density < 0.8f) continue;
						float nx = x + gridspacing * 0.5f;
						float nz = z + gridspacing * 0.5f;
						float yaw = Math::RangeRandom(yawfrom, yawto);
						float scale = Math::RangeRandom(scalefrom, scaleto);
						Vector3 pos = Vector3(nx, 0, nz);
						treeLoader->addTree(curTree, pos, Degree(yaw), (Ogre::Real)scale);
						if (strlen(treeCollmesh))
						{
							pos.y = hfinder->getHeightAt(pos.x, pos.z);
							scale *= 0.1f;
							collisions->addCollisionMesh(String(treeCollmesh), pos, Quaternion(Degree(yaw), Vector3::UNIT_Y), Vector3(scale, scale, scale));
						}
					}
				}

			} else
			{
				float gridsize = 10;
				if (gridspacing < 0 && gridspacing != 0)
				{
					gridsize = -gridspacing;
				}
				float hd = highdens;
				// normal style, random
				for(float x=0; x < mapsizex; x += gridsize)
				{
					for(float z=0; z < mapsizez; z += gridsize)
					{
						if (highdens < 0) hd = Math::RangeRandom(0, -highdens);
						float density = densityMap->_getDensityAt_Unfiltered(x, z, bounds);
						int numTreesToPlace = (int)((float)(hd) * density * pagedDetailFactor);
						float nx=0, nz=0;
						while(numTreesToPlace-->0)
						{
							nx = Math::RangeRandom(x, x + gridsize);
							nz = Math::RangeRandom(z, z + gridsize);
							float yaw = Math::RangeRandom(yawfrom, yawto);
							float scale = Math::RangeRandom(scalefrom, scaleto);
							Vector3 pos = Vector3(nx, 0, nz);
							treeLoader->addTree(curTree, pos, Degree(yaw), (Ogre::Real)scale);
							if (strlen(treeCollmesh))
							{
								pos.y = hfinder->getHeightAt(pos.x, pos.z);
								collisions->addCollisionMesh(String(treeCollmesh),pos, Quaternion(Degree(yaw), Vector3::UNIT_Y), Vector3(scale, scale, scale));
							}
						}
					}
				}
			}
			paged.loader = (void*)treeLoader;
			pagedGeometry.push_back(paged);
		}

		//ugly stuff to parse grass :)
		if (!strncmp("grass", line, 5) || !strncmp("grass2", line, 6))
		{
			// is paged geometry disabled by configuration?
			if (pagedMode == 0) continue;
			int range = 80;
			float SwaySpeed=0.5, SwayLength=0.05, SwayDistribution=10.0, minx=0.2, miny=0.2, maxx=1, maxy=0.6, Density=0.6, minH=-9999, maxH=9999;
			char grassmat[256]="";
			char ColorMap[256]="";
			char DensityMap[256]="";
			int growtechnique = 0;
			int techn = GRASSTECH_CROSSQUADS;
			if (!strncmp("grass2", line, 6))
				sscanf(line, "grass2 %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %d, %s %s %s", &range, &SwaySpeed, &SwayLength, &SwayDistribution, &Density, &minx, &miny, &maxx, &maxy, &growtechnique, &minH, &maxH, &techn, grassmat, ColorMap, DensityMap);
			else if (!strncmp("grass", line, 5))
				sscanf(line, "grass %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %s %s %s", &range, &SwaySpeed, &SwayLength, &SwayDistribution, &Density, &minx, &miny, &maxx, &maxy, &growtechnique, &minH, &maxH, grassmat, ColorMap, DensityMap);

			//Initialize the PagedGeometry engine
			try
			{
				paged_geometry_t paged;
				PagedGeometry *grass = new PagedGeometry(mCamera, 30);
				//Set up LODs

				grass->addDetailLevel<GrassPage>(range * pagedDetailFactor); // original value: 80

				//Set up a GrassLoader for easy use
				GrassLoader *grassLoader = new GrassLoader(grass);
				grass->setPageLoader(grassLoader);
				grassLoader->setHeightFunction(&getTerrainHeight);

				// render grass at first
				grassLoader->setRenderQueueGroup(RENDER_QUEUE_MAIN-1);

				GrassLayer* grassLayer = grassLoader->addLayer(grassmat);
				grassLayer->setHeightRange(minH, maxH);
				//grassLayer->setLightingEnabled(true);

				grassLayer->setAnimationEnabled((SwaySpeed>0));
				grassLayer->setSwaySpeed(SwaySpeed);
				grassLayer->setSwayLength(SwayLength);
				grassLayer->setSwayDistribution(SwayDistribution);

				grassdensityTextureFilename = String(DensityMap);

				grassLayer->setDensity(Density * pagedDetailFactor);
				if (techn>10)
					grassLayer->setRenderTechnique(static_cast<GrassTechnique>(techn-10), true);
				else
					grassLayer->setRenderTechnique(static_cast<GrassTechnique>(techn), false);

				grassLayer->setMapBounds(TBounds(0, 0, mapsizex, mapsizez));

				if (strcmp(ColorMap,"none") != 0)
				{
					grassLayer->setColorMap(ColorMap);
					grassLayer->setColorMapFilter(MAPFILTER_BILINEAR);
				}

				if (strcmp(DensityMap,"none") != 0)
				{
					grassLayer->setDensityMap(DensityMap);
					grassLayer->setDensityMapFilter(MAPFILTER_BILINEAR);
				}

				//grassLayer->setMinimumSize(0.5,0.5);
				//grassLayer->setMaximumSize(1.0, 1.0);

				grassLayer->setMinimumSize(minx, miny);
				grassLayer->setMaximumSize(maxx, maxy);

				// growtechnique
				if (growtechnique == 0)
					grassLayer->setFadeTechnique(FADETECH_GROW);
				else if (growtechnique == 1)
					grassLayer->setFadeTechnique(FADETECH_ALPHAGROW);
				else if (growtechnique == 2)
					grassLayer->setFadeTechnique(FADETECH_ALPHA);
				paged.geom = grass;
				paged.loader = (void*)grassLoader;
				pagedGeometry.push_back(paged);
			} catch(...)
			{
				LOG("error loading grass!");
			}

			continue;
		}
#endif //USE_PAGED

		{ // ugly stuff to parse procedural roads
			if (!strncmp("begin_procedural_roads", line, 22))
			{
				po = ProceduralObject();
				po.loadingState = 1;
				r2oldmode = 1;
				proroad = true;
				continue;
			}
			if (!strncmp("end_procedural_roads", line, 20))
			{
				if (r2oldmode)
				{
					if (proceduralManager)
						proceduralManager->addObject(po);
					po = ProceduralObject();
				}
				proroad = false;
				continue;
			}
			if (proroad)
			{
				float rwidth, bwidth, bheight;
				//position x,y,z rotation rx,ry,rz, width, border width, border height, type
				r=sscanf(line, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %s",&ox,&oy,&oz, &rx, &ry, &rz, &rwidth, &bwidth, &bheight, oname);
				Vector3 pos=Vector3(ox, oy, oz);
				Quaternion rotation = Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);
				int roadtype=Road2::ROAD_AUTOMATIC;
				int pillartype = 0;
				if (!strcmp(oname, "flat")) roadtype=Road2::ROAD_FLAT;
				if (!strcmp(oname, "left")) roadtype=Road2::ROAD_LEFT;
				if (!strcmp(oname, "right")) roadtype=Road2::ROAD_RIGHT;
				if (!strcmp(oname, "both")) roadtype=Road2::ROAD_BOTH;
				if (!strcmp(oname, "bridge")) {roadtype=Road2::ROAD_BRIDGE;pillartype=1;}
				if (!strcmp(oname, "monorail")) {roadtype=Road2::ROAD_MONORAIL;pillartype=2;}
				if (!strcmp(oname, "monorail2")) {roadtype=Road2::ROAD_MONORAIL;pillartype=0;}
				if (!strcmp(oname, "bridge_no_pillars")) {roadtype=Road2::ROAD_BRIDGE;pillartype=0;}

				if (r2oldmode)
				{
					//fill object
					ProceduralPoint pp;
					pp.bheight = bheight;
					pp.bwidth = bwidth;
					pp.pillartype = pillartype;
					pp.position = pos;
					pp.rotation = rotation;
					pp.type = roadtype;
					pp.width = rwidth;

					po.points.push_back(pp);
				}
				continue;
			}
		} //end of the ugly (somewhat)

		strcpy(name, "generic");
		memset(oname, 0, 255);
		memset(type, 0, 255);
		memset(name, 0, 255);
		r=sscanf(line, "%f, %f, %f, %f, %f, %f, %s %s %s",&ox,&oy,&oz, &rx, &ry, &rz, oname, type, name);
		if (r<6)
			continue;
		if ((!strcmp(oname, "truck")) || (!strcmp(oname, "load") || (!strcmp(oname, "machine")) || (!strcmp(oname, "boat")) || (!strcmp(oname, "truck2")) ))
		{
			bool newFormat = (!strcmp(oname, "truck2"));

			if (!strcmp(oname, "boat") && !w)
				// no water so do not load boats!
				continue;
			String group="";
			String truckname=String(type);
			if (!CACHE.checkResourceLoaded(truckname, group))
			{
				LOG("Error while loading Terrain: truck " + String(type) + " not found. ignoring.");
				continue;
			}
			//this is a truck or load declaration
			truck_preload[truck_preload_num].px=ox;
			truck_preload[truck_preload_num].py=oy;
			truck_preload[truck_preload_num].pz=oz;
			truck_preload[truck_preload_num].freePosition = newFormat;
			truck_preload[truck_preload_num].ismachine=(!strcmp(oname, "machine"));
			truck_preload[truck_preload_num].rotation=Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);
			//truck_preload[truck_preload_num].ry=ry;
			strcpy(truck_preload[truck_preload_num].name, truckname.c_str());
			truck_preload_num++;
			continue;
		}
		if (   !strcmp(oname, "road")
			|| !strcmp(oname, "roadborderleft")
			|| !strcmp(oname, "roadborderright")
			|| !strcmp(oname, "roadborderboth")
			|| !strcmp(oname, "roadbridgenopillar")
			|| !strcmp(oname, "roadbridge"))
		{
			int pillartype = !(strcmp(oname, "roadbridgenopillar") == 0);
			//okay, this is a job for roads2
			int roadtype=Road2::ROAD_AUTOMATIC;
			if (!strcmp(oname, "road")) roadtype=Road2::ROAD_FLAT;
			Vector3 pos=Vector3(ox, oy, oz);
			Quaternion rotation;
			rotation=Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);
			if ((pos-r2lastpos).length()>20.0)
			{
				//break the road
				if (r2oldmode!=0)
				{
					//fill object
					ProceduralPoint pp;
					pp.bheight = 0.2;
					pp.bwidth = 1.4;
					pp.pillartype = pillartype;
					pp.position = r2lastpos+r2lastrot*Vector3(10.0,0,0);
					pp.rotation = r2lastrot;
					pp.type = roadtype;
					pp.width = 8;
					po.points.push_back(pp);

					// finish it and start new object
					if (proceduralManager)
						proceduralManager->addObject(po);
					po = ProceduralObject();
					r2oldmode=1;
				}
				r2oldmode=1;
				// beginning of new
				ProceduralPoint pp;
				pp.bheight = 0.2;
				pp.bwidth = 1.4;
				pp.pillartype = pillartype;
				pp.position = pos;
				pp.rotation = rotation;
				pp.type = roadtype;
				pp.width = 8;
				po.points.push_back(pp);
			}
			else
			{
				//fill object
				ProceduralPoint pp;
				pp.bheight = 0.2;
				pp.bwidth = 1.4;
				pp.pillartype = pillartype;
				pp.position = pos;
				pp.rotation = rotation;
				pp.type = roadtype;
				pp.width = 8;
				po.points.push_back(pp);
			}
			r2lastpos=pos;
			r2lastrot=rotation;


			continue;
		}
		loadObject(oname, ox, oy, oz, rx, ry, rz, bakeNode, name, true, -1, type);
	}


	//fclose(fd);

	// ds closes automatically, so do not close it explicitly here:
	//ds->close();

	// finish the last road
	if (r2oldmode != 0)
	{
		//fill object
		ProceduralPoint pp;
		pp.bheight = 0.2;
		pp.bwidth = 1.4;
		pp.pillartype = 1;
		pp.position = r2lastpos+r2lastrot*Vector3(10.0,0,0);
		pp.rotation = r2lastrot;
		pp.type = Road2::ROAD_AUTOMATIC;
		pp.width = 8;
		po.points.push_back(pp);

		// finish it and start new object
		if (proceduralManager)
			proceduralManager->addObject(po);
	}


	// okay, now bake everything
	bakesg = mSceneMgr->createStaticGeometry("bakeSG");
	bakesg->setCastShadows(true);
	bakesg->addSceneNode(bakeNode);
	bakesg->setRegionDimensions(Vector3(farclip/2.0, 10000.0, farclip/2.0));
	bakesg->setRenderingDistance(farclip);
	try
	{
		bakesg->build();
		bakeNode->detachAllObjects();
		// crash under linux:
		//bakeNode->removeAndDestroyAllChildren();
	}catch(...)
	{
		LOG("error while baking roads. ignoring.");

	}
}

void TerrainObjectManager::unloadObject(const char* instancename)
{
	if (loadedObjects.find(std::string(instancename)) == loadedObjects.end())
	{
		LOG("unable to unload object: " + std::string(instancename));
		return;
	}

	loaded_object_t obj = loadedObjects[std::string(instancename)];

	// check if it was already deleted
	if (!obj.enabled)
		return;

	// unload any collision tris
	if (obj.collTris.size() > 0)
	{
		for(std::vector<int>::iterator it = obj.collTris.begin(); it != obj.collTris.end(); it++)
		{
			collisions->removeCollisionTri(*it);
		}
	}

	// and any collision boxes
	if (obj.collBoxes.size() > 0)
	{
		for(std::vector<int>::iterator it = obj.collBoxes.begin(); it != obj.collBoxes.end(); it++)
		{
			collisions->removeCollisionBox(*it);
		}
	}

	obj.sceneNode->detachAllObjects();
	obj.sceneNode->setVisible(false);
	obj.enabled = false;
}

void TerrainObjectManager::loadObject(const char* name, float px, float py, float pz, float rx, float ry, float rz, SceneNode * bakeNode, const char* instancename, bool enable_collisions, int scripthandler, const char *type, bool uniquifyMaterial)
{
	ScopeLog log("object_"+String(name));
	if (type && !strcmp(type, "grid"))
	{
		// some fast grid object hacks :)
		for(int x=0;x<500;x+=50)
			for(int z=0;z<500;z+=50)
				loadObject(name, px+x, py, pz+z, rx, ry, rz, bakeNode, 0, enable_collisions, scripthandler, 0);
		return;
	}

	// nice idea, but too many random hits
	//if (abs(rx+1) < 0.001) rx = Math::RangeRandom(0, 360);
	//if (abs(ry+1) < 0.001) ry = Math::RangeRandom(0, 360);
	//if (abs(rz+1) < 0.001) rz = Math::RangeRandom(0, 360);

	if (strnlen(name, 250)==0)
		return;

	//FILE *fd;
	char fname[1024] = {};
	char oname[1024] = {};
	char mesh[1024] = {};
	char line[1024] = {};
	char collmesh[1024] = {};
	float scx = 0, scy = 0, scz = 0;
	float lx = 0, hx = 0, ly = 0, hy = 0, lz = 0, hz = 0;
	float srx = 0, sry = 0, srz = 0;
	float drx = 0, dry = 0, drz = 0;
	float fcx = 0, fcy = 0, fcz = 0;
	bool forcecam=false;
	bool ismovable=false;

	int event_filter = EVENT_ALL;
	Quaternion rotation = Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);

	// try to load with UID first!
	String odefgroup = "";
	String odefname = "";
	bool odefFound = false;
	if (terrainUID != "" && !CACHE.stringHasUID(name))
	{
		sprintf(fname,"%s-%s.odef", terrainUID.c_str(), name);
		odefname = String(fname);
		bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(odefname);
		if (exists)
		{	
			odefgroup = ResourceGroupManager::getSingleton().findGroupContainingResource(odefname);
			odefFound = true;
		}
	}

	if (!odefFound)
	{
		sprintf(fname,"%s.odef", name);
		odefname = String(fname);
		bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(odefname);
		if (exists)
		{
			odefgroup = ResourceGroupManager::getSingleton().findGroupContainingResource(odefname);
			odefFound = true;
		}
	}


	if (!CACHE.checkResourceLoaded(odefname, odefgroup))
		if (!odefFound)
		{
			LOG("Error while loading Terrain: could not find required .odef file: " + odefname + ". Ignoring entry.");
			return;
		}

		DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource(odefname, odefgroup);

		ds->readLine(mesh, 1023);
		if (String(mesh) == "LOD")
		{
			// LOD line is obsolete
			ds->readLine(mesh, 1023);
		}

		//scale
		ds->readLine(line, 1023);
		sscanf(line, "%f, %f, %f",&scx,&scy,&scz);
		sprintf(oname,"object%i(%s)", objcounter,name);
		objcounter++;


		SceneNode *tenode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
		bool background_loading = BSETTING("Background Loading", false);

		MeshObject *mo = NULL;
		if (String(mesh) != "none")
			mo = new MeshObject(mSceneMgr, mesh, oname, tenode, NULL, background_loading);

		//mo->setQueryFlags(OBJECTS_MASK);
		//tenode->attachObject(te);
		tenode->setScale(scx,scy,scz);
		tenode->setPosition(px,py,pz);
		tenode->rotate(rotation);
		tenode->pitch(Degree(-90));
		tenode->setVisible(true);

		// register in map
		loaded_object_t *obj = &loadedObjects[std::string(instancename)];
		obj->instanceName = std::string(instancename);
		obj->loadType     = 0;
		obj->enabled      = true;
		obj->sceneNode    = tenode;
		obj->collTris.clear();


		if (mo && uniquifyMaterial && instancename)
		{
			for(unsigned int i = 0; i < mo->getEntity()->getNumSubEntities(); i++)
			{
				SubEntity *se = mo->getEntity()->getSubEntity(i);
				String matname = se->getMaterialName();
				String newmatname = matname + "/" + String(instancename);
				//LOG("subentity " + TOSTRING(i) + ": "+ matname + " -> " + newmatname);
				se->getMaterial()->clone(newmatname);
				se->setMaterialName(newmatname);
			}
		}

		//String meshGroup = ResourceGroupManager::getSingleton().findGroupContainingResource(mesh);
		//MeshPtr mainMesh = mo->getMesh();

		//collision box(es)
		bool virt=false;
		bool rotating=false;
		bool classic_ref=true;
		// everything is of concrete by default
		ground_model_t *gm = collisions->getGroundModelByString("concrete");
		char eventname[256];
		eventname[0]=0;
		while (!ds->eof())
		{
			size_t ll=ds->readLine(line, 1023);

			// little workaround to trim it
			String lineStr = String(line);
			Ogre::StringUtil::trim(lineStr);

			const char* ptline = lineStr.c_str();
			if (ll==0 || line[0]=='/' || line[0]==';') continue;

			if (!strcmp("end",ptline)) break;
			if (!strcmp("movable", ptline)) {ismovable=true;continue;};
			if (!strcmp("localizer-h", ptline))
			{
				localizers[free_localizer].position=Vector3(px,py,pz);
				localizers[free_localizer].rotation=rotation;
				localizers[free_localizer].type=Autopilot::LOCALIZER_HORIZONTAL;
				free_localizer++;
				continue;
			}
			if (!strcmp("localizer-v", ptline))
			{
				localizers[free_localizer].position=Vector3(px,py,pz);
				localizers[free_localizer].rotation=rotation;
				localizers[free_localizer].type=Autopilot::LOCALIZER_VERTICAL;
				free_localizer++;
				continue;
			}
			if (!strcmp("localizer-ndb", ptline))
			{
				localizers[free_localizer].position=Vector3(px,py,pz);
				localizers[free_localizer].rotation=rotation;
				localizers[free_localizer].type=Autopilot::LOCALIZER_NDB;
				free_localizer++;
				continue;
			}
			if (!strcmp("localizer-vor", ptline))
			{
				localizers[free_localizer].position=Vector3(px,py,pz);
				localizers[free_localizer].rotation=rotation;
				localizers[free_localizer].type=Autopilot::LOCALIZER_VOR;
				free_localizer++;
				continue;
			}
			if (!strcmp("standard", ptline)) {classic_ref=false;tenode->pitch(Degree(90));continue;};
			if (!strncmp("sound", ptline, 5))
			{
#ifdef USE_OPENAL
				if (!SoundScriptManager::getSingleton().isDisabled())
				{
					char tmp[255]="";
					sscanf(ptline, "sound %s", tmp);
					SoundScriptInstance *sound = SoundScriptManager::getSingleton().createInstance(tmp, MAX_TRUCKS+1, tenode);
					sound->setPosition(tenode->getPosition(), Vector3::ZERO);
					sound->start();
				}
#endif //USE_OPENAL
				continue;
			}
			if (!strcmp("beginbox", ptline) || !strcmp("beginmesh", ptline))
			{
				drx=dry=drz=0.0;
				rotating=false;
				virt=false;
				forcecam=false;
				event_filter=EVENT_NONE;
				eventname[0]=0;
				collmesh[0]=0;
				gm = collisions->getGroundModelByString("concrete");
				continue;
			};
			if (!strncmp("boxcoords", ptline, 9))
			{
				sscanf(ptline, "boxcoords %f, %f, %f, %f, %f, %f",&lx,&hx,&ly,&hy,&lz, &hz);
				continue;
			}
			if (!strncmp("mesh", ptline, 4))
			{
				sscanf(ptline, "mesh %s",collmesh);
				continue;
			}
			if (!strncmp("rotate", ptline, 6))
			{
				sscanf(ptline, "rotate %f, %f, %f",&srx, &sry, &srz);
				rotating=true;
				continue;
			}
			if (!strncmp("forcecamera", ptline, 11))
			{
				sscanf(ptline, "forcecamera %f, %f, %f",&fcx, &fcy, &fcz);
				forcecam=true;
				continue;
			}
			if (!strncmp("direction", ptline, 9))
			{
				sscanf(ptline, "direction %f, %f, %f",&drx, &dry, &drz);
				continue;
			}
			if (!strncmp("frictionconfig", ptline, 14) && strlen(ptline) > 15)
			{
				// load a custom friction config
				collisions->loadGroundModelsConfigFile(String(ptline + 15));
				continue;
			}
			if ((!strncmp("stdfriction", ptline, 11) || !strncmp("usefriction", ptline, 11)) && strlen(ptline) > 12)
			{
				String modelName = String(ptline + 12);
				gm = collisions->getGroundModelByString(modelName);
				continue;
			}
			if (!strcmp("virtual", ptline)) {virt=true;continue;};
			if (!strncmp("event", ptline, 5))
			{
				char ts[256];
				ts[0]=0;
				sscanf(ptline, "event %s %s",eventname, ts);
				if (!strncmp(ts, "avatar", 6))
					event_filter=EVENT_AVATAR;
				else if (!strncmp(ts, "truck", 5))
					event_filter=EVENT_TRUCK;
				else if (!strncmp(ts, "airplane", 8))
					event_filter=EVENT_AIRPLANE;
				else if (!strncmp(ts, "delete", 8))
					event_filter=EVENT_DELETE;

				if (!strncmp(ts, "shoptruck", 9))
					terrainHasTruckShop=true;

				// fallback
				if (strlen(ts) == 0)
					event_filter=EVENT_ALL;

				continue;
			}
			//resp=sscanf(ptline, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %c",&lx,&hx,&ly, &hy,&lz, &hz, &srx, &sry, &srz,&type);
			if (!strcmp("endbox", ptline))
			{
				if (enable_collisions)
				{
					int boxnum = collisions->addCollisionBox(tenode, rotating, virt,px,py,pz,rx,ry,rz,lx,hx,ly,hy,lz,hz,srx,sry,srz,eventname, instancename, forcecam, Vector3(fcx, fcy, fcz), scx, scy, scz, drx, dry, drz, event_filter, scripthandler);
					obj->collBoxes.push_back((boxnum));
				}
				continue;
			}
			if (!strcmp("endmesh", ptline))
			{
				collisions->addCollisionMesh(collmesh, Vector3(px,py,pz), tenode->getOrientation(), Vector3(scx, scy, scz), gm, &(obj->collTris));
				continue;
			}

			if (!strncmp("particleSystem", ptline, 14) && tenode)
			{
				float x=0, y=0, z=0, scale=0;
				char pname[255]="", sname[255]="";
				int res = sscanf(ptline, "particleSystem %f, %f, %f, %f, %s %s", &scale, &x, &y, &z, pname, sname);
				if (res != 6) continue;

				// hacky: prevent duplicates
				String paname = String(pname);
				while(mSceneMgr->hasParticleSystem(paname))
					paname += "_";

				// create particle system
				ParticleSystem* pParticleSys = mSceneMgr->createParticleSystem(paname, String(sname));
				pParticleSys->setCastShadows(false);
				pParticleSys->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap

				// Some affectors may need its instance name (e.g. for script feedback purposes)
#ifdef USE_ANGELSCRIPT
				unsigned short affCount = pParticleSys->getNumAffectors();
				ParticleAffector* pAff;
				for(unsigned short i = 0; i<affCount; ++i)
				{
					pAff = pParticleSys->getAffector(i);
					if (pAff->getType()=="ExtinguishableFire")
						((ExtinguishableFireAffector*)pAff)->setInstanceName(obj->instanceName);
				}
#endif // USE_ANGELSCRIPT

				SceneNode *sn = tenode->createChildSceneNode();
				sn->attachObject(pParticleSys);
				sn->pitch(Degree(90));
				continue;
			}

			if (!strncmp("setMeshMaterial", ptline, 15))
			{
				char mat[256]="";
				sscanf(ptline, "setMeshMaterial %s", mat);
				if (mo->getEntity() && strnlen(mat,250)>0)
				{
					mo->getEntity()->setMaterialName(String(mat));
					// load it
					//MaterialManager::getSingleton().load(String(mat), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
				}
				continue;
			}
			if (!strncmp("generateMaterialShaders", ptline, 23))
			{
				char mat[256]="";
				sscanf(ptline, "generateMaterialShaders %s", mat);
				if (BSETTING("Use RTShader System", false))
				{
					Ogre::RTShader::ShaderGenerator::getSingleton().createShaderBasedTechnique(String(mat), Ogre::MaterialManager::DEFAULT_SCHEME_NAME, Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
					Ogre::RTShader::ShaderGenerator::getSingleton().invalidateMaterial(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, String(mat));
				}

				continue;
			}
			if (!strncmp("playanimation", ptline, 13) && mo)
			{
				char animname[256]="";
				float speedfactorMin = 0, speedfactorMax = 0;
				sscanf(ptline, "playanimation %f, %f, %s", &speedfactorMin, &speedfactorMax, animname);
				if (tenode && mo->getEntity() && strnlen(animname,250)>0)
				{
					AnimationStateSet *s = mo->getEntity()->getAllAnimationStates();
					if (!s->hasAnimationState(String(animname)))
					{
						LOG("ODEF: animation '" + String(animname) + "' for mesh: '" + String(mesh) + "' in odef file '" + String(name) + ".odef' not found!");
						continue;
					}
					animated_object_t ao;
					ao.node = tenode;
					ao.ent = mo->getEntity();
					ao.speedfactor = speedfactorMin;
					if (speedfactorMin != speedfactorMax)
						ao.speedfactor = Math::RangeRandom(speedfactorMin, speedfactorMax);
					ao.anim = 0;
					try
					{
						ao.anim = mo->getEntity()->getAnimationState(String(animname));
					} catch (...)
					{
						ao.anim = 0;
					}
					if (!ao.anim)
					{
						LOG("ODEF: animation '" + String(animname) + "' for mesh: '" + String(mesh) + "' in odef file '" + String(name) + ".odef' not found!");
						continue;
					}
					ao.anim->setEnabled(true);
					animatedObjects.push_back(ao);
				}
				continue;
			}
			if (!strncmp("drawTextOnMeshTexture", ptline, 21) && mo)
			{
				if (!mo->getEntity())
					continue;
				String matName = mo->getEntity()->getSubEntity(0)->getMaterialName();
				MaterialPtr m = MaterialManager::getSingleton().getByName(matName);
				if (m.getPointer() == 0)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: mesh material not found: "+String(fname)+" : "+String(ptline));
					continue;
				}
				String texName = m->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName();
				Texture* background = (Texture *)TextureManager::getSingleton().getByName(texName).getPointer();
				if (!background)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: mesh texture not found: "+String(fname)+" : "+String(ptline));
					continue;
				}

				static int textureNumber = 0;
				textureNumber++;
				char tmpTextName[256]="", tmpMatName[256]="";
				sprintf(tmpTextName, "TextOnTexture_%d_Texture", textureNumber);
				sprintf(tmpMatName, "TextOnTexture_%d_Material", textureNumber);			// Make sure the texture is not WRITE_ONLY, we need to read the buffer to do the blending with the font (get the alpha for example)
				TexturePtr texture = TextureManager::getSingleton().createManual(tmpTextName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, (Ogre::uint)background->getWidth(), (Ogre::uint)background->getHeight(), MIP_UNLIMITED , PF_X8R8G8B8, Ogre::TU_STATIC|Ogre::TU_AUTOMIPMAP, new ResourceBuffer());
				if (texture.getPointer() == 0)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: could not create texture: "+String(fname)+" : "+String(ptline));
					continue;
				}

				float x=0, y=0, w=0, h=0;
				float a=0, r=0, g=0, b=0;
				char fontname[256]="";
				char text[256]="";
				char option='l';
				int res = sscanf(ptline, "drawTextOnMeshTexture %f, %f, %f, %f, %f, %f, %f, %f, %c, %s %s", &x, &y, &w, &h, &r, &g, &b, &a, &option, fontname, text);
				if (res < 11)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: "+String(fname)+" : "+String(ptline));
					continue;
				}

				// cehck if we got a template argument
				if (!strncmp(text, "{{argument1}}", 13))
					strncpy(text, instancename, 250);

				// replace '_' with ' '
				char *text_pointer = text;
				while (*text_pointer!=0) {if (*text_pointer=='_') *text_pointer=' ';text_pointer++;};

				Font* font = (Font *)FontManager::getSingleton().getByName(String(fontname)).getPointer();
				if (!font)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: font not found: "+String(fname)+" : "+String(ptline));
					continue;
				}


				//Draw the background to the new texture
				texture->getBuffer()->blit(background->getBuffer());

				x = background->getWidth() * x;
				y = background->getHeight() * y;
				w = background->getWidth() * w;
				h = background->getHeight() * h;

				Image::Box box = Image::Box((size_t)x, (size_t)y, (size_t)(x+w), (size_t)(y+h));
				WriteToTexture(String(text), texture, box, font, ColourValue(r, g, b, a), option);

				// we can save it to disc for debug purposes:
				//SaveImage(texture, "test.png");

				m->clone(tmpMatName);
				MaterialPtr mNew = MaterialManager::getSingleton().getByName(tmpMatName);
				mNew->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(tmpTextName);

				mo->getEntity()->setMaterialName(String(tmpMatName));
				continue;
			}

			LOG("ODEF: unknown command in "+String(fname)+" : "+String(ptline));
		}

		//add icons if type is set
#ifdef USE_MYGUI
		String typestr = "";
		if (type && surveyMap)
		{
			typestr = String(type);
			// hack for raceways
			if (!strcmp(name, "chp-checkpoint"))
				typestr = "checkpoint";
			if (!strcmp(name, "chp-start"))
				typestr = "racestart";
			if (!strncmp(name, "road", 4))
				typestr = "road";

			if (typestr != String("") && typestr != String("road") && typestr != String("sign"))
			{
				MapEntity *e = surveyMap->createMapEntity(typestr);
				if (e)
				{
					e->setVisibility(true);
					e->setPosition(px, pz);
					e->setRotation(Radian(ry));

					if (String(name) != String(""))
						e->setDescription(String(instancename));
				}
			}
		}
#endif //USE_MYGUI
}

bool TerrainObjectManager::updateAnimatedObjects(float dt)
{
	if (animatedObjects.size() == 0)
		return true;
	std::vector<animated_object_t>::iterator it;
	for(it=animatedObjects.begin(); it!=animatedObjects.end(); it++)
	{
		if (it->anim && it->speedfactor != 0)
		{
			Real time = dt * it->speedfactor;
			it->anim->addTime(time);
		}
	}
	return true;
}
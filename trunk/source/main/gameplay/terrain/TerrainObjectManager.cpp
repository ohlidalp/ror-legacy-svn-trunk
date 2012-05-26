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

TerrainObjectManager::TerrainObjectManager(Ogre::SceneManager *smgr, TerrainManager *terrainManager) :
	  mSceneMgr(smgr)
	, terrainManager(terrainManager)
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

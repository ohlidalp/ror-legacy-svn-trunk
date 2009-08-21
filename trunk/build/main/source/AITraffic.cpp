#ifdef AITRAFFIC
#include "AITraffic.h"
#include "ExampleFrameListener.h"

AITraffic::AITraffic(ScriptEngine *engine)
{
	scriptengine = engine;
	initialize();
}

AITraffic::~AITraffic()
{
	if (aimatrix) delete(aimatrix);
}

void AITraffic::load()
{}

void AITraffic::initialize()
{
	aimatrix = new AITraffic_Matrix();

	mLampTimer = 0;
	mLampTimer				= 0;
	cnt						= 0;
	num_of_intersections	= 1;
//	num_of_waypoints		= 50;
	mTotalElapsedTime		= 0.0f;

/* test for intube
	Ogre::Vector3 p1(0,5,5);
	Ogre::Vector3 p2(10,5,5);
	
	if (aimatrix->inTube(p1,p2,3, Ogre::Vector3(13,5,5))) Ogre::LogManager::getSingleton().logMessage("1 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("1 - FALSE");	

	if (aimatrix->inTube(p1,p2,3, Ogre::Vector3(5,5,5))) Ogre::LogManager::getSingleton().logMessage("2 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("2 - FALSE");	

	if (aimatrix->inTube(p1,p2,3, Ogre::Vector3(5,8,5))) Ogre::LogManager::getSingleton().logMessage("3 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("3 - FALSE");	

	if (aimatrix->inTube(p1,p2,3, Ogre::Vector3(5,5,8))) Ogre::LogManager::getSingleton().logMessage("4 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("4 - FALSE");	

	if (aimatrix->inTube(p1,p2,3, Ogre::Vector3(5,9,5))) Ogre::LogManager::getSingleton().logMessage("5 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("5 - FALSE");	

	if (aimatrix->inTube(p1,p2,3, Ogre::Vector3(5,5,9))) Ogre::LogManager::getSingleton().logMessage("6 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("6 - FALSE");	

	if (aimatrix->inTube(p1,p2,3, Ogre::Vector3(-1,5,5))) Ogre::LogManager::getSingleton().logMessage("7 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("7 - FALSE");	

	exit(0);
*/
/* test for portals
	Ogre::Vector3 p1(5,5,5);
	Ogre::Vector3 p2(10,5,5);
	
	if (aimatrix->intersect(p1,p2, Ogre::Vector3(9,5,8), Ogre::Vector3(9,5,6))) Ogre::LogManager::getSingleton().logMessage("1 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("1 - FALSE");	

	if (aimatrix->intersect(p1,p2, Ogre::Vector3(8,5,6), Ogre::Vector3(8,5,2))) Ogre::LogManager::getSingleton().logMessage("2 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("2 - FALSE");	

	if (aimatrix->intersect(p1,p2, Ogre::Vector3(7,5,2), Ogre::Vector3(7,5,1))) Ogre::LogManager::getSingleton().logMessage("3 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("3 - FALSE");	

	if (aimatrix->intersect(p1,p2, Ogre::Vector3(7,5,1), Ogre::Vector3(7,5,2))) Ogre::LogManager::getSingleton().logMessage("4 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("4 - FALSE");	

	if (aimatrix->intersect(p1,p2, Ogre::Vector3(8,5,2), Ogre::Vector3(8,5,6))) Ogre::LogManager::getSingleton().logMessage("5 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("5 - FALSE");	

	if (aimatrix->intersect(p1,p2, Ogre::Vector3(9,5,6), Ogre::Vector3(9,5,8))) Ogre::LogManager::getSingleton().logMessage("6 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("6 - FALSE");	

	if (aimatrix->intersect(p1,p2, Ogre::Vector3(5,5,10), Ogre::Vector3(10,5,10))) Ogre::LogManager::getSingleton().logMessage("7 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("7 - FALSE");	

	if (aimatrix->intersect(p1,p2, Ogre::Vector3(5,5,2), Ogre::Vector3(10,5,2))) Ogre::LogManager::getSingleton().logMessage("8 - TRUE");	
	else Ogre::LogManager::getSingleton().logMessage("8 - FALSE");	

	exit(0);
*/

	// setting up waypoints
	aimatrix->trafficgrid->num_of_waypoints = 26;
	aimatrix->trafficgrid->waypoints[0].position = Ogre::Vector3(30.9446, 0.0489268, 19.8821);
	aimatrix->trafficgrid->waypoints[1].position = Ogre::Vector3(122.746, 0.050382, 19.8096);
	aimatrix->trafficgrid->waypoints[2].position = Ogre::Vector3(164.037, 0.0483047, 19.8957);
	aimatrix->trafficgrid->waypoints[3].position = Ogre::Vector3(243.493, 0.0479039, 20.0598);
	aimatrix->trafficgrid->waypoints[4].position = Ogre::Vector3(260.141, 0.0496448, 33.8395);
	aimatrix->trafficgrid->waypoints[5].position = Ogre::Vector3(260.19, 0.050381, 72.7011);
	aimatrix->trafficgrid->waypoints[6].position = Ogre::Vector3(246.288, 0.0481501, 87.535);
	aimatrix->trafficgrid->waypoints[7].position = Ogre::Vector3(165.071, 0.0466857, 87.4356);
	aimatrix->trafficgrid->waypoints[8].position = Ogre::Vector3(126.147, 0.0462679, 87.4419);
	aimatrix->trafficgrid->waypoints[9].position = Ogre::Vector3(34.419, 0.04701, 87.4518);
	aimatrix->trafficgrid->waypoints[10].position = Ogre::Vector3(19.8215, 0.0500146, 75.8752);
	aimatrix->trafficgrid->waypoints[11].position = Ogre::Vector3(19.9578, 0.0481654, 30.3277);
// zoned car

	aimatrix->trafficgrid->waypoints[12].position = Ogre::Vector3(149.997, 0.0477668, 67.8343);
	aimatrix->trafficgrid->waypoints[13].position = Ogre::Vector3(149.518, 0.00510367, 5.66228);

// outer loop

	aimatrix->trafficgrid->waypoints[14].position = Ogre::Vector3(246.168, 0.0497008, 10.1458);
	aimatrix->trafficgrid->waypoints[15].position = Ogre::Vector3(167.339, 0.0513986, 10.3141);
	aimatrix->trafficgrid->waypoints[16].position = Ogre::Vector3(126.32, 0.0521396, 10.3908);
	aimatrix->trafficgrid->waypoints[17].position = Ogre::Vector3(31.7747, 0.0491756, 10.0687);
	aimatrix->trafficgrid->waypoints[18].position = Ogre::Vector3(10.4096, 0.0466004, 27.3078);
	aimatrix->trafficgrid->waypoints[19].position = Ogre::Vector3(10.2352, 0.0505975, 72.9877);
	aimatrix->trafficgrid->waypoints[20].position = Ogre::Vector3(33.2656, 0.0547261, 92.1399);
	aimatrix->trafficgrid->waypoints[21].position = Ogre::Vector3(125.678, 0.0518688, 92.2861);
	aimatrix->trafficgrid->waypoints[22].position = Ogre::Vector3(163.513, 0.0528035, 92.2349);
	aimatrix->trafficgrid->waypoints[23].position = Ogre::Vector3(245.263, 0.0557191, 92.113);
	aimatrix->trafficgrid->waypoints[24].position = Ogre::Vector3(269.699, 0.0512372, 76.5808);
	aimatrix->trafficgrid->waypoints[25].position = Ogre::Vector3(269.89, 0.0494916, 36.956);

	// setting up segments
	aimatrix->trafficgrid->num_of_segments = 24;

	aimatrix->trafficgrid->segments[0].start			= 0;
	aimatrix->trafficgrid->segments[0].end				= 1;
	aimatrix->trafficgrid->segments[0].num_of_lanes		= 2;
	aimatrix->trafficgrid->segments[0].width			= 5;

	aimatrix->trafficgrid->segments[1].start			= 1;
	aimatrix->trafficgrid->segments[1].end				= 2;
	aimatrix->trafficgrid->segments[1].num_of_lanes		= 2;
	aimatrix->trafficgrid->segments[1].width			= 5;

	aimatrix->trafficgrid->segments[2].start			= 2;
	aimatrix->trafficgrid->segments[2].end				= 3;
	aimatrix->trafficgrid->segments[2].num_of_lanes		= 2;
	aimatrix->trafficgrid->segments[2].width			= 5;

	aimatrix->trafficgrid->segments[3].start			= 3;
	aimatrix->trafficgrid->segments[3].end				= 4;
	aimatrix->trafficgrid->segments[3].num_of_lanes		= 2;
	aimatrix->trafficgrid->segments[3].width			= 5;

	aimatrix->trafficgrid->segments[4].start			= 4;
	aimatrix->trafficgrid->segments[4].end				= 5;
	aimatrix->trafficgrid->segments[4].num_of_lanes		= 2;
	aimatrix->trafficgrid->segments[4].width			= 5;

	aimatrix->trafficgrid->segments[5].start			= 5;
	aimatrix->trafficgrid->segments[5].end				= 6;
	aimatrix->trafficgrid->segments[5].num_of_lanes		= 2;
	aimatrix->trafficgrid->segments[5].width			= 5;

	aimatrix->trafficgrid->segments[6].start			= 6;
	aimatrix->trafficgrid->segments[6].end				= 7;
	aimatrix->trafficgrid->segments[6].num_of_lanes		= 1;
	aimatrix->trafficgrid->segments[6].width			= 5;

	aimatrix->trafficgrid->segments[7].start			= 7;
	aimatrix->trafficgrid->segments[7].end				= 8;
	aimatrix->trafficgrid->segments[7].num_of_lanes		= 1;
	aimatrix->trafficgrid->segments[7].width			= 5;

	aimatrix->trafficgrid->segments[8].start			= 8;
	aimatrix->trafficgrid->segments[8].end				= 9;
	aimatrix->trafficgrid->segments[8].num_of_lanes		= 1;
	aimatrix->trafficgrid->segments[8].width			= 5;

	aimatrix->trafficgrid->segments[9].start			= 9;
	aimatrix->trafficgrid->segments[9].end				= 10;
	aimatrix->trafficgrid->segments[9].num_of_lanes		= 1;
	aimatrix->trafficgrid->segments[9].width			= 5;

	aimatrix->trafficgrid->segments[10].start			= 10;
	aimatrix->trafficgrid->segments[10].end				= 11;
	aimatrix->trafficgrid->segments[10].num_of_lanes	= 2;
	aimatrix->trafficgrid->segments[10].width			= 5;

	aimatrix->trafficgrid->segments[11].start			= 11;
	aimatrix->trafficgrid->segments[11].end				= 0;
	aimatrix->trafficgrid->segments[11].num_of_lanes	= 2;
	aimatrix->trafficgrid->segments[11].width			= 5;

	aimatrix->trafficgrid->segments[12].start			= 12;
	aimatrix->trafficgrid->segments[12].end				= 13;
	aimatrix->trafficgrid->segments[12].num_of_lanes	= 2;
	aimatrix->trafficgrid->segments[12].width			= 5;


	for (int z=13;z<25;z++)
		{
			aimatrix->trafficgrid->segments[z].start	= z+1;
			aimatrix->trafficgrid->segments[z].end		= z+2;
			if (z==24)
				aimatrix->trafficgrid->segments[z].end		= 14;

		}


	for (int z=14;z<25;z++)
		{
			aimatrix->trafficgrid->waypoints[z].num_of_connections	= 1;
			aimatrix->trafficgrid->waypoints[z].nextsegs[0].segment = z-1;
			aimatrix->trafficgrid->segments[z].start_wait = 0.0f;
			aimatrix->trafficgrid->segments[z].end_wait = 0.0f;
		}

	aimatrix->trafficgrid->waypoints[25].num_of_connections	= 1;
	aimatrix->trafficgrid->waypoints[25].nextsegs[0].segment = 14;

	for (int z=0;z<13;z++)
		{
			aimatrix->trafficgrid->waypoints[z].num_of_connections	= 1;
			aimatrix->trafficgrid->waypoints[z].nextsegs[0].segment = z;
			aimatrix->trafficgrid->segments[z].start_wait = 0.0f;
			aimatrix->trafficgrid->segments[z].end_wait = 0.0f;
		}

	// waypoints path for zoned car
	
	aimatrix->trafficgrid->waypoints[13].num_of_connections	= 0;

	// creating virtual paths for traffic

	aimatrix->trafficgrid->num_of_paths = 3;

	aimatrix->trafficgrid->paths[0].num_of_segments = 12;			// normal circular traffic
	aimatrix->trafficgrid->paths[0].path_type	= 1;		
	for (int z=0;z<12;z++) aimatrix->trafficgrid->paths[0].segments[z] = z;

	aimatrix->trafficgrid->paths[1].num_of_segments = 1;			// zoned car one segment
	aimatrix->trafficgrid->paths[1].path_type	= 1 ;		
	aimatrix->trafficgrid->paths[1].segments[0] = 12;

	aimatrix->trafficgrid->paths[2].num_of_segments = 12;			// outercircular traffic
	aimatrix->trafficgrid->paths[2].path_type	= 1;		
	for (int z=0;z<12;z++) aimatrix->trafficgrid->paths[2].segments[z] = z+13;

	// setting up zones

	aimatrix->trafficgrid->num_of_zones = 1;

	aimatrix->trafficgrid->zones[0].p1 = Ogre::Vector3(145.007, 0.0822257, 13.4135);
	aimatrix->trafficgrid->zones[0].p2 = Ogre::Vector3(123.31, 0, 24.8975);

	num_of_vehicles = 14;
	aimatrix->trafficgrid->num_of_objects = num_of_vehicles;

	aimatrix->calculateInternals();

	aimatrix->trafficgrid->segments[12].start_wait = 0.0f;
	aimatrix->trafficgrid->segments[12].end_wait = 100000.0f;

	// setting up person

	aimatrix->trafficgrid->trafficnodes[0].type = 0;
/*
	// setting up vehicles
	for (int i=1;i<NUM_OF_TRAFFICED_CARS || i<=num_of_vehicles;i++)
		{
			aimatrix->trafficgrid->trafficnodes[i].type = 1;
			aimatrix->trafficgrid->trafficnodes[i].wait = 0.0f;
			vehicles[i] = new AITraffic_Vehicle();
			vehicles[i]->serial = i;
			vehicles[i]->path_id= 0;				// inner rim car
			if (i==3) vehicles[i]->path_id = 1;		// zoned car
			if (i>8)  vehicles[i]->path_id = 2;		// outer rim car
			vehicles[i]->ps_idx = aimatrix->trafficgrid->paths[vehicles[i]->path_id].segments[0];
			vehicles[i]->aimatrix = aimatrix;
			vehicles[i]->setPosition(Ogre::Vector3(i*5,0,0) + aimatrix->trafficgrid->waypoints[aimatrix->trafficgrid->segments[vehicles[i]->ps_idx].start].position);
			if (i>8) vehicles[i]->setPosition(Ogre::Vector3(i*-10,0,0) + aimatrix->trafficgrid->waypoints[aimatrix->trafficgrid->segments[vehicles[i]->ps_idx].start].position);
			vehicles[i]->reset();

			if (i==3) 
				{
					aimatrix->trafficgrid->trafficnodes[i].active = false;
					vehicles[i]->speed = 30;
				}
			else aimatrix->trafficgrid->trafficnodes[i].active = true;
		}
*/
}

void AITraffic::frameStep(Ogre::Real deltat)
{
/*
	mLampTimer +=deltat;

//	deltat = deltat/1000.0f;
	float elapsedTime =  deltat;
	mTotalElapsedTime += deltat;

	for (int i=0;i<=num_of_vehicles;i++)
	{
		if (aimatrix->trafficgrid->trafficnodes[i].type == 1)	// if this node is a car
			{
				if (aimatrix->trafficgrid->trafficnodes[i].active)
					{
						vehicles[i]->updateSimple(elapsedTime, mTotalElapsedTime);
						aimatrix->trafficgrid->trafficnodes[i].position = vehicles[i]->getPosition();
						aimatrix->trafficgrid->trafficnodes[i].rotation = vehicles[i]->getOrientation();
					}
			}
		else if (aimatrix->trafficgrid->trafficnodes[i].type==0)	// node is person
			{
				aimatrix->trafficgrid->trafficnodes[i].rotation = playerrot; 
				aimatrix->trafficgrid->trafficnodes[i].position = playerpos; 
			}

		aimatrix->trafficgrid->trafficnodes[i].zone = aimatrix->getZone(aimatrix->trafficgrid->trafficnodes[i].position);
	
		if (aimatrix->trafficgrid->trafficnodes[i].zone!=-1)
			{
				if (aimatrix->trafficgrid->trafficnodes[i].inzone)
					{
						Ogre::LogManager::getSingleton().logMessage("Vehicle is inzone : "+Ogre::StringConverter::toString(i));
					}	// we already in the zone ... do nothing
				else
					{
						if (i==0)	// only person should trigger
							{
								aimatrix->trafficgrid->trafficnodes[i].inzone = true;
								for (int j=1;j<=num_of_vehicles;j++)
									{
										aimatrix->trafficgrid->trafficnodes[j].active = true;
									}
								vehicles[3]->speed = 30;
							}
					}
			}
		else
			{
				aimatrix->trafficgrid->trafficnodes[i].inzone = false;
			}
	}

return;
	cnt ++;
	if (cnt==1) 	
		{
			spawnObject("trafficlight3", "trafficlight-1-1", 5, 0, 5, 0, 0, 0, "trafficevent", true);
			spawnObject("trafficlight3", "trafficlight-1-2", 5, 0, 6, 0, 0, 0, "trafficevent", true);
			spawnObject("trafficlight3", "trafficlight-1-3", 5, 0, 7, 0, 0, 0, "trafficevent", true);
			spawnObject("trafficlight3", "trafficlight-1-4", 5, 0, 8, 0, 0, 0, "trafficevent", true);
			spawnObject("trafficlight3", "trafficlight-1-5", 5, 0, 9, 0, 0, 0, "trafficevent", true);
			spawnObject("trafficlight3", "trafficlight-1-6", 5, 0, 10, 0, 0, 0, "trafficevent", true);
			
			setSignalState("/trafficlight-1-1",0);
			setSignalState("/trafficlight-1-2",0);
			setSignalState("/trafficlight-1-3",0);
			setSignalState("/trafficlight-1-4",0);
			setSignalState("/trafficlight-1-5",0);
			setSignalState("/trafficlight-1-6",0);

			intersection[0].num_of_lamps = 6;
			intersection[0].lamps[0].name = "/trafficlight-1-1";
			intersection[0].lamps[1].name = "/trafficlight-1-2";
			intersection[0].lamps[2].name = "/trafficlight-1-3";
			intersection[0].lamps[3].name = "/trafficlight-1-4";
			intersection[0].lamps[4].name = "/trafficlight-1-5";
			intersection[0].lamps[5].name = "/trafficlight-1-6";

			intersection[0].length_of_program = 3;
			intersection[0].program[0] = 35;
			intersection[0].program[1] = 56;
			intersection[0].program[2] = 44;

			intersection[0].inchange = false;
			intersection[0].prg_idx = 0;
		}

	if (mLampTimer>1)
		{
			updateLamps();
			mLampTimer = 0.0f;
		}
*/
}

void AITraffic::processOneCar(int idx, float delta)
{
}

/* ============================== TRAFFIC LAMP MANAGEMENT ================================= */

void AITraffic::updateLamps()
{
	for (int k=0;k<num_of_intersections;k++)
		{
			if (intersection[k].inchange)
				{
					intersection[k].inchange = false;
					for (int i=0;i<intersection[k].num_of_lamps;i++)
						{
							if (intersection[k].lamps[i].state == intersection[k].lamps[i].nextstate)
								{
									// do nothing
								}
							else
								{
									int stat = 1;
									if (intersection[k].lamps[i].state==4)			stat = 2;
									else if (intersection[k].lamps[i].state==1)	stat = 3;
									setSignalState(intersection[k].lamps[i].name, stat);
								}
						}
				}
			else  
				{
					intersection[k].inchange = true;
					intersection[k].prg_idx++;
					if (intersection[k].prg_idx>=intersection[k].length_of_program) intersection[k].prg_idx = 0;
//					intersection.prg_idx = intersection.prg_idx % intersection.length_of_program;
//					int nextidx = (intersection.prg_idx+1) % intersection.length_of_program;

					int nextidx = intersection[k].prg_idx+1;
					if (nextidx>=intersection[k].length_of_program) nextidx = 0;

					int flag = 1;
					for (int i=0;i<intersection[k].num_of_lamps;i++)
						{
							// this should be optimized the following way:
							// we calculate only nextstate and when updating that, we use the older state 
							// to disply

							int stat = 1;
							// set the current state
							if (intersection[k].program[intersection[k].prg_idx] & flag) stat = 4;
							intersection[k].lamps[i].state = stat;
	
							setSignalState(intersection[k].lamps[i].name, stat);
				
							// calculate next state
							stat = 1;
							if (intersection[k].program[nextidx] & flag) stat = 4;
							intersection[k].lamps[i].nextstate = stat;

							flag *=2;
						}
				}
		}
}


// register traffic lamp to the system
void AITraffic::registerTrafficLamp(int id, int group_id, int role, int name)
{
}

// mode: 0 - normal, 1 - blinking, 2 - out of order			
void AITraffic::setTrafficLampGroupServiceMode(int mode)
{
}


int AITraffic::setMaterialAmbient(const std::string &materialName, float red, float green, float blue)
{
	try
	{
		Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setAmbient(red, green, blue);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

int AITraffic::setMaterialDiffuse(const std::string &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setDiffuse(red, green, blue, alpha);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

int AITraffic::setMaterialSpecular(const std::string &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setSpecular(red, green, blue, alpha);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

int AITraffic::setMaterialEmissive(const std::string &materialName, float red, float green, float blue)
{
	try
	{
		Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setSelfIllumination(red, green, blue);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

void AITraffic::spawnObject(const std::string &objectName, const std::string &instanceName, float px, float py, float pz, float rx, float ry, float rz, const std::string &eventhandler, bool uniquifyMaterials)
{
	// trying to create the new object
	ExampleFrameListener *mefl = (ExampleFrameListener *)scriptengine;
	SceneNode *bakeNode=mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode();
	mefl->loadObject(const_cast<char*>(objectName.c_str()), px, py, pz, rx, ry, rz, bakeNode, const_cast<char*>(instanceName.c_str()), true, 0, const_cast<char*>(objectName.c_str()), uniquifyMaterials);
}

void AITraffic::setSignalState(Ogre::String instance, int state)
{
	int red		= state & 1;
	int yellow	= state & 2;
	int green	= state & 4;
	// states:
	// 0 = red
	// 1 = yellow
	// 2 = green


	setMaterialAmbient("trafficlight3/top"+instance,	red,	0,	0);
	setMaterialDiffuse("trafficlight3/top"+instance,	red,	0,	0, 1);
	setMaterialEmissive("trafficlight3/top"+instance,	(red? 1:0),	0,	0);

	setMaterialAmbient("trafficlight3/middle"+instance, yellow, yellow, 0);
	setMaterialDiffuse("trafficlight3/middle"+instance, yellow, yellow, 0, 1);
	setMaterialEmissive("trafficlight3/middle"+instance, (yellow? 1:0), (yellow? 1:0), 0);

	setMaterialAmbient("trafficlight3/bottom"+instance, 0, green, 0);
	setMaterialDiffuse("trafficlight3/bottom"+instance, 0, green, 0, 1);
	setMaterialEmissive("trafficlight3/bottom"+instance, 0, (green? 1:0), 0);
}

void AITraffic::checkForPortals()
{
	// iterate 
}



#endif //AITRAFFIC
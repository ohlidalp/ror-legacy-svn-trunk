#ifdef OPENSTEER
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
	mLampTimer = 0;
	mLampTimer				= 0;
	cnt						= 0;
	num_of_intersections	= 1;
	num_of_vehicles			= 5;
	num_of_waypoints		= 50;
	mTotalElapsedTime		= 0.0f;

	for (int i=0;i<NUM_OF_TRAFFICED_CARS || i<num_of_vehicles;i++)
		{
			
			aimatrix->trafficgrid[i]->position = Ogre::Vector3(30+i*5,		0,	15);
//			trafficgrid
			vehicles[i] = new AITraffic_Vehicle();
			vehicles[i]->aimatrix = aimatrix;
			vehicles[i]->setPosition(aimatrix->trafficgrid[i]->position);
			vehicles[i]->reset();
		}
}

void AITraffic::frameStep(Ogre::Real deltat)
{
	mLampTimer +=deltat;

//	deltat = deltat/1000.0f;
	float elapsedTime =  deltat;
	mTotalElapsedTime += deltat;

	for (int i=0;i<num_of_vehicles;i++)
		{
			vehicles[i]->updateSimple(elapsedTime, mTotalElapsedTime);
			aimatrix->trafficgrid[i]->position = vehicles[i]->getPosition();
			aimatrix->trafficgrid[i]->rotation = vehicles[i]->getOrientation();
//			aimatrix->trafficgrid[i].position = Ogre::Vector3(0+i*5, 0, 15);
//			aimatrix->trafficgrid[i].rotation = Ogre::Quaternion(-1,0,0,0);
//			Ogre::Vector3 pos = trafficgrid[i].position;
//			Ogre::LogManager::getSingleton().logMessage("Passed position: "+Ogre::StringConverter::toString(i)+" "+Ogre::StringConverter::toString(pos.x)+" "+Ogre::StringConverter::toString(pos.y)+" "+Ogre::StringConverter::toString(pos.z));
			
	}
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

}

void AITraffic::processOneCar(int idx, float delta)
{

		// is it close to the driven car?
		
		// now we simply update car offsets
//		trafficgrid[idx].x1 += 0.1f;
//		trafficgrid[idx].x2 += 0.1f;
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
/*
	asIScriptModule *mod=0;
	try
	{
		mod = mse->getEngine()->GetModule("terrainScript", asGM_ONLY_IF_EXISTS);
	}catch(...)
	{
		return;
	}
	if(!mod) return;
	int functionPtr = mod->GetFunctionIdByName(eventhandler.c_str());
*/
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



#endif //OPENSTEER
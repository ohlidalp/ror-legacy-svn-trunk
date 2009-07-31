#ifdef OPENSTEER
#include "AITraffic.h"

AITraffic::AITraffic()
{
	initialize();
}

AITraffic::~AITraffic()
{
}

void AITraffic::load()
{}

void AITraffic::initialize()
{
	num_of_vehicles = 1;
	num_of_waypoints = 50;

	rs = 0.02;
	mTotalElapsedTime = 0.0f;

	for (int i=0;i<NUM_OF_TRAFFICED_CARS || i<num_of_vehicles;i++)
		{
			vehicles[i] = 0;
		}

	for (int i=0;i<NUM_OF_TRAFFICED_CARS || i<num_of_vehicles;i++)
		{
			trafficgrid[i].position = Ogre::Vector3(30.2037,		0,	15.4732);
/*
			trafficgrid[i].wpid = 0;
			trafficgrid[i].position = waypoints[trafficgrid[i].wpid];
			trafficgrid[i].rotation = turnpoints[trafficgrid[i].wpid];
*/
			vehicles[i] = new AITraffic_Vehicle();
			vehicles[i]->setPosition(trafficgrid[i].position);

		}
}

void AITraffic::frameStep(Ogre::Real deltat)
{
	deltat = deltat/1000.0f;
	float elapsedTime =  deltat;
	mTotalElapsedTime += deltat;

	for (int i=0;i<num_of_vehicles;i++)
		{
			vehicles[i]->update(elapsedTime, mTotalElapsedTime);
			Ogre::Vector3 pos = vehicles[i]->getPosition();
			Ogre::LogManager::getSingleton().logMessage("Passed position: "+Ogre::StringConverter::toString(pos.x)+" "+Ogre::StringConverter::toString(pos.y)+" "+Ogre::StringConverter::toString(pos.z));
			trafficgrid[i].position = vehicles[i]->getPosition();
		}
}

void AITraffic::processOneCar(int idx, float delta)
{
		// is it close to the driven car?
		
		// now we simply update car offsets
//		trafficgrid[idx].x1 += 0.1f;
//		trafficgrid[idx].x2 += 0.1f;
}


#endif //OPENSTEER
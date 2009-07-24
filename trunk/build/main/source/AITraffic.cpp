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

	mTotalElapsedTime = 0.0f;

	for (int i=0;i<NUM_OF_TRAFFICED_CARS;i++)
		{
			vehicles[i] = 0;
		}

	num_of_vehicles = 4;

	for (int i=0;i<NUM_OF_TRAFFICED_CARS;i++)
		{
			vehicles[i] = new AITraffic_Vehicle();
		}


	
	wpi		= 0;
	max_wpi = 12;
//	349.283, 0.199998, 191.973, 0, 135.378, 0
/*
	401.606, 0.0673499, 198.488, 0, 357.799, 0
	389.108, 0.0284055, 198.008, 0, 357.799, 0
	379.816, 0.0252295, 197.651, 0, 357.799, 0
	370.444, 0.021662, 197.29, 0, 357.799, 0
	359.901, 0.026783, 197.808, 0, 2.03928, 0
	351.041, 0.029905, 198.123, 0, 2.03928, 0
	346.432, 0.0228046, 196.944, 0, 303.483, 0
	343.651, 0.0283103, 192.74, 0, 303.483, 0
	342.215, 0.055717, 190.57, 0, 268.189, 0
	342.545, 0.0465309, 180.126, 0, 268.189, 0
	342.901, 0.0381824, 168.89, 0, 268.189, 0
	343.196, 0.0321056, 159.57, 0, 268.189, 0
	343.429, 0.0273725, 152.194, 0, 268.189, 0
*/
	waypoints[0]	= Ogre::Vector3(401.606, 0.0673499, 198.488);
	waypoints[1]	= Ogre::Vector3(389.108, 0.0284055, 198.008);
	waypoints[2]	= Ogre::Vector3(379.816, 0.0252295, 197.651);
	waypoints[3]	= Ogre::Vector3(370.444, 0.021662,	197.29);
	waypoints[4]	= Ogre::Vector3(359.901, 0.026783,	197.808);
	waypoints[5]	= Ogre::Vector3(351.041, 0.029905,	198.123);
	waypoints[6]	= Ogre::Vector3(346.432, 0.0228046, 196.944);
	waypoints[7]	= Ogre::Vector3(343.651, 0.0283103, 192.74);
	waypoints[8]	= Ogre::Vector3(342.215, 0.055717,	190.57);
	waypoints[9]	= Ogre::Vector3(342.545, 0.0465309, 180.126);
	waypoints[10]	= Ogre::Vector3(342.901, 0.0381824, 168.89);
	waypoints[11]	= Ogre::Vector3(343.196, 0.0321056, 159.57);
	waypoints[12]	= Ogre::Vector3(343.429, 0.0273725, 152.194);

	for (int i=0;i<NUM_OF_TRAFFICED_CARS;i++)
		{
		}
}

void AITraffic::frameStep(Ogre::Real deltat)
{
	float elapsedTime =  deltat;
	mTotalElapsedTime += deltat;

	for (int i=0;i<num_of_vehicles;i++)
		{
			vehicles[i]->update(elapsedTime, mTotalElapsedTime);
			//processOneCar(i, deltat);
		}
}

void AITraffic::processOneCar(int idx, float delta)
{
		// is it close to the driven car?
		
		// now we simply update car offsets
		trafficgrid[idx].x1 += 0.1f;
		trafficgrid[idx].x2 += 0.1f;
}


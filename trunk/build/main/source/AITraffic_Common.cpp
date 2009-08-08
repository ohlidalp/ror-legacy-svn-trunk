#ifdef OPENSTEER
#include "AITraffic_Common.h"

AITraffic_Matrix::AITraffic_Matrix() 
{ 
	trafficgrid = (trafficgrid_t*) malloc(sizeof(trafficgrid_t));
	num_of_objs = 6;
}

AITraffic_Matrix::~AITraffic_Matrix() 
{
	if (trafficgrid) free(trafficgrid);
}

void AITraffic_Matrix::calculateInternals()
{
	for (int i=0;i<trafficgrid->num_of_objects;i++)
		{
			trafficgrid->trafficnodes[i].active = true;
			trafficgrid->trafficnodes[i].inzone = false;
			trafficgrid->trafficnodes[i].zone	= -1;
			trafficgrid->trafficnodes[i].type	= 1;			// all object is a vehicle now
		}

	for (int i=0;i<trafficgrid->num_of_segments;i++)
		{
			trafficgrid->segments[i].offset = trafficgrid->segments[i].end-trafficgrid->segments[i].start;
			Ogre::Vector3 offset_n = trafficgrid->segments[i].offset;
			offset_n.normalise();
			trafficgrid->segments[i].dot = Ogre::Vector3(-offset_n.z, 0, -offset_n.x);
			trafficgrid->segments[i].length = trafficgrid->segments[i].offset.length();
		}
}

float AITraffic_Matrix::distanceFromLine(Ogre::Vector3 x1, Ogre::Vector3 x2, Ogre::Vector3 x0)
{
	float retfloat = 0.0f;
	Ogre::Vector3 x0x1 = x0-x1;
	Ogre::Vector3 x0x2 = x0-x2;
	Ogre::Vector3 x2x1 = x2-x1;

	float flag = 1;
	Ogre::Quaternion q = x2x1.getRotationTo(x0x1);
	if (q.getYaw()<Ogre::Radian(0)) flag =-1;


	if (x2!=x1)
		{
			x0x1 = x0x1.crossProduct(x0x2);
			retfloat = x0x1.length()/x2x1.length();
		}
	return retfloat*flag;
}

int AITraffic_Matrix::lane(int segment_idx, int p1, int p2, Ogre::Vector3 pos)
{
	int retint = -1;

	// check for idx?
	int max_lanes  = trafficgrid->segments[segment_idx].num_of_lanes;
	int lane_width = trafficgrid->segments[segment_idx].width;
	float d = distanceFromLine(trafficgrid->waypoints[p1].position, trafficgrid->waypoints[p2].position, pos);

	if (max_lanes%2==0)		// segmentline is between lanes
		{
			if (d<0) retint = 0;
			else	 retint = 1;
		}
	else
		{}					// segmentline halfs the middle lane

	
//	Ogre::LogManager::getSingleton().logMessage("LANE-N: "+Ogre::StringConverter::toString(retint));
	return retint;
}

Ogre::Vector3 AITraffic_Matrix::offsetByLane(int segment_idx, int lanenum, Ogre::Vector3 pos)
{
	Ogre::Vector3 retvector = pos;
	return retvector;
}

// we should "normalize" p1 and p2 ... so if they are given in different order,
// this should keep working

int AITraffic_Matrix::getZone(Ogre::Vector3 pos)
{
	int retint = -1;
	bool found = false;
	for (int i=0;!found && i<trafficgrid->num_of_zones;i++)
		{
			Ogre::Vector3 p2 = trafficgrid->zones[i].p2;
			if (pos.x>=p2.x && pos.z<=p2.z)
				{
					Ogre::Vector3 p1 = trafficgrid->zones[i].p1;
					if (pos.x<=p1.x && pos.z>=p1.z)
						{
							retint = i;
							found = true;
						}
				}
		}
	return retint;
}

#endif //OPENSTEER

//aimatrix->trafficgrid->zones[0].p1 = Ogre::Vector3(137.549, 0.0905831, 14.2536);
//aimatrix->trafficgrid->zones[0].p2 = Ogre::Vector3(124.868, 0.00840096, 24.0489);


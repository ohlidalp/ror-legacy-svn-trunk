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
	for (int i=0;i<trafficgrid->num_of_segments;i++)
		{
			trafficgrid->segments[i].offset = trafficgrid->segments[i].end-trafficgrid->segments[i].start;
			Ogre::Vector3 offset_n = trafficgrid->segments[i].offset;
			offset_n.normalise();
			trafficgrid->segments[i].dot = Ogre::Vector3(-offset_n.z, 0, -offset_n.x);
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

	
	Ogre::LogManager::getSingleton().logMessage("LANE-N: "+Ogre::StringConverter::toString(retint));
	return retint;
}


#endif //OPENSTEER

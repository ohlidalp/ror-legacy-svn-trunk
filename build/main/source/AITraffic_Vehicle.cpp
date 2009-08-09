#ifdef OPENSTEER

#include "AITraffic_Vehicle.h"

AITraffic_Vehicle::AITraffic_Vehicle()// : map (makeMap ()), path (makePath ())
{
	path_direction = 1;
}

AITraffic_Vehicle::~AITraffic_Vehicle()
{
}

void AITraffic_Vehicle::reset (void)
{
	wp_idx = 0;
	advanceToNextWayPoint();
	speed = 0;
}

// -------------------------------- SIMPLE AI TRAFFIC DRIVE ------------------------------------

void AITraffic_Vehicle::updateSimple(const float currentTime, const float elapsedTime)
{
	// are we in waiting position

	if (aimatrix->trafficgrid->trafficnodes[serial].wait>0.01f)
		{
			aimatrix->trafficgrid->trafficnodes[serial].wait-=currentTime;
			return;
		}

	// are we in the path-tube?
	// if so find the neares one
	// for mow we assume: yes

	if (aimatrix->lane(ps_idx, wp_prev_idx, wp_idx, getPosition())==-1)
		{
			// out of path
		}

	// are we close to the next waypoint
	// if so,update the target, means we set the 

	if (closeToWayPoint(getHeadedWayPoint(),3.0f))
		{
			advanceToNextWayPoint();
			aimatrix->trafficgrid->trafficnodes[serial].wait = aimatrix->trafficgrid->segments[ps_idx].end_wait;
		}

	int safe_follow		= calculateSafeFollowDistance();
	int brake_dist		= calculateBrakeDistance();
	int safe_distance	= objectsOnTravelPath();
	advance(currentTime);
}

int	 AITraffic_Vehicle::closestWayPoint()
{	
	int idx = 0;
	float dist = position.distance(aimatrix->trafficgrid->waypoints[0].position);
	for (int i=1;i<aimatrix->trafficgrid->num_of_waypoints;i++)
		{
			float d2 = position.distance(aimatrix->trafficgrid->waypoints[i].position);
			if (d2<dist) { dist = d2; idx = i; }
		}
	return idx;
}

float AITraffic_Vehicle::calculateSafeFollowDistance()
{
	return 10.0;
}

float  AITraffic_Vehicle::calculateBrakeDistance()
{
	return 5.0;
}

int	AITraffic_Vehicle::getHeadedWayPoint()
{
	return wp_idx;
}

int	AITraffic_Vehicle::getLeftWayPoint()
{
	return wp_prev_idx;
}

int	AITraffic_Vehicle::advanceToNextWayPoint()
{
	// we create separate function for this, since we will implement "direction" of travel later on by these
	ps_idx += path_direction;
	if (ps_idx>=aimatrix->trafficgrid->paths[path_id].num_of_segments || ps_idx<0) 
		{
			int tmp = ps_idx;
			switch(aimatrix->trafficgrid->paths[path_id].path_type)
				{
					case 0:
						active = false;		// this vehicle does not go anywhere anymore
						break;
					case 1:
						ps_idx		= 0;
						setPosition(aimatrix->trafficgrid->waypoints[aimatrix->trafficgrid->paths[path_id].segments[ps_idx]].position);
						break;
					case 2:	// not implemented yet
						path_direction = -path_direction;
						ps_idx += path_direction;
						break;
				}
		}

	int cseg = aimatrix->trafficgrid->paths[path_id].segments[ps_idx]; // current segment index
	if (path_direction>0)	// forward on path
		{
			wp_prev_idx = aimatrix->trafficgrid->segments[cseg].start;
			wp_idx		= aimatrix->trafficgrid->segments[cseg].end;
		}
	else					// backward on path
		{
			wp_prev_idx = aimatrix->trafficgrid->segments[cseg].end;
			wp_idx		= aimatrix->trafficgrid->segments[cseg].start;
		}

	Ogre::Vector3 new_wp = aimatrix->trafficgrid->waypoints[wp_idx].position;
//	new_wp = aimatrix->trafficgrid

	forward = new_wp-getPosition();
	forward.normalise();
	
	return wp_idx;
}


bool  AITraffic_Vehicle::closeToWayPoint(int idx, float r)
{
	bool retbool = false;
	if (position.distance(aimatrix->trafficgrid->waypoints[idx].position)<=r) retbool = true;
	return retbool;
}

void  AITraffic_Vehicle::advance(float deltat)
{
	position += forward * deltat * speed;
}

// this is the main function to identify all related objects in our path

float  AITraffic_Vehicle::objectsOnTravelPath()
{
	float r = 30.0f;		// distance we want sweep within
	const Ogre::Vector3		pos = getPosition();
	const Ogre::Quaternion	ori = getOrientation();

	int shield[36];
	for (int i=0;i<36;i++) shield[i] = 0;

	for (int i=0;i<aimatrix->trafficgrid->num_of_objects;i++)	// not efficient if too many objects, prefilter should be used here
	{
		if (i!=serial)
			{
				const Ogre::Vector3 target = aimatrix->trafficgrid->trafficnodes[i].position;

				if (pos.distance(target)<r)	// object within distance
					{
						Ogre::Vector3 target_diff = target-pos;
						Ogre::Quaternion sq = forward.getRotationTo(target_diff,Ogre::Vector3::UNIT_Y);
						sq.normalise();
						float yaw = sq.getYaw().valueDegrees()+180.0f;
						int idx = (int) yaw/10.0f;
						if (idx<0) idx = 0;
						else if (idx>35) idx = 35;

						shield[idx]++;
					}
			}
	}

//	return 0.0f;
	// we have shield info
	// now let's calculate where to steer or speed next 

	// check for -10..10 degrees ahead for object
	bool obs = false;

	for (int i=15;i<21;i++)
		{
			if (shield[i]) obs = true;
		}

	if (obs) speed -= 5.0;
	else speed+=0.1;
	if (speed>50) speed = 50;
	if (speed<0) speed = 0;
	return 100.0f;
}

Ogre::Vector3 AITraffic_Vehicle::getPosition()
{
//	OpenSteer::AbstractVehicle* vehicle = this;
//	return Ogre::Vector3(vehicle->position().x,vehicle->position().y,vehicle->position().z);
	return position;
}

void AITraffic_Vehicle::setPosition(Ogre::Vector3 newPos)
{
//	static_cast<AbstractVehicle*>(this)->setPosition(OpenSteer::Vec3(newPos.x, newPos.y, newPos.z));
	position = newPos;
}

Ogre::Quaternion AITraffic_Vehicle::getOrientation()
{
	Ogre::Vector3 v1(-1, 0, 0);
	Ogre::Vector3 v2(forward.x, forward.y, forward.z);
	Ogre::Quaternion retquat = v1.getRotationTo(v2, Ogre::Vector3::UNIT_Y);
	return retquat;
}

#endif //OPENSTEER

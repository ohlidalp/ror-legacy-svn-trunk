#ifdef OPENSTEER
#pragma once
#ifndef AITraffic_Common_H
#define AITraffic_Common_H

#define MAX_WAYPOINTS					100
#define MAX_SEGMENTS					100
#define MAX_PATHS						100
#define MAX_LANES						8
#define MAX_ZONES						16

#define NUM_OF_TRAFFICED_CARS			10
#define NUM_OF_TRAFFICLIGHTS			1000
#define NUM_OF_INTERSECTIONS			100
#define NUM_OF_LAMPS_PER_INTERSECTION	32			// we use one bit for a lamp
#define MAX_PROGRAM_LENGTH				16
#define MAX_SEGMENTS_PER_PATH			1000
#define MAX_CONNECTED_SEGMENTS			8

#include "Ogre.h"

typedef struct _trafficlightnode
{
	int state;				// bitwise  1-green, 2-yellow, 4-red, 128-blinking (this is current state)
	int nextstate;			// as above, but the next state is stored here (so no red-yellow-red cycle)
	int intersection_id;	// id of intersection this lamp is a part of
	int group_id;			// lane group id of the lamp
	Ogre::String name;

} trafficlightnode_t;

typedef struct _trafficintersection
{
	int num_of_lamps;
	int length_of_program;
	trafficlightnode_t lamps[NUM_OF_LAMPS_PER_INTERSECTION];
	int program[MAX_PROGRAM_LENGTH];

	bool inchange;
	int prg_idx;
} trafficintersection_t;


//---------------- Traffic Vehicle related types

typedef struct _turntype
{
	int segment;									// index of segment that is connected to this waypoint
	int start_lane;									// we can move to the new segment from this lane only (index)	
	int next_lane;									// we can arrive to this lane from this lane from <start_lane>
} turntype_t;

typedef struct _waypoint							// defines a waypoint in space
{
	Ogre::Vector3 position;							// position of waypoint
	int num_of_connections;							// number of segments connected to this waypoint
	turntype_t nextsegs[MAX_CONNECTED_SEGMENTS];	// where we can go from here		
} waypoint_t;

typedef struct _zone
{
	Ogre::Vector3 p1;
	Ogre::Vector3 p2;
} zone_t;

typedef struct _segment							// defines a segment between two waypoints
{
	int start;									// index of waypoint the segment started from
	int end;									// index of waypoint the segment ended at
	float width;								// width of the segment
	int num_of_lanes;							// how many lanes are on that segment (>=1)
	bool turnlight;								// it is needed to use turnlight? (blinking yellow)
	float length;								// length of the segment
	Ogre::Vector3 offset;						// equals end-start
	Ogre::Vector3 dot;							// vector perpendicular to the center (used for lane cration)
	float start_wait;							// time object needs to wait before starting to move on path
	float end_wait;								// time object needs to wait at the end of segment

} segment_t;

typedef struct _path							// defines an order of segments that describe a path
{
	int num_of_segments;						// how many segments are stored for this path
	int segments[MAX_SEGMENTS_PER_PATH];		// array for storing segment indexes
	int path_type;								// 0 - one way, 1 - circular, 2 ping-pong (back and forth)
} path_t;

typedef struct _trafficnode
{
	Ogre::Vector3		position;		// current position of vehicle
	Ogre::Quaternion	rotation;		// current orientation of vehicle
	Ogre::Vector3		aabb;			// axis aligned bounding box for the vehicle
	Ogre::Vector3		dimensions;		// width (x), height (y), length (z) of the vehicle
	int					type;			// register object type here (1 - vehicle, 2 - pedestrian)
	bool				active;			// if false, it is not updated
	int					zone;			// what zone it is current in (-1 if no zone)
	bool				inzone;			// used to flag procession of entrance to a zone
	float				wait;			// set this to >0 if node should wait before progressing further
	float				steer;			// steering hydro command
	float				accel;			// acceleration hydro command
	float				brake;			// brake hydro command

} trafficnode_t;

typedef struct _trafficgrid
{
	int num_of_waypoints;						// max index for waypoints
	int num_of_segments;						// max index for segments
	int num_of_paths;							// max index for paths
	int num_of_objects;							// max index for traffic objects
	int num_of_zones;

	waypoint_t		waypoints[MAX_WAYPOINTS];
	segment_t		segments[MAX_SEGMENTS];
	path_t			paths[MAX_PATHS];
	trafficnode_t	trafficnodes[NUM_OF_TRAFFICED_CARS];
	zone_t			zones[MAX_ZONES];

} trafficgrid_t;


typedef trafficlightnode_t		trafficlightgrid_t[NUM_OF_TRAFFICLIGHTS];
typedef trafficintersection_t	trafficintersectiongrid_t[NUM_OF_INTERSECTIONS];

//typedef trafficnode_t			trafficgrid_t[NUM_OF_TRAFFICED_CARS];

class AITraffic_Matrix
{
	public:
		AITraffic_Matrix();
		~AITraffic_Matrix();

		void calculateInternals();						// calculate all internal values that help calculations later
		float distanceFromLine(Ogre::Vector3 linep1, Ogre::Vector3 linep2, Ogre::Vector3 point);
		int lane(int segment_idx, int p1, int p2, Ogre::Vector3 pos);
		Ogre::Vector3 offsetByLane(int segment_idx, int lanenum, Ogre::Vector3 pos);

		int getZone(Ogre::Vector3 pos);

		trafficgrid_t *trafficgrid;
};

#endif
#endif //OPENSTEER

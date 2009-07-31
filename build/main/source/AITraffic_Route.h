#ifdef OPENSTEER
#pragma once
#ifndef AITRAFFIC_ROUTE_H
#define AITRAFFIC_ROUTE_H

#include "OpenSteer/Pathway.h"
#include "OpenSteer/Utilities.h"

class AITraffic_Route : public OpenSteer::PolylinePathway
{
public:

    AITraffic_Route (const int _pointCount,		    // construct a AITraffic_Route given the number of points (vertices), an
			 const OpenSteer::Vec3 _points[],					// array of points, an array of per-segment path radii, and a flag
             const float _radii[],					// indiating if the path is connected at the end.
             const bool _cyclic);
    

    OpenSteer::Vec3 mapPointToPath (const OpenSteer::Vec3& point, OpenSteer::Vec3& tangent, float& outside);	// override the PolylinePathway method to allow per-leg radii
    																		// Given an arbitrary point ("A"), returns the nearest point ("P") on
																			// this path.  Also returns, via output arguments, the path tangent at
																			// P and a measure of how far A is outside the Pathway's "tube".  Note
																			// that a negative distance indicates A is inside the Pathway.

    
    OpenSteer::Vec3 mapPointToPath (const OpenSteer::Vec3& point, float& outside);
    int indexOfNearestSegment (const OpenSteer::Vec3& point);							// get the index number of the path segment nearest the given point
    
    float dotSegmentUnitTangents (int segmentIndex0, int segmentIndex1);	// returns the dot product of the tangents of two path segments, 
																			// used to measure the "angle" at a path vertex: how sharp is the turn?

    
    OpenSteer::Vec3 tangentAt (const OpenSteer::Vec3& point);										// return path tangent at given point (its projection on path)

    OpenSteer::Vec3 tangentAt (const OpenSteer::Vec3& point, const int pathFollowDirection);		// return path tangent at given point (its projection on path),
																			// multiplied by the given pathfollowing direction (+1/-1 =
																			// upstream/downstream).  Near path vertices (waypoints) use the
																			// tangent of the "next segment" in the given direction

    
    bool nearWaypoint (const OpenSteer::Vec3& point);									// is the given point "near" a waypoint of this path?  ("near" == closer
																			// to the waypoint than the max of radii of two adjacent segments)

    bool isInsidePathSegment (const OpenSteer::Vec3& point, const int segmentIndex);	// is the given point inside the path tube of the given segment number?
    
	float* radii;    // per-segment radius (width) array
};


#endif
#endif //OPENSTEER

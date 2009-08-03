#ifdef OPENSTEER
#pragma once
#ifndef AITRAFFIC_ROUTE_CPP
#define AITRAFFIC_ROUTE_CPP

#include "AITraffic_Route.h"

AITraffic_Route::AITraffic_Route (	const int _pointCount,
									const OpenSteer::Vec3 _points[],
									const float _radii[],
									const bool _cyclic)
{
	initialize (_pointCount, _points, _radii[0], _cyclic);
	radii = new float [pointCount];

    for (int i = 0; i < pointCount; i++)
		{
			// copy in point locations, closing cycle when appropriate
			const bool closeCycle = cyclic && (i == pointCount-1);
			const int j = closeCycle ? 0 : i;
			points[i] = _points[j];
			radii[i] = _radii[i];
		}
}

OpenSteer::Vec3 AITraffic_Route::mapPointToPath (const OpenSteer::Vec3& point, OpenSteer::Vec3& tangent, float& outside)
{
	OpenSteer::Vec3 onPath;
    outside = FLT_MAX;

    // loop over all segments, find the one nearest to the given point
    for (int i = 1; i < pointCount; i++)
        {
			// QQQ note bizarre calling sequence of pointToSegmentDistance
            segmentLength = lengths[i];
            segmentNormal = normals[i];
            const float d =pointToSegmentDistance(point,points[i-1],points[i]);

            // measure how far original point is outside the Pathway's "tube"
            // (negative values (from 0 to -radius) measure "insideness")
            const float o = d - radii[i];

            if (o < outside)
            {
                outside = o;
                onPath = chosen;
                tangent = segmentNormal;
            }
        }
        return onPath;		// return point on path
}

OpenSteer::Vec3 AITraffic_Route::mapPointToPath (const OpenSteer::Vec3& point, float& outside)
{
	OpenSteer::Vec3 tangent;
    return mapPointToPath (point, tangent, outside);
}

int AITraffic_Route::indexOfNearestSegment (const OpenSteer::Vec3& point)
{
	int index = 0;
    float minDistance = FLT_MAX;

    // loop over all segments, find the one nearest the given point
    for (int i = 1; i < pointCount; i++)
        {
            segmentLength = lengths[i];
            segmentNormal = normals[i];
            float d = pointToSegmentDistance (point, points[i-1], points[i]);
            if (d < minDistance)
            {
                minDistance = d;
                index = i;
            }
        }
	return index;
}

float AITraffic_Route::dotSegmentUnitTangents (int segmentIndex0, int segmentIndex1)
{
	return normals[segmentIndex0].dot (normals[segmentIndex1]);
}

OpenSteer::Vec3 AITraffic_Route::tangentAt (const OpenSteer::Vec3& point)
{
	return normals [indexOfNearestSegment (point)];
}

OpenSteer::Vec3 AITraffic_Route::tangentAt (const OpenSteer::Vec3& point, const int pathFollowDirection)
{
	const int segmentIndex = indexOfNearestSegment (point);
    const int nextIndex = segmentIndex + pathFollowDirection;
    const bool insideNextSegment = isInsidePathSegment (point, nextIndex);
    const int i = (segmentIndex + (insideNextSegment ? pathFollowDirection : 0));
    return normals [i] * (float)pathFollowDirection;
}

bool AITraffic_Route::nearWaypoint (const OpenSteer::Vec3& point)
{
	for (int i = 1; i < pointCount; i++)
        {
            // return true if near enough to this waypoint
			const float r = OpenSteer::maxXXX (radii[i], radii[i+1]);
            const float d = (point - points[i]).length ();
            if (d < r) return true;
        }
        return false;
}

bool AITraffic_Route::isInsidePathSegment (const OpenSteer::Vec3& point, const int segmentIndex)
{
	const int i = segmentIndex;

	segmentLength = lengths[i];
	segmentNormal = normals[i];
	const float d = pointToSegmentDistance(point, points[i-1], points[i]);

	const float o = d - radii[i];
	return o < 0;
}

#endif
#endif //OPENSTEER

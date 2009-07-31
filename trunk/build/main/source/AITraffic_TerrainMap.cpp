#ifdef OPENSTEER
#pragma once
#ifndef AITRAFFIC_TERRAINMAP_CPP
#define AITRAFFIC_TERRAINMAP_CPP

#include "AITraffic_TerrainMap.h"

AITraffic_TerrainMap::AITraffic_TerrainMap()
{}

AITraffic_TerrainMap::AITraffic_TerrainMap (const OpenSteer::Vec3& c, float x, float z, int r)
        : center(c),
          xSize(x),
          zSize(z),
          resolution(r),
          outsideValue (false)
{
	map.reserve (resolution * resolution);
}

// destructor
AITraffic_TerrainMap::~AITraffic_TerrainMap ()
{
}

    // clear the map (to false)
void AITraffic_TerrainMap::clear (void)
{
	for (int i = 0; i < resolution; i++)
		for (int j = 0; j < resolution; j++)
			setMapBit (i, j, 0);
}


    // get and set a bit based on 2d integer map index
bool AITraffic_TerrainMap::getMapBit (int i, int j) const
{
	return map[mapAddress(i, j)];
}

bool AITraffic_TerrainMap::setMapBit (int i, int j, bool value)
{
	return map[mapAddress(i, j)] = value;
}


    // get a value based on a position in 3d world space
bool AITraffic_TerrainMap::getMapValue (const OpenSteer::Vec3& point) const
{
	const OpenSteer::Vec3 local = point - center;
	const OpenSteer::Vec3 localXZ = local.setYtoZero();

	const float hxs = xSize/2;
	const float hzs = zSize/2;

	const float x = localXZ.x;
	const float z = localXZ.z;

	const bool out = (x > +hxs) || (x < -hxs) || (z > +hzs) || (z < -hzs);

	if (out) 
        {
            return outsideValue;
        }
	else
        {
            const float r = (float) resolution; // prevent VC7.1 warning
			const int i = (int) OpenSteer::remapInterval (x, -hxs, hxs, 0.0f, r);
			const int j = (int) OpenSteer::remapInterval (z, -hzs, hzs, 0.0f, r);
            return getMapBit (i, j);
        }
}


float AITraffic_TerrainMap::minSpacing (void) const
{
	return OpenSteer::minXXX (xSize, zSize) / (float)resolution;
}

    // used to detect if vehicle body is on any obstacles
bool AITraffic_TerrainMap::scanLocalXZRectangle (const OpenSteer::AbstractLocalSpace& localSpace,
                               float xMin, float xMax,
                               float zMin, float zMax) const
{
	const float spacing = minSpacing() / 2;

	for (float x = xMin; x < xMax; x += spacing)
        {
            for (float z = zMin; z < zMax; z += spacing)
            {
                const OpenSteer::Vec3 sample (x, 0, z);
                const OpenSteer::Vec3 global = localSpace.globalizePosition (sample);
                if (getMapValue (global)) return true;
            }
        }
	return false;
}

// Scans along a ray (directed line segment) on the XZ plane, sampling
// the map for a "true" cell.  Returns the index of the first sample
// that gets a "hit", or zero if no hits found.
int AITraffic_TerrainMap::scanXZray (const OpenSteer::Vec3& origin,
                   const OpenSteer::Vec3& sampleSpacing,
                   const int sampleCount) const
{
	OpenSteer::Vec3 samplePoint (origin);

	for (int i = 1; i <= sampleCount; i++)
        {
            samplePoint += sampleSpacing;
            if (getMapValue (samplePoint)) return i;
        }

	return 0;
}


int AITraffic_TerrainMap::cellwidth (void) const 
{
	return resolution;
}  

int AITraffic_TerrainMap::cellheight (void) const 
{
	return resolution;
} 

bool AITraffic_TerrainMap::isPassable (const OpenSteer::Vec3& point) const 
{
	return ! getMapValue (point);
}

int AITraffic_TerrainMap::mapAddress (int i, int j) const 
{
	return i + (j * resolution);
}
 
#endif
#endif //OPENSTEER

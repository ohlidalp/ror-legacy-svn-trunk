#ifdef OPENSTEER
#pragma once
#ifndef AITRAFFIC_TERRAINMAP_H
#define AITRAFFIC_TERRAINMAP_H

#include "OpenSteer/SteerLibrary.h"
#include "OpenSteer/Utilities.h"

class AITraffic_TerrainMap
{
	public:
		AITraffic_TerrainMap();

		// constructor
		AITraffic_TerrainMap (const OpenSteer::Vec3& c, float x, float z, int r);

		// destructor
		~AITraffic_TerrainMap ();
  
		// clear the map (to false)
		void clear (void);

		// get and set a bit based on 2d integer map index
		bool getMapBit (int i, int j) const;
		bool setMapBit (int i, int j, bool value);

		// get a value based on a position in 3d world space
		bool getMapValue (const OpenSteer::Vec3& point) const;

		float minSpacing (void) const;

		// used to detect if vehicle body is on any obstacles
		bool scanLocalXZRectangle (const OpenSteer::AbstractLocalSpace& localSpace,
                               float xMin, float xMax,
                               float zMin, float zMax) const;
    
		// Scans along a ray (directed line segment) on the XZ plane, sampling
		// the map for a "true" cell.  Returns the index of the first sample
		// that gets a "hit", or zero if no hits found.
		int scanXZray (const OpenSteer::Vec3& origin,
                   const OpenSteer::Vec3& sampleSpacing,
                   const int sampleCount) const;

		int cellwidth (void) const;
		int cellheight (void) const;
		bool isPassable (const OpenSteer::Vec3& point) const;

	public:

		OpenSteer::Vec3 center;
		float xSize;
		float zSize;
		int resolution;

		bool outsideValue;

	private:

		int mapAddress (int i, int j) const;
		std::vector<bool> map;
};

#endif
#endif //OPENSTEER

/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

//created by Thomas Fischer 22th of January 2011

#ifndef DECALMANAGER_H__
#define DECALMANAGER_H__

#include "RoRPrerequisites.h"
#include <math.h>
#include <OgrePrerequisites.h>



class DecalSpline
{
private:
	Ogre::SceneManager *mSceneMgr;
	Ogre::ManualObject *mo_spline;
	Ogre::SceneNode *mo_spline_node, *snparent;
	Ogre::SimpleSpline *spline;


public:
	DecalSpline(Ogre::SceneManager *manager, Ogre::SceneNode *parent);
	~DecalSpline();

	int addPoint(Ogre::Vector3 v);
	int showDebugLine(bool enabled);
};

class DecalManager
{
private:
	Ogre::SceneManager *mSceneMgr;

	int addTerrainDecal(Ogre::Vector3 position, Ogre::Vector2 size, Ogre::Vector2 numSeg, Ogre::Real rotation, Ogre::String materialname, Ogre::String normalname);
	int addTerrainSplineDecal(Ogre::SimpleSpline *spline, float width, Ogre::Vector2 numSeg, Ogre::Vector2 uvSeg, Ogre::String materialname, float ground_offset, Ogre::String export_fn, bool debug);
	int finishTerrainDecal();
	Ogre::StaticGeometry *terrain_decals_sg;
	Ogre::SceneNode *terrain_decals_snode;
	int terrain_decal_count;

	// parser things
	bool decalSplineMode;
	Ogre::String splinemat, spline_export_fn;
	float spline_width, splinetex_u, splinetex_v, ground_offset;
	int spline_segments_x, spline_segments_y;

public:
	DecalManager(Ogre::SceneManager *manager);
	~DecalManager();

	int parseLine(char *line);
};


#endif // DECALMANAGER_H__

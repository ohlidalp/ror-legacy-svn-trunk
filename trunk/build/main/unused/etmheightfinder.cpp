/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#include "heightfinder.h"
//#include <OgreTerrainSceneManager.h>

#include <ETTerrainInfo.h>

using namespace Ogre;
using namespace ET;

// this is the Height-Finder for the Editable Terrain Scene Manager

ETMHeightFinder::ETMHeightFinder(ET::TerrainInfo *terrainInfo) : terrainInfo(terrainInfo)
{
}

ETMHeightFinder::~ETMHeightFinder()
{
}

float ETMHeightFinder::getHeightAt(float x, float z)
{
	return terrainInfo->getHeightAt(x, z);
}

void ETMHeightFinder::getNormalAt(float x, float z, Vector3 *result, float precision)
{
	*result = terrainInfo->getNormalAt(x, z);
}

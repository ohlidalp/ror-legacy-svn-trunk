/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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
#include "heightfinder.h"
//#include <OgreTerrainSceneManager.h>
#include <Ogre.h>
#include <OgreConfigFile.h>
#include <OgreLogManager.h>
#include <OgreStringConverter.h>

using namespace Ogre;

// this is the Height-Finder for the standart ogre Terrain Manager

TSMHeightFinder::TSMHeightFinder(char *_cfgfilename, char *fname, float defaultheight)
{
	flipped=false;
	//cfgfilename = _cfgfilename; this is a bug
	strcpy(cfgfilename, _cfgfilename);
	defaulth=defaultheight;
//	scale=tsm->getScale();
	//this is not correct, but it works
//	size=tsm->getPageSize();
    ConfigFile config;
    String val;
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	DataStreamPtr stream=rgm.openResource(cfgfilename, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    config.load( stream );
    val = config.getSetting( "PageSize" );
    if ( !val.empty() ) size=atoi(val.c_str());
    scale = Vector3::UNIT_SCALE;

    val = config.getSetting( "PageWorldX" );
    if ( !val.empty() )
        scale.x = atof( val.c_str() );

    val = config.getSetting( "MaxHeight" );
    if ( !val.empty() )
        scale.y = atof( val.c_str() );

    val = config.getSetting( "PageWorldZ" );
    if ( !val.empty() )
        scale.z = atof( val.c_str() );
    // Scale x/z relative to pagesize
    scale.x /= size - 1;
    scale.z /= size - 1;

	data=(unsigned short*)malloc(size*size*2);
	DataStreamPtr ds=rgm.openResource(fname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	ds->read(data, size*size*2);
	// ds closes automatically, so do not close it explicitly here
	loadSettings();
}

void TSMHeightFinder::loadSettings()
{
	ConfigFile cfg;
	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(String(cfgfilename), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	cfg.load(ds, "\t:=", false);
	flipped=(cfg.getSetting("Heightmap.flip")=="true");
	LogManager::getSingleton().logMessage("loading HeightFinder configuration from " + String(cfgfilename) + " flipped: " + StringConverter::toString(flipped));
}

TSMHeightFinder::~TSMHeightFinder()
{
	if (data) free(data);
}

float TSMHeightFinder::getHeightAt(float x, float z)
{
	float rx=x/scale.x;
	float rz=z/scale.z;
	float irx, irz;
	float x_pct=modff(rx, &irx);
	float z_pct=modff(rz, &irz);
	int x_index=(int)irx;
	int z_index=(int)irz;
	if (irx<0 || irz<0 || irx>=size-1 || irz>=size-1) return defaulth;

	//dx=irx; dz=irz;
	float t1=0, t2=0, b1=0, b2=0;
	if(flipped)
	{
		t1 = data[x_index+re(z_index)*size];
		t2 = data[x_index+1+re(z_index)*size];
		b1 = data[x_index+re(z_index+1)*size];
		b2 = data[x_index+1+re(z_index+1)*size];
	} 
	 else
	{
		t1 = data[x_index+(z_index)*size];
		t2 = data[x_index+1+(z_index)*size];
		b1 = data[x_index+(z_index+1)*size];
		b2 = data[x_index+1+(z_index+1)*size];
	}

	//		dx=irx; dz=x_pct;

	float midpoint = (b1 + t2) / 2.0;

	if (x_pct + z_pct <= 1) {
		b2 = midpoint + (midpoint - t1);
	} else {
		t1 = midpoint + (midpoint - b2);
	}

	float t = ( t1 * ( 1 - x_pct ) ) + ( t2 * ( x_pct ) );
	float b = ( b1 * ( 1 - x_pct ) ) + ( b2 * ( x_pct ) );

	float h = (( t * ( 1 - z_pct ) ) + ( b * ( z_pct ) ))*scale.y/65535.0;


	if (h<defaulth) h=defaulth;
	return h;
}

void TSMHeightFinder::getNormalAt(float x, float z, Vector3 *result, float precision)
{
	Vector3 here, left, down;
	here.x = x;
	here.y = getHeightAt( x, z );
	here.z = z;

	left.x = x - precision;
	left.y = getHeightAt( x - precision, z );
	left.z = z;

	down.x = x;
	down.y = getHeightAt( x, z + precision );
	down.z = z + precision;

	left = left - here;

	down = down - here;

	left.normalise();
	down.normalise();

	*result = left.crossProduct( down );
	result -> normalise();
}

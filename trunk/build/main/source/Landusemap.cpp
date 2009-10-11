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
#include "Landusemap.h"
#include "collisions.h"
#include <Ogre.h>
#include <OgreConfigFile.h>
#include <OgreLogManager.h>
#include <OgreStringConverter.h>
#include "language.h"
#include "errorutils.h"

using namespace Ogre;

// this is the Height-Finder for the standart ogre Terrain Manager

Landusemap::Landusemap(String cfgfilename, Collisions *c, Real _mapsizex, Real _mapsizez) : version(0)
{
	configFilename = cfgfilename;
	coll = c;
	mapsizex=_mapsizex;
	mapsizez=_mapsizez;
	loadSettings();
}

void Landusemap::loadSettings()
{
	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(configFilename, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	int linecounter = -1;
	char line[1024]="";
	usemap.clear();

	LogManager::getSingleton().logMessage("Parsing '"+configFilename+"'");
	int section=-1;
	while (!ds->eof())
	{
		linecounter++;
		size_t ll=ds->readLine(line, 1023);
		if (!line || ll==0 || line[0]==';' || line[0]=='/' || !strnlen(line, 255))
			continue;
		if(!strcmp(line, "config"))
		{
			section=1;
			continue;
		} else if(!strcmp(line, "ground-models"))
		{
			section=-1;
			// this section is obsolete
			continue;
		} else if(!strcmp(line, "use-map"))
		{
			if(version != LATEST_GROUND_MODEL_VERSION)
			{
				// warning
				showError(_L("Configuration error"), _L("This map's ground model config is too old, please update it. Will use defaults!"));
				// we ignore the section
				section=-1;
			} else
			{
				section=3;
			}
			continue;
		}
		if(section == 1)
		{
			// config section
			std::vector<Ogre::String> args = StringUtil::split(String(line), "=");
			if(args.size() != 2)
			{
				LogManager::getSingleton().logMessage("invalid config line in " + configFilename + " line " + StringConverter::toString(linecounter));
				continue;
			}
			String key = args[0];
			String value = args[1];
			StringUtil::toLowerCase(key);
			StringUtil::trim(key);
			StringUtil::trim(value);

			if(key == "texture")
				textureFilename = value;
			else if(key == "frictionconfig")
				coll->loadGroundModelsConfigFile(value);
			else if(key == "defaultuse")
				default_ground_model = coll->getGroundModelByString(value);
			else if(key == "version")
				version = StringConverter::parseInt(value);

		} else if(section == 2)
		{
			// obsolete section, replaced with config value "frictionconfig"
		} else if(section == 3)
		{
			// use map
			std::vector<Ogre::String> args = StringUtil::split(String(line));
			if(args.size() != 2)
			{
				LogManager::getSingleton().logMessage("invalid use line in " + configFilename + " line " + StringConverter::toString(linecounter));
				continue;
			}
			if(args[0][0] != '#')
			{
				LogManager::getSingleton().logMessage("invalid use line in " + configFilename + " line " + StringConverter::toString(linecounter));
				continue;
			}
			char *ptr;
			String fmt = "0xff"+args[0].substr(1);
			uint32 color = strtoul(fmt.c_str(), &ptr, 16);
			String use = args[1];
			usemap[color] = use;
		}

	}
	Forests::ColorMap *colourMap = Forests::ColorMap::load(textureFilename, CHANNEL_COLOR);
	colourMap->setFilter(Forests::MAPFILTER_NONE);
	Ogre::TRect<Ogre::Real> bounds = TBounds(0, 0, mapsizex, mapsizez);

	/*
	// debug things below
	printf("found ground use definitions:\n");
	for(std::map < uint32, String >::iterator it=usemap.begin(); it!=usemap.end(); it++)
	{
		printf(" 0x%Lx : %s\n", it->first, it->second.c_str());
	}
	*/

	bool bgr = colourMap->getPixelBox().format == PF_A8B8G8R8;

	// now allocate the data buffer to hold pointers to ground models
	data=(unsigned long long*)malloc(mapsizex*mapsizez*sizeof(unsigned long long));
	unsigned long long *ptr = data;
	//std::map < String, int > counters;
	for(int z=0; z<mapsizez; z++)
	{
		for(int x=0; x<mapsizex; x++)
		{
			uint32 col = colourMap->getColorAt(x, z, bounds);
			if (bgr)
			{
				// Swap red and blue values
				uint32 cols = col & 0xFF00FF00;
				cols |= (col & 0xFF) << 16;
				cols |= (col & 0xFF0000) >> 16;
				col = cols;
			}
			String use = usemap[col];
			//if(use!="")
			//	counters[use]++;

			// store the pointer to the ground model in the data slot
			*ptr = (unsigned long long)coll->getGroundModelByString(use);
			ptr++;
		}
	}

	/*
	// debug things below
	printf("used ground models:\n");
	for(std::map < String, int >::iterator it=counters.begin(); it!=counters.end(); it++)
	{
		printf(" %s : %d\n", it->first.c_str(), it->second);
	}
	*/

}

Landusemap::~Landusemap()
{
	if (data) free(data);
}

ground_model_t *Landusemap::getGroundModelAt(int x, int z)
{
	// we return the default ground model if we are not anymore in this map
	if(x<0 || x > mapsizex || z<0 || z>mapsizez)
		return default_ground_model;

	return (ground_model_t *)(data + x + z * (int)mapsizex);
}


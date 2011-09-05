/*
This source file is part of Rigs of Rods
Copyright 2005-2011 Pierre-Michel Ricordel
Copyright 2007-2011 Thomas Fischer

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
//created by thomas fischer 23 February 2009
#ifndef SAVEGAME_H__
#define SAVEGAME_H__

#include "RoRPrerequisites.h"

class Savegame
{
public:
	Savegame();
	~Savegame();

	int load(Ogre::String &filename);
	int save(Ogre::String &filename);
protected:
	static const char *current_version;

	void logMessage(Ogre::String m);

	// data structres for the file format below
	struct savegame_header {
		char savegame_version[28];
		char ror_version[20];
		unsigned int entries;
		int current_truck;
		float player_pos_x;
		float player_pos_y;
		float player_pos_z;
		float cam_pos_x;
		float cam_pos_y;
		float cam_pos_z;
	};
	struct savegame_entry_header {
		char filename[1024];
		unsigned int buffersize;
		unsigned int free_nodes;
		unsigned int free_beams;
		unsigned int free_shock;
		unsigned int free_wheel;
		unsigned int free_hooks;
		unsigned int free_rotator;
		unsigned int state;
		unsigned int engine;
		float origin[3];
	};
	struct savegame_entry_data_engine {
		// TODO
	};

	struct savegame_entry_data_node {

	};
};

#endif //SAVEGAME_H__

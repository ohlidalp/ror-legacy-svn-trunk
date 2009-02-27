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
// created by thomas fischer 18th of January 2009

#ifndef NETWORKINFO_H
#define NETWORKINFO_H

// this struct exists in a separate file to remove cyclic dependency between components

typedef struct
{
	bool          used;
	int           trucknum;
	bool          loaded;
	bool          invisible;

	char          client_version[10];
	char          protocol_version[10];
	char          truck_name[255];
	unsigned int  truck_size;
	char          user_language[10];
	char          user_name[20];
	unsigned int  user_id;
	unsigned int  user_level;
} client_t;

#endif

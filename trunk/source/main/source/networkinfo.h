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
	bool          used;                 //!< if this slot is used already
	int           trucknum;             //!< the truck's number of the truck bound to this slot
	int           slotnum;              //!< server slot number
	bool          loaded;               //!< if the truck bound to this slot is loaded
	bool          invisible;            //!< if the truck is invisible for the user (since he dont have the required resource)

	char          client_version[10];   //!< clients version, i.e. RoR-0.36.1
	char          protocol_version[10]; //!< protocol version, i.e. RoRNet-3.0
	char          truck_name[255];      //!< the truck filename, i.e. agoras.truck
	unsigned int  truck_size;           //!< the size truck buffer needed
	char          user_language[10];    //!< the users language
	char          user_name[20];        //!< the users nickname
	unsigned int  user_id;              //!< the users id, set by the server
	unsigned int  user_authlevel;       //!< the users authorization level.
	int  colournum;                        //!< the users authorization colour
} client_t;

#endif

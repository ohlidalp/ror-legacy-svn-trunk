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

#ifndef RORNETV2_H__
#define RORNETV2_H__

#define MAX_CLIENTS 10
#define SERVER_PORT 31117

#include <iostream>
#include <ctime>
#include <cstdlib>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "RakNetworkFactory.h"

#define ROR_DATA_MSG ID_USER_PACKET_ENUM
#define RORNETv2_VERSION "RoRnet3.0.0"

typedef struct
{
	int time;
	float engine_speed;
	float engine_force;
	unsigned int flagmask;
} oob2_t;

enum {
	MSG3_VERSION = 1,
	MSG3_USE_VEHICLE,
	MSG3_BUFFER_SIZE,
	MSG3_VEHICLE_DATA,
	MSG3_HELLO,
	MSG3_USER_CREDENTIALS,
	MSG3_TERRAIN_RESP,
	MSG3_SERVER_FULL,
	MSG3_USER_BANNED,
	MSG3_WRONG_SERVER_PW,
	MSG3_WELCOME,
	MSG3_CHAT,
	MSG3_DELETE,
	MSG3_GAME_CMD
};



using namespace std;

#endif



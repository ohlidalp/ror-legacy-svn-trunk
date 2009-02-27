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

#include <iostream>
#include <ctime>
#include <cstdlib>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "RakNetworkFactory.h"

#define ROR_DATA_MSG ID_USER_PACKET_ENUM
#define RORNETv2_VERSION "RoRnet3.0.0"
#define MAX_MESSAGE_LENGTHv2 8192

typedef struct net_oob2
{
	int time;
	float engine_speed;
	float engine_force;
	unsigned int flagmask;
} oob2_t;

// note: never ever put std::string in any of those structs that go over the net (since we memset the structs)
typedef struct net_userinfo
{
	// client-server data
	char          server_password[50];
	char          client_version[20];
	char          protocol_version[20];
	
	// truck data
	char          truck_name[256];
	unsigned int  truck_size;
	
	// user data
	char          user_language[11];
	char          user_token[51];
	char          user_name[21];

	// fields overwritten by the server
	unsigned int  user_level;
	unsigned int  user_id;
} net_userinfo_t;

typedef struct net_serverinfo
{
	char          server_version[20];
	char          protocol_version[20];
	char          terrain_name[256];
} net_serverinfo_t;

enum {
	MSG3_VERSION = 1,
	MSG3_USER_INFO,
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
	MSG3_GAME_CMD,
	MSG3_SERVER_INFO,
};



using namespace std;

#endif



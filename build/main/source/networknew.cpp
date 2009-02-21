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
// created by thomas fischer 18th of january 2009
#ifdef NEWNET

#include <stdio.h>
#include <string.h>

#include "networknew.h"
#include "ExampleFrameListener.h"
#include "ColoredTextAreaOverlayElement.h"
#include "IngameConsole.h"
#include "CacheSystem.h"
#include "turboprop.h"
#include "sha1.h"
#include "Settings.h"



#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "GetTime.h"
#include "Settings.h"
#include "sha1.h"
#include "nethelper.h"

using namespace std;
using namespace Ogre;
using namespace RakNet;

char MSG3_NAMES[255][255] = {"ERROR", "MSG3_VERSION", "MSG3_USE_VEHICLE", "MSG3_BUFFER_SIZE", "MSG3_VEHICLE_DATA", "MSG3_HELLO", "MSG3_USER_CREDENTIALS", "MSG3_TERRAIN_RESP", "MSG3_SERVER_FULL", "MSG3_USER_BANNED", "MSG3_WRONG_SERVER_PW", "MSG3_WELCOME", "MSG3_CHAT", "MSG3_DELETE", "MSG3_GAME_CMD"};

NetworkNew *net_new_instance; // workaround for thread entry points

void *s_new_sendthreadstart(void* vid)
{
	net_new_instance->sendthreadstart();
	return NULL;
}

void *s_new_receivethreadstart(void* vid)
{
	net_new_instance->receivethreadstart();
	return NULL;
}



NetworkNew::NetworkNew(Beam **btrucks, Ogre::String servername, long sport, ExampleFrameListener *efl) : NetworkBase(btrucks, servername, sport, efl), peer(0)
{
	net_new_instance = this; // workaround for thread entry points
	trucks=btrucks;
	mefl=efl;

	LogManager::getSingleton().logMessage("NetworkNew::NetworkNew()");
	for (int i=0; i<MAX_PEERS; i++) clients[i].used=false;
	
	serverAddress = UNASSIGNED_SYSTEM_ADDRESS;
	last_time = 0;
	soundManager = SoundScriptManager::getSingleton();
	strcpy(terrainName, "");
	strcpy(ourTruckname, "");
	myuid = -1;
	send_buffer = 0;

	pthread_mutex_init(&send_work_mutex, NULL);
	pthread_cond_init(&send_work_cv, NULL);
	pthread_mutex_init(&clients_mutex, NULL);
	pthread_mutex_init(&chat_mutex, NULL);

	myServerName = servername;
	myServerPort = sport;
	shutdown = false;
}

NetworkNew::~NetworkNew()
{
	LogManager::getSingleton().logMessage("NetworkNew::~NetworkNew()");
	pthread_mutex_destroy(&send_work_mutex);
	pthread_cond_destroy(&send_work_cv);
}

void NetworkNew::netFatalError(String errormsg, bool exitProgram)
{
	if(shutdown)
		return;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	String err = "Network fatal error: "+errormsg;
	MessageBox( NULL, err.c_str(), "Network Connection Problem", MB_OK | MB_ICONERROR | MB_TOPMOST);
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#endif

	LogManager::getSingleton().logMessage("NET FATAL ERROR: " + errormsg);
	if(exitProgram)
		exit(124);
}

bool NetworkNew::connect()
{
	LogManager::getSingleton().logMessage("NetworkNew::connect()");

    peer = RakNetworkFactory::GetRakPeerInterface();
    
	Packet *packet;
    peer->Startup(1, 30, &SocketDescriptor(), 1);
	peer->Connect(myServerName.c_str(), myServerPort, 0, 0);

	//start the handling threads
	pthread_create(&receivethread, NULL, s_new_receivethreadstart, (void*)(0));

	int t=timer.getMilliseconds();
	while(timer.getMilliseconds()-t<5000 && myuid == -1)
	{
		Sleep(10);
	}
	if(myuid == -1)
		netFatalError("error getting user id");

	return true;
}

void NetworkNew::sendVehicleType(char* name, int buffersize)
{
	LogManager::getSingleton().logMessage("NetworkNew::sendVehicleType()");
	
	strcpy(ourTruckname, name);
	//lets send our vehicle name
	LogManager::getSingleton().logMessage("sending truckname: '"+String(name)+"'...");
	sendmessage(peer, serverAddress, MSG3_USE_VEHICLE, myuid, strlen(name), name);
	
	//send the buffer size
	send_buffer_len=buffersize;
	unsigned int bsize=sizeof(oob_t)+send_buffer_len;
	send_buffer=(char*)malloc(send_buffer_len);
	
	sendmessage(peer, serverAddress, MSG3_BUFFER_SIZE, myuid, 4, (char*)(&bsize));
	
	//start the handling threads
	pthread_create(&sendthread, NULL, s_new_sendthreadstart, (void*)(0));
	//pthread_create(&receivethread, NULL, s_new_receivethreadstart, (void*)(0));

}

//this is called at each frame to check if a new vehicle must be spawn
bool NetworkNew::vehicle_to_spawn(char* name, unsigned int *uid, unsigned int *label)
{
	//LogManager::getSingleton().logMessage("NetworkNew::vehicle_to_spawn()");
	pthread_mutex_lock(&clients_mutex);
	for (int i=0; i<MAX_PEERS; i++)
	{
		if (clients[i].used && !clients[i].loaded && !clients[i].invisible)
		{
			strcpy(name, clients[i].vehicle);
			*uid=clients[i].uid;
			*label=i;
			pthread_mutex_unlock(&clients_mutex);
			return true;
		}
	}
	pthread_mutex_unlock(&clients_mutex);
	return false;
}

client_t NetworkNew::vehicle_spawned(unsigned int uid, int trucknum)
{
	LogManager::getSingleton().logMessage("NetworkNew::vehicle_to_spawn()");
	client_t return_client;
	pthread_mutex_lock(&clients_mutex);
	for (int i=0; i<MAX_PEERS; i++)
	{
		if (clients[i].uid==uid)
		{
			clients[i].loaded=true;
			clients[i].trucknum=trucknum;
			//ret=clients[i].nickname;
			return_client = clients[i];
		}
	}
	pthread_mutex_unlock(&clients_mutex);
	return return_client;
}

void NetworkNew::sendData(Beam* truck)
{
	if(serverAddress == UNASSIGNED_SYSTEM_ADDRESS)
		// not yet connected
		return;
	int t=timer.getMilliseconds();
	if (t-last_time>100)
	{
		// 100 ms passed, send some new data

		memset(send_buffer, 0, send_buffer_len);
		last_time = t;

		//copy data in send_buffer, send_buffer_len
		if (send_buffer==0)
		{
			//boy, thats soo bad
			return;
		}
		int i;
		Vector3 refpos=truck->nodes[0].AbsPosition;
		((float*)send_buffer)[0]=refpos.x;
		((float*)send_buffer)[1]=refpos.y;
		((float*)send_buffer)[2]=refpos.z;
		short *sbuf=(short*)(send_buffer+4*3);
		for (i=1; i<truck->first_wheel_node; i++)
		{
			Vector3 relpos=truck->nodes[i].AbsPosition-refpos;
			sbuf[(i-1)*3]   = (short int)(relpos.x*300.0);
			sbuf[(i-1)*3+1] = (short int)(relpos.y*300.0);
			sbuf[(i-1)*3+2] = (short int)(relpos.z*300.0);
		}
		float *wfbuf=(float*)(send_buffer+truck->nodebuffersize);
		for (i=0; i<truck->free_wheel; i++)
		{
			wfbuf[i]=truck->wheels[i].rp;
		}
		send_oob.time=t;
		if (truck->engine)
		{
			send_oob.engine_speed=truck->engine->getRPM();
			send_oob.engine_force=truck->engine->getAcc();
		}
		if(truck->free_aeroengine>0)
		{
			float rpm = truck->aeroengines[0]->getRPM();
			send_oob.engine_speed=rpm;
		}

		send_oob.flagmask=0;
		// update horn
		if (soundManager->getTrigState(truck->trucknum, SS_TRIG_HORN))
			send_oob.flagmask+=NETMASK_HORN;

		// update particle mode
		if (truck->getCustomParticleMode())
			send_oob.flagmask+=NETMASK_PARTICLE;

		// update lights and flares and such
		if (truck->lights)
			send_oob.flagmask+=NETMASK_LIGHTS;

		blinktype b = truck->getBlinkType();
		if (b==BLINK_LEFT)
			send_oob.flagmask+=NETMASK_BLINK_LEFT;
		else if (b==BLINK_RIGHT)
			send_oob.flagmask+=NETMASK_BLINK_RIGHT;
		else if (b==BLINK_WARN)
			send_oob.flagmask+=NETMASK_BLINK_WARN;

		if (truck->getCustomLightVisible(0))
			send_oob.flagmask+=NETMASK_CLIGHT1;
		if (truck->getCustomLightVisible(1))
			send_oob.flagmask+=NETMASK_CLIGHT2;
		if (truck->getCustomLightVisible(2))
			send_oob.flagmask+=NETMASK_CLIGHT3;
		if (truck->getCustomLightVisible(3))
			send_oob.flagmask+=NETMASK_CLIGHT4;

		if (truck->getBrakeLightVisible())
			send_oob.flagmask+=NETMASK_BRAKES;
		if (truck->getReverseLightVisible())
			send_oob.flagmask+=NETMASK_REVERSE;
		if (truck->getBeaconMode())
			send_oob.flagmask+=NETMASK_BEACONS;

//		if (truck->ispolice && truck->audio->getPoliceState())
//			send_oob.flagmask+=NETMASK_POLICEAUDIO;

		//netlock=truck->netlock;
//LogManager::getSingleton().logMessage("Sending data");
		//unleash the gang
		pthread_mutex_lock(&send_work_mutex);
		pthread_cond_broadcast(&send_work_cv);
		pthread_mutex_unlock(&send_work_mutex);
	}
}

void NetworkNew::sendthreadstart()
{
	LogManager::getSingleton().logMessage("Sendthread starting");
	while (!shutdown)
	{
		//wait signal
		pthread_mutex_lock(&send_work_mutex);
		pthread_cond_wait(&send_work_cv, &send_work_mutex);
		pthread_mutex_unlock(&send_work_mutex);
		//send data
		if (send_buffer)
		{
			int blen = send_buffer_len+sizeof(oob_t);
			//LogManager::getSingleton().logMessage("sending data: " + StringConverter::toString(blen));
			memcpy(sendthreadstart_buffer, (char*)&send_oob, sizeof(oob_t));
			memcpy(sendthreadstart_buffer+sizeof(oob_t), (char*)send_buffer, send_buffer_len);
			sendmessage(peer, serverAddress, MSG3_VEHICLE_DATA, myuid, blen, sendthreadstart_buffer);
		}
	}
}

void NetworkNew::sendChat(char* line)
{
	int etype=sendmessage(peer, serverAddress, MSG3_CHAT, myuid, (int)strlen(line), line);
	if (etype)
	{
		char emsg[256];
		sprintf(emsg, "Error %i while sending chat packet", etype);
		netFatalError(emsg);
		return;
	}
}

void NetworkNew::receivethreadstart()
{
	Packet *packet;

	char *buffer=(char*)malloc(MAX_MESSAGE_LENGTH);
    while (!shutdown)
    {
        packet=peer->Receive();

        if (packet)
        {
			int packetID = getPacketIdentifier(packet);
			LogManager::getSingleton().logMessage("NET| got packet: "+StringConverter::toString(packetID));
            switch (packetID)
            {
                case ID_CONNECTION_ATTEMPT_FAILED:
                    LogManager::getSingleton().logMessage("NET| connection failed");
                    return; // 1;
                    break;
                case ID_REMOTE_DISCONNECTION_NOTIFICATION:
                    LogManager::getSingleton().logMessage("NET| Another client has disconnected.");
                    break;
                case ID_REMOTE_CONNECTION_LOST:
                    LogManager::getSingleton().logMessage("NET| Another client has lost the connection.");
                    break;
                case ID_REMOTE_NEW_INCOMING_CONNECTION:
                    LogManager::getSingleton().logMessage("NET| Another client has connected.");
                    break;
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    LogManager::getSingleton().logMessage("NET| Our connection request has been accepted.");
					serverAddress = packet->systemAddress;
                    break;
                
                case ROR_DATA_MSG:
                {
                    // The false is for efficiency so we don't make a copy of the passed data
					BitStream stream(packet->data, packet->length, false);
					
					// read basic stuff
					unsigned char useTimeStamp=0, typeId=0;
					unsigned long timeStamp=0, size=0;
					unsigned char contentType=0, contentSource=0;
					unsigned long contentSize;
					int mySize=0;
					stream.Read(useTimeStamp);
					stream.Read(timeStamp);
					stream.Read(typeId); // should be ROR_DATA_MSG
					stream.Read(contentType);
					stream.Read(contentSource);
					stream.Read(contentSize);

					// clear the buffer, important...
					memset(buffer, 0, MAX_MESSAGE_LENGTH);

					// fill buffer
                    stream.SerializeBits(false, (unsigned char*)buffer, contentSize*8);

					printf("GOT message %s (%d) (%d bytes) from client %d (%s)\n", MSG3_NAMES[contentType], contentType, contentSize, contentSource, packet->guid.ToString());
					this->handlePacket(contentType, contentSource, contentSize, buffer);                    
                }
                break;

                case ID_NEW_INCOMING_CONNECTION:
                    LogManager::getSingleton().logMessage("NET| A connection is incoming.");
                    break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                    LogManager::getSingleton().logMessage("NET| The server is full.");
					shutdown=true;
                    break;
                case ID_DISCONNECTION_NOTIFICATION:
                    LogManager::getSingleton().logMessage("NET| We have been disconnected.");
					shutdown=true;
                    break;
                case ID_CONNECTION_LOST:
                    LogManager::getSingleton().logMessage("NET| Connection lost.");
					shutdown=true;
                    break;
                default:
					LogManager::getSingleton().logMessage("NET| Message with identifier " + StringConverter::toString(packet->data[0]) + " has arrived.");
                    break;
            }
            peer->DeallocatePacket(packet);
        }
    }
    //RakNetworkFactory::DestroyRakPeerInterface(peer);
}

void NetworkNew::handlePacket(unsigned char type, unsigned char source, unsigned long wrotelen, char *buffer)
{
	if (type==MSG3_VEHICLE_DATA)
	{
		//we must update a vehicle
		//find which vehicle it is
		//then call pushNetwork(buffer, wrotelen)
		pthread_mutex_lock(&clients_mutex);
		for (int i=0; i<MAX_PEERS; i++)
		{
			if (clients[i].used && !clients[i].invisible && clients[i].uid==source && clients[i].loaded)
			{
				//okay
				trucks[clients[i].trucknum]->pushNetwork(buffer, wrotelen);
//		LogManager::getSingleton().logMessage("Id like to push to "+StringConverter::toString(clients[i].trucknum));
				break;
			}
		}
		pthread_mutex_unlock(&clients_mutex);
	} else if (type == MSG3_WELCOME)
	{
		unsigned short *uid = (unsigned short *)buffer;
		myuid = *uid;
		LogManager::getSingleton().logMessage("userid is now " + StringConverter::toString(myuid));

	} else if (type==MSG3_USE_VEHICLE)
	{
		//LogManager::getSingleton().logMessage("I got a vehicle");
		//we want first to check if the vehicle name is valid before committing to anything

		//possible load from cache system
		String truckname = buffer;
		String nickname = buffer+strlen(buffer)+1;
		bool authed=false;
		if(strnlen(buffer+strlen(buffer)+1+nickname.size()+1,5) == 6 && !strncmp(buffer+strlen(buffer)+1+nickname.size()+1,"ATUHED",6))
			authed=true;
		bool resourceExists = CACHE.checkResourceLoaded(truckname);
		// check if found
		if(!resourceExists)
		{
			resourceExists=false;
			LogManager::getSingleton().logMessage("Network warning: truck named '"+truckname+"' not found in local installation");
		}
		//spawn vehicle query
		pthread_mutex_lock(&clients_mutex);
		//first check if its not already known
		bool known=false;
		for (int i=0; i<MAX_PEERS; i++)
		{
			if (clients[i].used && clients[i].uid==source) known=true;
		}
		if (!known)
		{
			for (int i=0; i<MAX_PEERS; i++)
			{
				if (!clients[i].used)
				{
					//LogManager::getSingleton().logMessage("Registering as client "+StringConverter::toString(i));
					clients[i].used=true;
					clients[i].invisible=!resourceExists;
					clients[i].loaded=false;
					clients[i].uid=source;
					clients[i].authed=authed;
					if(strnlen(clients[i].nickname, 5) == 0)
						strcpy(clients[i].nickname, "unkown");
					buffer[wrotelen]=0;

					strcpy(clients[i].vehicle, truckname.c_str());
					// fix for MP, important to support the same vehicle with different UID's
					String vehicle_without_uid = CACHE.stripUIDfromString(truckname);
					strcpy(clients[i].vehicle, vehicle_without_uid.c_str());

					strcpy(clients[i].nickname, nickname.c_str()); //buffer+strlen(buffer)+1); //magical!  // rather hackish if you ask me ...

					// update playerlist

					if(i < MAX_PLAYLIST_ENTRIES)
					{
						try
						{
							String plstr = StringConverter::toString(i) + ": " + ColoredTextAreaOverlayElement::StripColors(String(clients[i].nickname));
							if(!resourceExists)
								plstr += " (i)";
							mefl->playerlistOverlay[i]->setCaption(plstr);
						} catch(...)
						{
						}
					}

					// add some chat msg
					pthread_mutex_lock(&chat_mutex);

					// if we know the truck, use its name rather then the full UID thing
					String truckname = clients[i].vehicle;
					if(resourceExists)
					{
						Cache_Entry entry = CACHE.getResourceInfo(truckname);
						truckname = entry.dname;
					}
					CONSOLE.addText("^9* " + ColoredTextAreaOverlayElement::StripColors(clients[i].nickname) + " joined with " + truckname);
					if(!resourceExists)
						CONSOLE.addText("^1* " + String(clients[i].vehicle) + " not found. Player will be invisible.");
					pthread_mutex_unlock(&chat_mutex);


					break;
				}
			}
		}
		pthread_mutex_unlock(&clients_mutex);

		// also, send out our ID, so people know us :D
		sendmessage(peer, serverAddress, MSG3_USE_VEHICLE, myuid, strlen(ourTruckname), ourTruckname);

	}
	else if (type==MSG3_DELETE)
	{
		pthread_mutex_lock(&clients_mutex);
		for (int i=0; i<MAX_PEERS; i++)
		{
			if (clients[i].used && clients[i].uid==source && (clients[i].loaded || clients[i].invisible))
			{
				// pass event to the truck itself
				if(!clients[i].invisible)
					trucks[clients[i].trucknum]->deleteNetTruck();

				//delete network info
				clients[i].used=false;
				clients[i].invisible=false;


				// update all framelistener stuff
				if(!clients[i].invisible)
					mefl->netDisconnectTruck(clients[i].trucknum);
				// update playerlist
				if(i < MAX_PLAYLIST_ENTRIES)
				{
					mefl->playerlistOverlay[i]->setCaption("");
				}

				// add some chat msg
				pthread_mutex_lock(&chat_mutex);
				CONSOLE.addText("^9* " + ColoredTextAreaOverlayElement::StripColors(clients[i].nickname) + " disconnected");
				pthread_mutex_unlock(&chat_mutex);

				break;
			}
		}
		pthread_mutex_unlock(&clients_mutex);
	}
	else if (type==MSG3_CHAT)
	{
		if(source>=0)
		{
			for (int i=0; i<MAX_PEERS; i++)
			{
				if (clients[i].uid==source)
				{
					buffer[wrotelen]=0;
					pthread_mutex_lock(&chat_mutex);
					String nickname = ColoredTextAreaOverlayElement::StripColors(String(clients[i].nickname));
					if(clients[i].invisible)
						nickname += " (i)";
					CONSOLE.addText("^8" + nickname + ": ^7" + ColoredTextAreaOverlayElement::StripColors(String(buffer)));
					pthread_mutex_unlock(&chat_mutex);
					break;
				}
			}
		} else if(source == -1)
		{
			// server msg
			pthread_mutex_lock(&chat_mutex);
			CONSOLE.addText(String(buffer));
			pthread_mutex_unlock(&chat_mutex);
		}
	}
	else if (type==MSG3_GAME_CMD)
	{
	}
}


void NetworkNew::disconnect()
{
	shutdown=true;
	sendmessage(peer, serverAddress, MSG2_DELETE, myuid, 0, 0);
	LogManager::getSingleton().logMessage("NetworkNew::disconnect()");
}

int NetworkNew::rconlogin(char* rconpasswd)
{
	LogManager::getSingleton().logMessage("NetworkNew::rconlogin()");
	return 0;
}

int NetworkNew::rconcommand(char* rconcmd)
{
	LogManager::getSingleton().logMessage("NetworkNew::rconcommand()");
	return 0;
}

int NetworkNew::getConnectedClientCount()
{
	LogManager::getSingleton().logMessage("NetworkNew::getConnectedClientCount()");
	return 0;
}

char *NetworkNew::getTerrainName()
{
	LogManager::getSingleton().logMessage("NetworkNew::getTerrainName()");
    //peer->RPC("getTerrainName", 0, 0, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, false, 0, UNASSIGNED_NETWORK_ID, 0);
	return "aspen-test";
}

char *NetworkNew::getNickname()
{
	LogManager::getSingleton().logMessage("NetworkNew::getNickname()");
	return "notimplemented";
}

int NetworkNew::getRConState()
{
	LogManager::getSingleton().logMessage("NetworkNew::getRConState()");
	return 0;
}


#endif //NEWNET

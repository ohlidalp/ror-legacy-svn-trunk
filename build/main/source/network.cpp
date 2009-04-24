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
#include "network.h"
#include "ExampleFrameListener.h"
#include "ColoredTextAreaOverlayElement.h"
#include "IngameConsole.h"
#include "CacheSystem.h"
#include "turboprop.h"
#include "sha1.h"
#include "Settings.h"

using namespace RoR; //CSHA1

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//#include <CFUserNotification.h>
#endif

Network *net_instance;

void *s_sendthreadstart(void* vid)
{
	net_instance->sendthreadstart();
	return NULL;
}

void *s_receivethreadstart(void* vid)
{
	net_instance->receivethreadstart();
	return NULL;
}


int Network::downloadMod(char* modname)
{
	return 0;
}

Network::Network(Beam **btrucks, std::string servername, long sport, ExampleFrameListener *efl) : NetworkBase(btrucks, servername, sport, efl), lagDataClients()
{
	shutdown=false;
	ssm=SoundScriptManager::getSingleton();
	mySname = servername;
	mySport = sport;
	strcpy(terrainName, "");
	mefl = efl;
	myauthlevel = AUTH_NONE;
	net_instance=this;
	nickname = "";
	trucks=btrucks;
	myuid=0;
	
	speed_time=0;
	speed_bytes_sent = speed_bytes_sent_tmp = speed_bytes_recv = speed_bytes_sent_tmp = 0;

	rconauthed=0;
	last_time=0;
	send_buffer=0;
	pthread_mutex_init(&msgsend_mutex, NULL);
	pthread_mutex_init(&send_work_mutex, NULL);
	pthread_cond_init(&send_work_cv, NULL);
	for (int i=0; i<MAX_PEERS; i++) clients[i].used=false;
	pthread_mutex_init(&clients_mutex, NULL);
	pthread_mutex_init(&chat_mutex, NULL);
	//delayed start, we do nothing until we know the vehicle name
}

void Network::netFatalError(String errormsg, bool exitProgram)
{
	if(shutdown)
		return;

	SWBaseSocket::SWBaseError error;
	socket.set_timeout(1, 1000);
	socket.disconnect(&error);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	String err = "Network fatal error: "+errormsg;
	MessageBox( NULL, err.c_str(), "Network Connection Problem", MB_OK | MB_ICONERROR | MB_TOPMOST);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	//CFOptionFlags flgs;
	//CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, "A network error occured", errormsg.c_str(), NULL, NULL, NULL, &flgs);
#endif

	LogManager::getSingleton().logMessage("NET FATAL ERROR: " + errormsg);
	if(exitProgram)
		exit(124);
}

bool Network::connect()
{
	//here we go
	//begin setup with the caller thread
	SWBaseSocket::SWBaseError error;

	//manage the server socket
	socket.set_timeout(10, 10000); // 10 seconds timeout set as default
	socket.connect(mySport, mySname, &error);
	if (error!=SWBaseSocket::ok)
	{
		//this is an error!
		netFatalError("Establishing network session: ", false);
		return false;
	}
	//say hello to the server
	if (sendmessage(&socket, MSG2_HELLO, (unsigned int)strlen(RORNET_VERSION), RORNET_VERSION))
	{
		//this is an error!
		netFatalError("Establishing network session: error sending hello", false);
		return false;
	}
	int type;
	int source;
	unsigned int wrotelen;
	char buffer[256];
	//get server version
	if (receivemessage(&socket, &type, &source, &wrotelen, buffer, 255))
	{
		//this is an error!
		netFatalError("Establishing network session: error getting server version", false);
		return false;
	}
	//check server version
	if (strncmp(buffer, RORNET_VERSION, strlen(RORNET_VERSION)))
	{
		netFatalError("Establishing network session: wrong server version");
		return false;
	}
	// receive terrain name
	if (receivemessage(&socket, &type, &source, &wrotelen, buffer, 255))
	{
		netFatalError("Establishing network session: error getting server terrain");
		return false;
	}
	if (type!=MSG2_TERRAIN_RESP)
	{
		netFatalError("Establishing network session: error getting server terrain response");
		return false;
	}
	strncpy(terrainName, buffer, wrotelen);
	terrainName[wrotelen]=0;

	// first handshake done, increase the timeout, important!
	socket.set_timeout(0, 0);

	//send credencials
	nickname = SETTINGS.getSetting("Nickname");
	String nick = nickname;
	StringUtil::toLowerCase(nick);
	if (nick==String("pricorde") || nick==String("thomas"))
		nickname = "Anonymous";

	char pwbuffer[250];
	memset(pwbuffer, 0, 250);
	strncpy(pwbuffer, SETTINGS.getSetting("Server password").c_str(), 250);

	char sha1pwresult[250];
	memset(sha1pwresult, 0, 250);
	if(strnlen(pwbuffer, 250)>0)
	{
		CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)pwbuffer, strnlen(pwbuffer, 250));
		sha1.Final();
		sha1.ReportHash(sha1pwresult, CSHA1::REPORT_HEX_SHORT);
	}

	String usertoken = SETTINGS.getSetting("User Token");

	// construct user credentials
	user_credentials_t c;
	memset(&c, 0, sizeof(user_credentials_t));
	strncpy(c.username, nickname.c_str(), 20);
	strncpy(c.password, sha1pwresult, 40);
	strncpy(c.uniqueid, usertoken.c_str(), 40);

	if (sendmessage(&socket, MSG2_USER_CREDENTIALS, sizeof(user_credentials_t), (char*)&c))
	{
		//this is an error!
		netFatalError("Establishing network session: error sending hello", false);
		return false;
	}
	//now this is important, getting authorization
	if (receivemessage(&socket, &type, &source, &wrotelen, buffer, 255))
	{
		//this is an error!
		netFatalError("Establishing network session: error getting server authorization", false);
		return false;
	}
	if (type==MSG2_FULL)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, server has too many players", false);
		return false;
	}
	if (type==MSG2_BANNED)
	{
		char tmp[512];
		memset(tmp, 0, 512);
		if(buffer && strnlen(buffer, 20)>0)
		{
			buffer[wrotelen]=0;
			sprintf(tmp, "Establishing network session: sorry, you are banned:\n%s", buffer);
			netFatalError(tmp);
		} else
		{
			netFatalError("Establishing network session: sorry, you are banned!", false);
		}

		return false;
	}
	if (type==MSG2_WRONG_PW)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, wrong password!", false);
		return false;
	}
	if (type!=MSG2_WELCOME)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, unknown server response", false);
		return false;
	}
	//okay keep our uid
	myuid=source;

	return true;
}

int Network::rconlogin(char* rconpasswd)
{
	int type=0;
	unsigned int source=0;
	unsigned int wrotelen=0;

	char buffer[250];
	memset(buffer, 0, 250);
	strncpy(buffer, rconpasswd, 40);
	buffer[40]=0;

	char result[250];
	memset(result, 0, 250);
	if(strnlen(rconpasswd, 40)>0)
	{
		CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)buffer, strnlen(buffer, 250));
		sha1.Final();
		sha1.ReportHash(result, CSHA1::REPORT_HEX_SHORT);
	} else
		return -2;

	//LogManager::getSingleton().logMessage("SHA1('"+String(rconpasswd)+"') = '"+String(result)+"'");
	//lets send the request
	if (sendmessage(&socket, MSG2_RCON_LOGIN, (unsigned int)strnlen(result, 40), result))
	{
		//this is an error!
		netFatalError("Establishing network session: error sending rcon password");
		return -3;
	}
	return 0;
}

int Network::rconcommand(char* rconcmd)
{
	if(!rconauthed)
		return -1;
	if (sendmessage(&socket, MSG2_RCON_COMMAND, (unsigned int)strnlen(rconcmd, 250), rconcmd))
	{
		netFatalError("Establishing network session: error sending rcon command");
		return -2;
	}
	return 0;
}

void Network::sendVehicleType(char* name, int buffersize)
{
	//lets send our vehicle name
	if (sendmessage(&socket, MSG2_USE_VEHICLE, (unsigned int)strlen(name), name))
	{
		//this is an error!
		netFatalError("Establishing network session: error sending vehicle name");
		return;
	}
	//send the buffer size
	send_buffer_len=buffersize;
	unsigned int bsize=sizeof(oob_t)+send_buffer_len;
	send_buffer=(char*)malloc(send_buffer_len);
	if (sendmessage(&socket, MSG2_BUFFER_SIZE, 4, (char*)(&bsize)))
	{
		//this is an error!
		netFatalError("Establishing network session: error sending vehicle data size");
		return;
	}
	//start the handling threads
	pthread_create(&sendthread, NULL, s_sendthreadstart, (void*)(0));
	pthread_create(&receivethread, NULL, s_receivethreadstart, (void*)(0));
}

Ogre::String Network::getNickname(bool colour)
{
	// returns coloured nickname
	int nickColour = 8;
	if(myauthlevel & AUTH_NONE)   nickColour = 8; // grey
	if(myauthlevel & AUTH_BOT )   nickColour = 4; // blue
	if(myauthlevel & AUTH_RANKED) nickColour = 2; // green
	if(myauthlevel & AUTH_MOD)    nickColour = 1; // red
	if(myauthlevel & AUTH_ADMIN)  nickColour = 1; // red

	String nick = ColoredTextAreaOverlayElement::StripColors(nickname);
	if(colour)
		return String("^") + StringConverter::toString(nickColour) + nick + String("^7");
	
	return nick;
}

int Network::sendmessage(SWInetSocket *socket, int type, unsigned int len, char* content)
{
	pthread_mutex_lock(&msgsend_mutex); //we use a mutex because a chat message can be sent asynchronously
	SWBaseSocket::SWBaseError error;
	header_t head;
	memset(&head, 0, sizeof(header_t));
	head.command=type;
	head.source=myuid;
	head.size=len;
	int hlen=0;

	// construct buffer
	const int msgsize = sizeof(header_t) + len;
	char buffer[MAX_MESSAGE_LENGTH];
	memset(buffer, 0, MAX_MESSAGE_LENGTH);
	memcpy(buffer, (char *)&head, sizeof(header_t));
	memcpy(buffer+sizeof(header_t), content, len);

	int rlen=0;
	speed_bytes_sent_tmp += msgsize;
	while (rlen<(int)msgsize)
	{
		int sendnum=socket->send(buffer+rlen, msgsize-rlen, &error);
		if (sendnum<0)
		{
			LogManager::getSingleton().logMessage("NET send error: " + StringConverter::toString(sendnum));
			return -1;
		}
		rlen+=sendnum;
	}
	pthread_mutex_unlock(&msgsend_mutex);
	calcSpeed();
	return 0;
}

int Network::receivemessage(SWInetSocket *socket, int *type, int *source, unsigned int *wrotelen, char* content, unsigned int bufferlen)
{
	SWBaseSocket::SWBaseError error;

	char buffer[MAX_MESSAGE_LENGTH];
	//ensure that the buffer is clean after each received message!
	memset(buffer, 0, MAX_MESSAGE_LENGTH);

	int hlen=0;
	while (hlen<(int)sizeof(header_t))
	{
		int recvnum=socket->recv(buffer+hlen, sizeof(header_t)-hlen,&error);
		if (recvnum<0)
		{
			LogManager::getSingleton().logMessage("NET receive error 1: " + StringConverter::toString(recvnum));
			return -1;
		}
		hlen+=recvnum;
	}
	header_t head;
	memcpy(&head, buffer, sizeof(header_t));
	*type=head.command;
	*source=head.source;
	*wrotelen=head.size;
	if(head.size>0)
	{
		//read the rest
		while (hlen<(int)sizeof(header_t)+(int)head.size)
		{
			int recvnum=socket->recv(buffer+hlen, (head.size+sizeof(header_t))-hlen,&error);
			if (recvnum<0)
			{
				LogManager::getSingleton().logMessage("NET receive error 2: "+ StringConverter::toString(recvnum));
				return -1;
			}
			hlen+=recvnum;
		}
	}
	speed_bytes_recv_tmp += head.size + sizeof(header_t);

	memcpy(content, buffer+sizeof(header_t), bufferlen);
	calcSpeed();
	return 0;
}

//this is called at each frame to check if a new vehicle must be spawn
bool Network::vehicle_to_spawn(char* name, unsigned int *uid, unsigned int *label)
{
	pthread_mutex_lock(&clients_mutex);
	for (int i=0; i<MAX_PEERS; i++)
	{
		if (clients[i].used && !clients[i].loaded && !clients[i].invisible)
		{
			strcpy(name, clients[i].truck_name);
			*uid=clients[i].user_id;
			*label=i;
			pthread_mutex_unlock(&clients_mutex);
			return true;
		}
	}
	pthread_mutex_unlock(&clients_mutex);
	return false;
}

//this is called to confirm a truck is loaded - must be called within the same frame as vehicle_to_spawn
int Network::vehicle_spawned(unsigned int uid, int trucknum, client_t &return_client)
{
	pthread_mutex_lock(&clients_mutex);
	for (int i=0; i<MAX_PEERS; i++)
	{
		if (clients[i].user_id==uid)
		{
			clients[i].loaded=true;
			clients[i].trucknum=trucknum;
			//ret=clients[i].nickname;
			return_client = clients[i];
			pthread_mutex_unlock(&clients_mutex);
			return 0;
		}
	}
	pthread_mutex_unlock(&clients_mutex);
	return 1;
}

int Network::getSpeedUp()
{
	return speed_bytes_sent;
}

int Network::getSpeedDown()
{
	return speed_bytes_recv;
}

void Network::calcSpeed()
{
	int t = timer.getMilliseconds();
	if(t - speed_time > 1000)
	{
		// we measure bytes / second
		speed_bytes_sent = speed_bytes_sent_tmp;
		speed_bytes_sent_tmp = 0;
		speed_bytes_recv = speed_bytes_recv_tmp;
		speed_bytes_recv_tmp = 0;
		speed_time = t;
	}
}


//external call to trigger data sending
//we are called once a frame!
void Network::sendData(Beam* truck)
{
	int t=timer.getMilliseconds();
	if (t-last_time>100)
	{
		memset(send_buffer,0,send_buffer_len);
		last_time=t;
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
		if (ssm->getTrigState(truck->trucknum, SS_TRIG_HORN))
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

		netlock=truck->netlock;
//LogManager::getSingleton().logMessage("Sending data");
		//unleash the gang
		pthread_mutex_lock(&send_work_mutex);
		pthread_cond_broadcast(&send_work_cv);
		pthread_mutex_unlock(&send_work_mutex);
	}
}


void Network::sendthreadstart()
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
			int etype=sendmessage(&socket, MSG2_VEHICLE_DATA, blen, sendthreadstart_buffer);
			if (etype)
			{
				char emsg[256];
				sprintf(emsg, "Error %i while sending data packet", etype);
				netFatalError(emsg);
				return;
			}
			if (netlock.state==LOCKED)
			{
				netforce_t msg;
				int uid=-1;
				//find uid
				if (netlock.remote_truck==0)
				{
					uid=myuid;
				}
				else
				{
					for (int i=0; i<MAX_PEERS; i++)
					{
						if (clients[i].used && !clients[i].invisible && clients[i].trucknum==netlock.remote_truck)
						{
							uid=clients[i].user_id;
							break;
						}
					}
				}
				if (uid!=-1)
				{
					msg.target_uid=uid;
					msg.node_id=netlock.remote_node;
					msg.fx=netlock.toSendForce.x;
					msg.fy=netlock.toSendForce.y;
					msg.fz=netlock.toSendForce.z;

					int etype=sendmessage(&socket, MSG2_FORCE, sizeof(netforce_t), (char*)&msg);
					if (etype)
					{
						char emsg[256];
						sprintf(emsg, "Error %i while sending netlock packet", etype);
						netFatalError(emsg);
						return;
					}
				}
			}
		}
	}
}

void Network::disconnect()
{
	shutdown=true;
	sendmessage(&socket, MSG2_DELETE, 0, 0);
	SWBaseSocket::SWBaseError error;
	socket.set_timeout(1, 1000);
	socket.disconnect(&error);
	LogManager::getSingleton().logMessage("Network error while disconnecting: ");
}

std::map<int, float> &Network::getLagData()
{
	return lagDataClients;
}

void Network::receivethreadstart()
{
	int type;
	int source;
	unsigned int wrotelen;
	char *buffer=(char*)malloc(MAX_MESSAGE_LENGTH);
	LogManager::getSingleton().logMessage("Receivethread starting");
	// unlimited timeout, important!
	socket.set_timeout(0,0);
	while (!shutdown)
	{
		//get one message
		int err=receivemessage(&socket, &type, &source, &wrotelen, buffer, MAX_MESSAGE_LENGTH);
		if (err)
		{
			//this is an error!
			char errmsg[256];
			sprintf(errmsg, "Error %i while receiving data", err);
			netFatalError(errmsg);
			return;
		}
		else if (type==MSG2_VEHICLE_DATA)
		{
			//we must update a vehicle
			//find which vehicle it is
			//then call pushNetwork(buffer, wrotelen)
			pthread_mutex_lock(&clients_mutex);
			for (int i=0; i<MAX_PEERS; i++)
			{
				if (clients[i].used && !clients[i].invisible && clients[i].user_id==source && clients[i].loaded)
				{
					//okay
					trucks[clients[i].trucknum]->pushNetwork(buffer, wrotelen);

					// hack-ish: detect LAG:
					oob_t *o = (oob_t *)buffer;
					lagDataClients[source] =  o->time - (trucks[clients[i].trucknum]->getTruckTime() + trucks[clients[i].trucknum]->getNetTruckTimeOffset());
					//LogManager::getSingleton().logMessage("Id like to push to "+StringConverter::toString(clients[i].trucknum));
					break;
				}
			}
			pthread_mutex_unlock(&clients_mutex);
		}
		else if (type==MSG2_USE_VEHICLE || type == MSG2_USE_VEHICLE2)
		{
			//LogManager::getSingleton().logMessage("I got a vehicle");
			//we want first to check if the vehicle name is valid before committing to anything
			bool newformat = (type == MSG2_USE_VEHICLE2);
			String truckname = "";
			String tmpnickname = "";
			int authstate = AUTH_NONE;

			if(!newformat)
			{
				// old format does not support auth info
				truckname = buffer;
				tmpnickname  = buffer+strlen(buffer)+1;
				authstate = AUTH_NONE;
			} else
			{
				// yay, new format :D
				// version = 1 for now, be prepared to recognize more in the future
				client_info_on_join *info = (client_info_on_join *)buffer;
				tmpnickname = String(info->nickname);
				truckname = String(info->vehiclename);
				authstate = info->authstatus;
			}
			if(source == myuid)
			{
				// we found ourself :D
				// update local data
				if(nickname != tmpnickname)
				{
					pthread_mutex_lock(&chat_mutex);
					NETCHAT.addText("^9You are now known as " + tmpnickname);
					pthread_mutex_unlock(&chat_mutex);
				}
				nickname = tmpnickname;

				// tell the user the new infos
				if(myauthlevel != authstate)
				{
					String msg = "";
					if(!(myauthlevel & AUTH_BOT) && (authstate & AUTH_BOT))
						msg = "Bot";
					if(!(myauthlevel & AUTH_RANKED) && (authstate & AUTH_RANKED))
						msg = "Ranked Member";
					if(!(myauthlevel & AUTH_MOD) && (authstate & AUTH_MOD))
						msg = "Server Moderator";
					if(!(myauthlevel & AUTH_ADMIN) && (authstate & AUTH_ADMIN))
						msg = "Server Admin";
					
					if(!msg.empty())
					{
						pthread_mutex_lock(&chat_mutex);
						NETCHAT.addText("^9You are now authorized as " + msg);
						pthread_mutex_unlock(&chat_mutex);
					}
				}
				myauthlevel = authstate;
				// and discard the rest, we dont want to create a clone of us...
				continue;
			}
			
			// WARNING: THIS PRODUCES LAGS!
			// we need to background load this ...
			//possible load from cache system
			bool resourceExists = CACHE.checkResourceLoaded(truckname);
			// check if found
			if(!resourceExists)
			{
				// check for different UID
				String truckname2 = CACHE.stripUIDfromString(truckname);
				resourceExists = CACHE.checkResourceLoaded(truckname2);
				if(!resourceExists)
				{
					LogManager::getSingleton().logMessage("Network warning: truck named '"+truckname+"' not found in local installation");
				}
			}
			//spawn vehicle query
			pthread_mutex_lock(&clients_mutex);
			//first check if its not already known
			bool known=false;
			for (int i=0; i<MAX_PEERS; i++)
			{
				if (clients[i].used && clients[i].user_id==source) known=true;
			}
			if (!known)
			{
				for (int i=0; i<MAX_PEERS; i++)
				{
					if (!clients[i].used)
					{
						//LogManager::getSingleton().logMessage("Registering as client "+StringConverter::toString(i));
						clients[i].used           = true;
						clients[i].invisible      = !resourceExists;
						clients[i].loaded         = false;
						clients[i].user_id        = source;
						clients[i].user_authlevel = authstate;
						buffer[wrotelen] = 0;

						//strcpy(clients[i].vehicle, truckname.c_str());
						// fix for MP, important to support the same vehicle with different UID's
						String vehicle_without_uid = CACHE.stripUIDfromString(truckname);
						strcpy(clients[i].truck_name, vehicle_without_uid.c_str());

						strcpy(clients[i].user_name, tmpnickname.c_str()); //buffer+strlen(buffer)+1); //magical!  // rather hackish if you ask me ...

						// update playerlist
						if(i < MAX_PLAYLIST_ENTRIES)
						{
							try
							{
								String plstr = StringConverter::toString(i) + ": " + ColoredTextAreaOverlayElement::StripColors(String(clients[i].user_name));
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
						String truckname = clients[i].truck_name;
						if(resourceExists)
						{
							Cache_Entry entry = CACHE.getResourceInfo(truckname);
							truckname = entry.dname;
						}

						NETCHAT.addText(getUserChatName(&clients[i]) + " ^9joined with " + truckname);

						if(!resourceExists)
							NETCHAT.addText("^1* " + String(clients[i].truck_name) + " not found. Player will be invisible.");
						pthread_mutex_unlock(&chat_mutex);

						break;
					}
				}
			}
			pthread_mutex_unlock(&clients_mutex);
		}
		else if (type==MSG2_DELETE)
		{
			pthread_mutex_lock(&clients_mutex);
			if(source == myuid)
			{
				// we got deleted D:
				netFatalError(String(buffer));
			}

			// remove key from lag stats
			if(lagDataClients.find(source) != lagDataClients.end()) lagDataClients.erase(source);

			for (int i=0; i<MAX_PEERS; i++)
			{
				if (clients[i].used && clients[i].user_id==source && (clients[i].loaded || clients[i].invisible))
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
					
					NETCHAT.addText(getUserChatName(&clients[i]) + " ^9disconnected: " + String(buffer));
					pthread_mutex_unlock(&chat_mutex);

					break;
				}
			}
			pthread_mutex_unlock(&clients_mutex);
		}
		else if (type==MSG2_CHAT)
		{
			if(source>=0)
			{
				for (int i=0; i<MAX_PEERS; i++)
				{
					if (clients[i].user_id==source)
					{
						buffer[wrotelen]=0;
						pthread_mutex_lock(&chat_mutex);
						
						NETCHAT.addText(getUserChatName(&clients[i]) + ": ^7" + ColoredTextAreaOverlayElement::StripColors(String(buffer)));
						pthread_mutex_unlock(&chat_mutex);
						break;
					}
				}
			} else if(source == -1)
			{
				// server msg
				pthread_mutex_lock(&chat_mutex);
				NETCHAT.addText(String(buffer));
				pthread_mutex_unlock(&chat_mutex);
			}
		}
		else if (type==MSG2_GAME_CMD)
		{
			if(source!=-1)
				// only accept game commands from the server and no one else
				continue;
		}
		else if (type==MSG2_FORCE)
		{
			netforce_t* msg=(netforce_t*)buffer;
			if (msg->target_uid==myuid)
			{
				trucks[0]->pushNetForce(msg->node_id, Vector3(msg->fx,msg->fy,msg->fz));
			}
		}
	}
}

Ogre::String Network::getUserChatName(client_t *c)
{
	// returns coloured nickname
	int nickColour = 8;
	if(c->user_authlevel & AUTH_NONE)   nickColour = 8; // grey
	if(c->user_authlevel & AUTH_BOT )   nickColour = 4; // blue
	if(c->user_authlevel & AUTH_RANKED) nickColour = 2; // green
	if(c->user_authlevel & AUTH_MOD)    nickColour = 1; // red
	if(c->user_authlevel & AUTH_ADMIN)  nickColour = 1; // red

	String nickname = ColoredTextAreaOverlayElement::StripColors(c->user_name);
	if(c->invisible)
		nickname += "^7 (i)";
	return String("^") + StringConverter::toString(nickColour) + nickname + String("^7");
}

void Network::sendChat(char *line)
{
	int etype=sendmessage(&socket, MSG2_CHAT, (int)strlen(line), line);
	if (etype)
	{
		char emsg[256];
		sprintf(emsg, "Error %i while sending chat packet", etype);
		netFatalError(emsg);
		return;
	}
}

int Network::getConnectedClientCount()
{
	int count=0;
	for (int i=0; i<MAX_PEERS; i++)
		if (clients[i].used)
			count++;
	return count;
}

Network::~Network()
{
	shutdown=true;
	pthread_mutex_destroy(&chat_mutex);
	pthread_mutex_destroy(&send_work_mutex);
	pthread_cond_destroy(&send_work_cv);
}




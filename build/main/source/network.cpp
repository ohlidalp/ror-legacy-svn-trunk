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
#ifdef ANGELSCRIPT
#include "ScriptEngine.h"
#endif //ANGELSCRIPT
#include "turboprop.h"
#include "sha1.h"
#include "Settings.h"
#include "utils.h"

#ifdef WSYNC
#include "wsync.h"
#endif //WSYNC

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

void *s_downloadthreadstart(void* vid)
{
	net_instance->downloadthreadstart((char*)vid);
	return NULL;
}

Timer Network::timer = Ogre::Timer();

Network::Network(Beam **btrucks, std::string servername, long sport, ExampleFrameListener *efl): lagDataClients()
{
	NetworkStreamManager::getSingleton().net = this;
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
	pthread_mutex_init(&dl_data_mutex, NULL);
	pthread_cond_init(&send_work_cv, NULL);
	for (int i=0; i<MAX_PEERS; i++) clients[i].used=false;
	pthread_mutex_init(&clients_mutex, NULL);
	pthread_mutex_init(&chat_mutex, NULL);
	//delayed start, we do nothing until we know the vehicle name
}


void Network::downloadthreadstart(char *modname_c)
{
	String modname = String(modname_c);
	// appearently we cannot free here D:
	//free(modname_c);
	LogManager::getSingleton().logMessage("new downloadthread for mod " + modname);
	// we try to call the updater here that will download the file for us hopefully
	String packsPath = SETTINGS.getSetting("User Path")+"packs" + SETTINGS.getSetting("dirsep") + "downloaded";
#ifdef WSYNC
	String modfilename = "";
	WSync *w = new WSync();
	int res = w->downloadMod(modname, modfilename, packsPath, true);
	// write the data back to the main handling stuff and close the thread
	if(modfilename == "")
		modfilename = "error";

	if(!shutdown)
	{
		// this prevents a crash on shutdown
		pthread_mutex_lock(&dl_data_mutex);
		downloadingMods[modname] = modfilename;
		pthread_mutex_unlock(&dl_data_mutex);
	}

#else //WSYNC
	// well, not available for you i guess :(
#endif //WSYNC
}

void Network::tryDownloadMod(Ogre::String modname)
{
	pthread_mutex_lock(&dl_data_mutex);
	// do not double-download mods
	if(downloadingMods.find(modname) != downloadingMods.end())
		return;

	downloadingMods[modname] = "";
	pthread_mutex_unlock(&dl_data_mutex);
	
	
	pthread_mutex_lock(&chat_mutex);
	NETCHAT.addText("^9Trying to download missing mod now: " + modname);
	pthread_mutex_unlock(&chat_mutex);

	// attention: we need permanent space, so we malloc it...
	char *modname_c = (char*)malloc(modname.size());
	strcpy(modname_c, modname.c_str());
	pthread_create(&downloadthread, NULL, s_downloadthreadstart, (void*)modname_c);
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
	if (sendmessage(&socket, MSG2_HELLO, 0, (unsigned int)strlen(RORNET_VERSION), RORNET_VERSION))
	{
		//this is an error!
		netFatalError("Establishing network session: error sending hello", false);
		return false;
	}

	header_t header;
	char buffer[256];
	//get server version
	if (receivemessage(&socket, &header, buffer, 255))
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
	if (receivemessage(&socket, &header, buffer, 255))
	{
		netFatalError("Establishing network session: error getting server terrain");
		return false;
	}
	if (header.command != MSG2_TERRAIN_RESP)
	{
		netFatalError("Establishing network session: error getting server terrain response");
		return false;
	}
	strncpy(terrainName, buffer, header.size);
	terrainName[header.size]=0;

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

	if (sendmessage(&socket, MSG2_USER_CREDENTIALS, 0, sizeof(user_credentials_t), (char*)&c))
	{
		//this is an error!
		netFatalError("Establishing network session: error sending hello", false);
		return false;
	}
	//now this is important, getting authorization
	if (receivemessage(&socket, &header, buffer, 255))
	{
		//this is an error!
		netFatalError("Establishing network session: error getting server authorization", false);
		return false;
	}
	if (header.command==MSG2_FULL)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, server has too many players", false);
		return false;
	}
	if (header.command==MSG2_BANNED)
	{
		char tmp[512];
		memset(tmp, 0, 512);
		if(buffer && strnlen(buffer, 20)>0)
		{
			buffer[header.size]=0;
			sprintf(tmp, "Establishing network session: sorry, you are banned:\n%s", buffer);
			netFatalError(tmp);
		} else
		{
			netFatalError("Establishing network session: sorry, you are banned!", false);
		}

		return false;
	}
	if (header.command==MSG2_WRONG_PW)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, wrong password!", false);
		return false;
	}
	if (header.command!=MSG2_WELCOME)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, unknown server response", false);
		return false;
	}
	//okay keep our uid
	myuid = header.source;

	//start the handling threads
	pthread_create(&sendthread, NULL, s_sendthreadstart, (void*)(0));
	pthread_create(&receivethread, NULL, s_receivethreadstart, (void*)(0));

	return true;
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

int Network::sendMessageRaw(SWInetSocket *socket, char *buffer, unsigned int msgsize)
{
	//LogManager::getSingleton().logMessage("* sending raw message: " + StringConverter::toString(msgsize));

	pthread_mutex_lock(&msgsend_mutex); //we use a mutex because a chat message can be sent asynchronously
	SWBaseSocket::SWBaseError error;
	
	int rlen=0;
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
	return 0;
}

int Network::sendmessage(SWInetSocket *socket, int type, unsigned int streamid, unsigned int len, char* content)
{
	pthread_mutex_lock(&msgsend_mutex); //we use a mutex because a chat message can be sent asynchronously
	SWBaseSocket::SWBaseError error;
	header_t head;
	memset(&head, 0, sizeof(header_t));
	head.command=type;
	head.source=myuid;
	head.size=len;
	head.streamid=streamid;
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

int Network::receivemessage(SWInetSocket *socket, header_t *head, char* content, unsigned int bufferlen)
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
	
	memcpy(head, buffer, sizeof(header_t));

	if(head->size>0)
	{
		//read the rest
		while (hlen<(int)sizeof(header_t)+(int)head->size)
		{
			int recvnum=socket->recv(buffer+hlen, (head->size+sizeof(header_t))-hlen,&error);
			if (recvnum<0)
			{
				LogManager::getSingleton().logMessage("NET receive error 2: "+ StringConverter::toString(recvnum));
				return -1;
			}
			hlen+=recvnum;
		}
	}
	speed_bytes_recv_tmp += head->size + sizeof(header_t);

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
		String truckname = String(clients[i].truck_name);
		// this tends to lock forever D:
		//pthread_mutex_lock(&dl_data_mutex);
		String zipname = downloadingMods[truckname];
		//pthread_mutex_unlock(&dl_data_mutex);
		
		if(clients[i].used && !clients[i].loaded && clients[i].invisible && !zipname.empty())
		{
			// we found a possible late-load candidate :D
			if(zipname == "error2")
			{
				// that thing is known to have failed, ignore it
				continue;
			}
			if(zipname == "error" || zipname == "notfound")
			{
				pthread_mutex_lock(&chat_mutex);
				if(zipname == "error")
					NETCHAT.addText("^9unkown error downloading mod: " + zipname + " (player "+String(clients[i].user_name) + ")");
				if(zipname == "notfound")
					NETCHAT.addText("^9mod not found on repository, stays invisible: " + zipname+ " (player "+String(clients[i].user_name) + ")");

				// set it as something else, so we wont check again
				downloadingMods[truckname] = "error2";
				pthread_mutex_unlock(&chat_mutex);
				// we leave it in there, so it wont be tried to download again
				continue;
			}
			
			String targetpath = SETTINGS.getSetting("User Path")+"packs" + SETTINGS.getSetting("dirsep") + "downloaded";
			String targetfilename = zipname;
			String targetfile = targetpath + SETTINGS.getSetting("dirsep") + targetfilename;
			if(CACHE.fileExists(targetfile))
			{
				// yay we downloaded the mod :D
				// now add it to the cache manually and then load it :)
				ResourceGroupManager::getSingleton().addResourceLocation(targetpath, "FileSystem");
				// finally try to load the mod
				CACHE.loadSingleZip(targetfile, -1);
				if(!CACHE.checkResourceLoaded(truckname))
					LogManager::getSingleton().logMessage("checkResourceLoaded failed for " + truckname);

				pthread_mutex_lock(&chat_mutex);
				NETCHAT.addText("^9downloaded mod successfully, spawning: " + targetfilename+ " (player "+String(clients[i].user_name) + ")");
				pthread_mutex_unlock(&chat_mutex);

				updatePlayerList();
				
				// yay, spawn
				strcpy(name, truckname.c_str());
				*uid=clients[i].user_id;
				clients[i].invisible = false;
				*label=i;
				pthread_mutex_unlock(&clients_mutex);
				return true;

			}

		}

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
		last_time=t;
		//copy data in send_buffer, send_buffer_len
		if (send_buffer==0)
		{
			//boy, thats soo bad
			return;
		}
		memset(send_buffer,0,send_buffer_len);
		int i;
		Vector3 refpos = truck->nodes[0].AbsPosition;
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
		// wait for data...
		NetworkStreamManager::getSingleton().sendStreams(this, &socket);

	}
}

void Network::disconnect()
{
	shutdown=true;
	sendmessage(&socket, MSG2_DELETE, 0, 0, 0);
	SWBaseSocket::SWBaseError error;
	socket.set_timeout(1, 1000);
	socket.disconnect(&error);
	LogManager::getSingleton().logMessage("Network error while disconnecting: ");
}

std::map<int, float> &Network::getLagData()
{
	return lagDataClients;
}

void Network::updatePlayerList()
{
	pthread_mutex_lock(&chat_mutex);
	for (int i=0; i<MAX_PEERS; i++)
	{
		if (!clients[i].used)
			continue;
		if(i < MAX_PLAYLIST_ENTRIES)
		{
			try
			{
				String plstr = StringConverter::toString(i) + ": " + ColoredTextAreaOverlayElement::StripColors(String(clients[i].user_name));
				if(clients[i].invisible)
					plstr += " (i)";
				mefl->playerlistOverlay[i]->setCaption(plstr);
			} catch(...)
			{
			}
		}
	}
	pthread_mutex_unlock(&chat_mutex);
}

unsigned long Network::getNetTime()
{
	return timer.getMilliseconds();
}

void Network::receivethreadstart()
{
	header_t header;

	char *buffer=(char*)malloc(MAX_MESSAGE_LENGTH);
	bool autoDl = (SETTINGS.getSetting("AutoDownload") == "Yes");
	LogManager::getSingleton().logMessage("Receivethread starting");
	// unlimited timeout, important!
	socket.set_timeout(0,0);
	while (!shutdown)
	{
		//get one message
		int err=receivemessage(&socket, &header, buffer, MAX_MESSAGE_LENGTH);
		LogManager::getSingleton().logMessage("received data: " + StringConverter::toString(header.command) + ", source: "+StringConverter::toString(header.source) + ":"+StringConverter::toString(header.streamid) + ", size: "+StringConverter::toString(header.size));
		if (err)
		{
			//this is an error!
			char errmsg[256];
			sprintf(errmsg, "Error %i while receiving data", err);
			netFatalError(errmsg);
			return;
		}

		// TODO: produce new streamable classes when required 
		if(header.command == MSG2_STREAM_REGISTER)
		{
			stream_register_t *reg = (stream_register_t *)buffer;
			LogManager::getSingleton().logMessage(" * received stream registration: " + StringConverter::toString(header.source) + ": "+StringConverter::toString(reg->sid) + ", type: "+StringConverter::toString(reg->type));
			if(reg->type == 0)
			{
				// truck
			} else if (reg->type == 1)
			{
				// person
				this->mefl->newCharacter(header.source, reg->sid);
			}
			continue;
				
		}

		NetworkStreamManager::getSingleton().pushReceivedStreamMessage(header.command, header.source, header.streamid, header.size, buffer);
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
	int etype=sendmessage(&socket, MSG2_CHAT, 1, (int)strlen(line), line);
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
	pthread_mutex_destroy(&dl_data_mutex);
	pthread_cond_destroy(&send_work_cv);
}




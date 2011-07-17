#include "OutProtocol.h"

#include "Settings.h"
#include "BeamFactory.h"
#include "Beam.h"
#include "engine.h"
#include "RoRVersion.h"

#ifdef WIN32
#include <Ws2tcpip.h>
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#else
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif //WIN32

using namespace std;

// from LFS/doc/insim.txt
#define OG_SHIFT		1		// key
#define OG_CTRL			2		// key

#define OG_TURBO		8192	// show turbo gauge
#define OG_KM			16384	// if not set - user prefers MILES
#define OG_BAR			32768	// if not set - user prefers PSI

enum
{
	DL_SHIFT      = BITMASK(1),  // bit 0	- shift light
	DL_FULLBEAM   = BITMASK(2),	 // bit 1	- full beam
	DL_HANDBRAKE  = BITMASK(3),	 // bit 2	- handbrake
	DL_PITSPEED   = BITMASK(4),	 // bit 3	- pit speed limiter
	DL_TC         = BITMASK(5),	 // bit 4	- TC active or switched off
	DL_SIGNAL_L   = BITMASK(6),	 // bit 5	- left turn signal
	DL_SIGNAL_R   = BITMASK(7),	 // bit 6	- right turn signal
	DL_SIGNAL_ANY = BITMASK(8),  // bit 7	- shared turn signal
	DL_OILWARN    = BITMASK(9),  // bit 8	- oil pressure warning
	DL_BATTERY    = BITMASK(10), // bit 9	- battery warning
	DL_ABS        = BITMASK(11), // bit 10	- ABS active or switched off
	DL_SPARE      = BITMASK(12), // bit 11
	DL_NUM        = BITMASK(13)  // bit 14  - end
};

PACK (struct OutGaugePack
{
	unsigned int Time;			// time in milliseconds (to check order)
	char         Car[4];		// Car name
	unsigned short Flags;			// Info (see OG_x below)
	unsigned char Gear;			// Reverse:0, Neutral:1, First:2...
	unsigned char PLID;			// Unique ID of viewed player (0 = none)
	float		 Speed;			// M/S
	float		 RPM;			// RPM
	float		 Turbo;			// BAR
	float		 EngTemp;		// C
	float		 Fuel;			// 0 to 1
	float		 OilPressure;	// BAR
	float		 OilTemp;		// C
	unsigned int DashLights;	// Dash lights available (see DL_x below)
	unsigned int ShowLights;	// Dash lights currently switched on
	float		 Throttle;		// 0 to 1
	float		 Brake;			// 0 to 1
	float		 Clutch;		// 0 to 1
	char         Display1[16];	// Usually Fuel
	char         Display2[16];	// Usually Settings
	int			ID;				// optional - only if OutGauge ID is specified
});

template<> OutProtocol * Ogre::Singleton< OutProtocol >::ms_Singleton = 0;

OutProtocol::OutProtocol(void) : sockfd(-1), delay(0.1), timer(0), mode(0), id(0), working(false), counter(0)
{
	delay = PARSEREAL(SSETTING("OutGauge Delay")) * 0.1f;
	mode  = PARSEINT(SSETTING("OutGauge Mode"));
	id    = PARSEINT(SSETTING("OutGauge ID"));

	if(mode > 0)
		startup();
}

OutProtocol::~OutProtocol(void)
{
	// and close the socket again
	if(sockfd>0)
		closesocket(sockfd);
}

void OutProtocol::startup()
{
#ifdef WIN32
	SWBaseSocket::SWBaseError error;

	string ipstr = SSETTING("OutGauge IP");
	int port = PARSEINT(SSETTING("OutGauge Port"));
	
	struct sockaddr_in sendaddr;
	memset(&sendaddr, 0, sizeof(sendaddr));

	WSADATA        wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		LOG("error starting up winsock");
		return;
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		char tmp[9047];
		sprintf(tmp, "error creating socket for OutGauge: %s", strerror(errno));
		LOG(tmp);
		return;
	}
	sendaddr.sin_family      = AF_INET;
	sendaddr.sin_port        = htons(port);
	inet_pton(AF_INET, ipstr.c_str(), &sendaddr.sin_addr);
	if(connect(sockfd, (struct sockaddr *) &sendaddr, sizeof(sendaddr)) == SOCKET_ERROR)
	{
		char tmp[9047];
		sprintf(tmp, "error connecting socket for OutGauge: %s", strerror(errno));
		LOG(tmp);
		return;
	}
	LOG("OutGauge connected successfully");
	working=true;
#endif // WIN32
}

bool OutProtocol::update(float dt)
{
	if(!working) return false;

	// not in a truck?
	Beam *truck = BeamFactory::getSingleton().getCurrentTruck();
	if(!truck || !truck->engine)
		return true;

	// below the set delay?
	timer+=dt;
	if(timer < delay)
		return true;
	timer = 0;

	// send a package
	OutGaugePack gd;
	memset(&gd, 0, sizeof(gd));
	
	gd.Time = counter;
	sprintf(gd.Car, "RoR");
	gd.Flags = 0;
	gd.Flags |= OG_KM;
	if(truck->engine->hasturbo) gd.Flags |= OG_TURBO;

	gd.Gear    = truck->engine->getGear() + 1;
	if(truck->engine->getGear() + 1 < 0) gd.Gear = 0; // we only support one reverse gear :\

	gd.PLID    = 0;
	gd.Speed   = fabs(truck->WheelSpeed);
	gd.RPM     = truck->engine->getRPM();
	gd.Turbo   = truck->engine->getTurboPSI();
	gd.EngTemp = 0; // TODO
	gd.Fuel    = 0; // TODO
	gd.OilPressure = 0; // TODO
	gd.OilTemp = 0; // TODO

	gd.DashLights = 0;
	gd.DashLights |= DL_HANDBRAKE;
	gd.DashLights |= DL_BATTERY;
	gd.DashLights |= DL_SIGNAL_L;
	gd.DashLights |= DL_SIGNAL_R;
	gd.DashLights |= DL_SIGNAL_ANY;
	if(truck->tc_present)   gd.DashLights |= DL_TC;
	if(truck->alb_present)  gd.DashLights |= DL_ABS;

	gd.ShowLights = 0;
	if(truck->parkingbrake) gd.ShowLights |= DL_HANDBRAKE;
	if(truck->tc_mode)      gd.ShowLights |= DL_TC;
	if(truck->lights)       gd.ShowLights |= DL_FULLBEAM;
	if(truck->engine->contact && !truck->engine->running) gd.ShowLights |=  DL_BATTERY;
	if(truck->left_blink_on)  gd.ShowLights |= DL_SIGNAL_L;
	if(truck->right_blink_on) gd.ShowLights |= DL_SIGNAL_R;
	if(truck->warn_blink_on)  gd.ShowLights |= DL_SIGNAL_ANY;
	if(truck->tc_mode)     gd.DashLights |= DL_TC;
	if(truck->alb_mode)    gd.DashLights |= DL_ABS;
	
	gd.Throttle   = truck->engine->getAcc();
	gd.Brake      = truck->brake / truck->brakeforce;
	gd.Clutch     = truck->engine->getClutch(); // 0-1

	sprintf(gd.Display1, "RoR v %s", ROR_VERSION_STRING);
	strncpy(gd.Display2, truck->realtruckname.c_str(), 15);

	gd.ID = id;

	// send the package
	send(sockfd, (const char*)&gd, sizeof(gd), NULL);

	counter++;

	return true;
}
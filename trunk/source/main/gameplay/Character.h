/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#ifndef __Character_H_
#define __Character_H_

#include "RoRPrerequisites.h"

#include "MovableText.h"
#include "Streamable.h"

enum {CHARCMD_POSITION, CHARCMD_ATTACH};

class Character : public Streamable
{
	friend class CharacterFactory;
	friend class Network;

public:

	Character(Ogre::Camera *cam, Collisions *c, Network *net, HeightFinder *h, Water *w, MapControl *m, Ogre::SceneManager *scm, int source=-1, unsigned int streamid=0, int colourNumber=0, bool remote=true);
	~Character();

	Ogre::Quaternion getOrientation();
	Ogre::SceneNode *getSceneNode() { return personode; };
	Ogre::Vector3 getPosition();
	bool getPhysicsEnabled() { return physicsEnabled; };
	bool getVisible();
	bool isRemote() { return remote; };
	float getAngle() { return persoangle; };
	int getUID() { return source; };
	int setBeamCoupling(bool enabled, Beam *truck=0);
	static unsigned int characterCounter;
	void move(Ogre::Vector3 v);
	void setAngle(float angle);
	void setCollisions(Collisions *c);
	void setColour(int number) { this->colourNumber = number; };
	void setHFinder(HeightFinder *h);
	void setOrientation(Ogre::Quaternion);
	void setPhysicsEnabled(bool val) { physicsEnabled=val; };
	void setPosition(Ogre::Vector3 pos);
	void setVisible(bool v);
	void setWater(Water *w);

	void update(float dt);
	void updateCharacterColour();
	void updateMapIcon();
	void updateNetLabel();

protected:

	Beam *beamCoupling;
	Collisions *collisions;
	HeightFinder *hfinder;
	MapControl *map;
	MapEntity *mapEnt;
	Network *net;
	Water *water;

	bool perso_canjump;
	bool physicsEnabled;
	bool remote;
	
	float persoangle;
	float persospeed;
	float persovspeed;
	
	int colourNumber;
	int networkAuthLevel;
	int source;

	Ogre::AnimationStateSet *persoanim;
	Ogre::Camera *mCamera;
	Ogre::MovableText *netMT;
	Ogre::SceneManager *scm;
	Ogre::SceneNode *personode;
	Ogre::String lastmode;
	Ogre::String myName;
	Ogre::UTFString networkUsername;
	Ogre::Vector3 lastpersopos;
	Ogre::Vector3 position;

	unsigned int myNumber;
	unsigned int streamid;
	
	void setAnimationMode(Ogre::String mode, float time=0);

	typedef struct header_netdata_t
	{
		int command;
	} header_netdata_t;

	typedef struct pos_netdata_t
	{
		int command;
		float posx, posy, posz;
		float rotx, roty, rotz, rotw;
		char animationMode[256];
		float animationTime;
	} pos_netdata_t;

	typedef struct attach_netdata_t
	{
		int command;
		bool enabled;
		int source_id;
		int stream_id;
		int position;
	} attach_netdata_t;

	// overloaded from Streamable:
	Ogre::Timer netTimer;
	int last_net_time;

	void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);
	void sendStreamData();
	void sendStreamSetup();
	void setUID(int uid);

	void updateNetLabelSize();
};

#endif // __Character_H_

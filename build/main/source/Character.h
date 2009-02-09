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

#ifndef CHARACTER_H_
#define CHARACTER_H_

#include <Ogre.h>
#include <OgreVector3.h>

class Water;
class Collisions;
class HeightFinder;
class MapControl;

class Character
{
public:
	Character(Collisions *c, HeightFinder *h, Water *w, MapControl *m, Ogre::SceneManager *scm);
	~Character();
	
	void setVisible(bool v);
	bool getVisible();
	

	// these two function are here for chacters in MP :) (placeholders only)
	void pushNetwork();
	void PullNetwork();
	
	Ogre::Quaternion getOrientation();
	Ogre::Vector3 getPosition();
	void setPosition(Ogre::Vector3 pos);
	float getAngle();
	
	void move(Ogre::Vector3 v);
	
	void update(float dt);
	Ogre::SceneNode *getSceneNode();

	void setAngle(float angle) { persoangle = angle; };

	static unsigned int characterCounter;
	void updateMapIcon();

protected:
	Ogre::Vector3 position;
	Collisions *collisions;
	HeightFinder *hfinder;
	Water *water;
	MapControl *map;
	Ogre::SceneManager *scm;
	
	Ogre::SceneNode *personode;
	Ogre::AnimationStateSet *persoanim;
	unsigned int myNumber;
	Ogre::String myName;
	float persoangle;
	float persospeed;
	float persovspeed;
	bool perso_canjump;
	Ogre::Vector3 lastpersopos;
	void setAnimationMode(Ogre::String mode, float time=0);
};

#endif

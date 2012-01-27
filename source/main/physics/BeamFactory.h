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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 24th of August 2009

#ifndef BEAMFACTORY_H__
#define BEAMFACTORY_H__

#include "RoRPrerequisites.h"
#include "StreamableFactory.h"
#include <map>

#include "Beam.h"
#include "TwoDReplay.h"

class BeamFactory : public StreamableFactory < BeamFactory, Beam >
{
	friend class Network;
	friend class RoRFrameListener;
public:
	BeamFactory(SceneManager *manager, SceneNode *parent, RenderWindow* win, Network *net, float *mapsizex, float *mapsizez, Collisions *icollisions, HeightFinder *mfinder, Water *w, Camera *pcam);
	~BeamFactory();

	Beam *createLocal(int slotid);
	Beam *createLocal(Ogre::Vector3 pos, Ogre::Quaternion rot, Ogre::String fname, collision_box_t *spawnbox=NULL, bool ismachine=false, int flareMode=0, std::vector<Ogre::String> *truckconfig=0, Skin *skin=0, bool freePosition=false);
	bool removeBeam(Beam *b);

	Beam *createRemoteInstance(stream_reg_t *reg);

	Beam *getBeam(int source_id, int stream_id); // used by character

	int getTruckCount() { return free_truck; };
	Beam *getTruck(int number) { return trucks[number]; };
	Beam *getCurrentTruck() { return (current_truck<0)?0:trucks[current_truck]; };
	int getCurrentTruckNumber() { return current_truck; };
	void recalcGravityMasses();
	void repairTruck(SoundScriptManager *ssm, Collisions *collisions, char* inst, char* box, bool keepPosition=false);
	void removeTruck(Collisions *collisions, char* inst, char* box);
	void removeTruck(int truck);
	void removeCurrentTruck();
	void setCurrentTruck(int new_truck);
	bool enterRescueTruck();

	void activateAllTrucks();
	void sendAllTrucksSleeping();

	void updateVisual(float dt);
	void updateAI(float dt);

	inline unsigned long getPhysFrame() { return physFrame; };

	void calcPhysics(float dt);

	Beam **getTrucks() { return trucks; };
	int updateSimulation(float dt);

	// beam engine functions
	bool checkForActive(int j, bool *sleepyList);
	void recursiveActivation(int j);
	void checkSleepingState();

	void _waitForSync();

protected:
	Ogre::SceneManager *manager;
	Ogre::SceneNode *parent;
	Ogre::RenderWindow* win;
	Network *net;
	float *mapsizex, *mapsizez;
	Collisions *icollisions;
	HeightFinder *mfinder;
	Water *w;
	Ogre::Camera *pcam;
	int thread_mode;
	
	Beam *trucks[MAX_TRUCKS];
	int free_truck;
	int current_truck;

	TwoDReplay *tdr;

	unsigned long physFrame;

	int done_count;

	int getFreeTruckSlot();

	// functions used by friends
	void netUserAttributesChanged(int source, int streamid);
	void localUserAttributesChanged(int newid);


	bool syncRemoteStreams();
	void updateGUI();
	void removeInstance(Beam *b);
	void removeInstance(stream_del_t *del);
	void _deleteTruck(Beam *b);


	pthread_mutex_t done_count_mutex;
	pthread_cond_t done_count_cv;
	pthread_mutex_t work_mutex;
	pthread_cond_t work_cv;
	pthread_t threads[32];

private:
	int findTruckInsideBox(Collisions *collisions, char* inst, char* box);

};


#endif // BEAMFACTORY_H__
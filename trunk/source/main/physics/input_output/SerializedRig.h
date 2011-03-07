/*
This source file is part of Rigs of Rods
Copyright 2005-2011 Pierre-Michel Ricordel
Copyright 2007-2011 Thomas Fischer

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
// created by thomas fischer thomas{AT}thomasfischer{DOT}biz on the fifth of March 2011

#ifndef SERIALIZEDRIG_H__
#define SERIALIZEDRIG_H__

#include "RoRPrerequisites.h"
#include "BeamData.h" // for rig_t

#include <OgreStringVector.h>

// parser specific helping structs, no need to export those for the whole sources
typedef struct trucksection_t
{
	int sectionID;
	char *name;
	bool titleContainsInfo;
} trucksection_t;

typedef struct parsecontext_t
{
	Ogre::String filename;
	Ogre::String modeString;
	Ogre::String warningText;
	unsigned int linecounter;
	char line[2048];
	int mode;
} parsecontext_t;

class SerializedRig : public rig_t
{
public:
	SerializedRig();
	~SerializedRig();

	int loadTruck(Ogre::String filename, Ogre::SceneManager *manager, Ogre::SceneNode *parent, Ogre::Vector3 pos, Ogre::Quaternion rot, collision_box_t *spawnbox);	

protected:
	std::vector <parsecontext_t> warnings;
	std::vector <parsecontext_t> modehistory;

	void init_node(int pos
		, float x
		, float y
		, float z
		, int type=NODE_NORMAL
		, float m=10.0
		, int iswheel=0
		, float friction=CHASSIS_FRICTION_COEF
		, int id=-1
		, int wheelid=-1
		, float nfriction=NODE_FRICTION_COEF_DEFAULT
		, float nvolume=NODE_VOLUME_COEF_DEFAULT
		, float nsurface=NODE_SURFACE_COEF_DEFAULT
		, float nloadweight=NODE_LOADWEIGHT_DEFAULT);
	
	int add_beam(node_t *p1
		, node_t *p2
		, Ogre::SceneManager *manager
		, Ogre::SceneNode *parent
		, int type
		, float strength
		, float spring
		, float damp
		, int detachgroupstate=DEFAULT_DETACHER_GROUP
		, float length=-1.0
		, float shortbound=-1.0
		, float longbound=-1.0
		, float precomp=1.0
		, float diameter=DEFAULT_BEAM_DIAMETER
		, parsecontext_t *c=0);

	void addWheel(Ogre::SceneManager *manager
		, Ogre::SceneNode *parent
		, float radius
		, float width
		, int rays
		, int node1
		, int node2
		, int snode
		, int braked
		, int propulsed
		, int torquenode
		, float mass
		, float wspring
		, float wdamp
		, char* texf
		, char* texb
		, bool meshwheel=false
		, float rimradius=0.0
		, bool rimreverse=false
		, parsecontext_t *c=0);
	
	void addWheel2(Ogre::SceneManager *manager
		, Ogre::SceneNode *parent
		, float radius
		, float radius2
		, float width
		, int rays
		, int node1
		, int node2
		, int snode
		, int braked
		, int propulsed
		, int torquenode
		, float mass
		, float wspring
		, float wdamp
		, float wspring2
		, float wdamp2
		, char* texf
		, char* texb
		, parsecontext_t *c=0);
		
	void addCamera(int nodepos, int nodedir, int noderoll);
	float warea(Ogre::Vector3 ref, Ogre::Vector3 x, Ogre::Vector3 y, Ogre::Vector3 aref);

	void addSoundSource(SoundScriptInstance *ssi, int nodenum, int type=-2, parsecontext_t *c=0);


	/**
	 * @param line line in configuration file
	 * @return true if line was successfully parsed, false if not
	 */
	bool parseRailGroupLine(parsecontext_t c);

	/**
	 * @param line line in configuration file
	 * @return true if line was successfully parsed, false if not
	 */
	bool parseSlideNodeLine(parsecontext_t c);

	// utility methods /////////////////////////////////////////////////////////

	/**
	 * searches the RailGRoup array for a rail with the corresponding id value
	 * @param id of rail group to search for
	 * @return NULL if no rail group is found, otherwise the corresponding rail group.
	 */
	RailGroup* getRailFromID(unsigned int id);

	/**
	 * wrapper for getRails, converts the list of strings to a compatible format
	 * @param railStrings list of node id's in string format
	 * @return same as getRails
	 */
	Rail* parseRailString( const Ogre::StringVector &railStrings, parsecontext_t c);

	/**
	 * parses an array of nodes id's to generate a Rail
	 * @param nodeids a list of node id's
	 * @return NULL if nodes do not form a continuous beam structure
	 */
	Rail* getRails(const std::vector<int>& nodeids, parsecontext_t c);

	/**
	 * Finds the beam instance based on the node IDs
	 * @param node1ID node id of one node
	 * @param node2ID node id of the other node
	 * @return pointer to beam instance, NULL if no beam is found
	 */
	beam_t* getBeam(unsigned int node1ID, unsigned int node2ID);

	/**
	 * Finds the beam based on actual node instances
	 * @param node1 first node of the beam
	 * @param node2 second node of the beam
	 * @return pointer to beam instance, NULL if no beam is found
	 */
	beam_t* getBeam(node_t* node1, node_t* node2);

	/**
	 * Find node instance baed on the id
	 * @param id of the node we are looking for
	 * @return pointer to node instance, NULL if no beam is found
	 */
	node_t* getNode(unsigned int id);

	void wash_calculator(Ogre::Quaternion rot, parsecontext_t c);
	void calcBox();

	void parser_warning(parsecontext_t &context, Ogre::String text);
	void parser_warning(parsecontext_t *context, Ogre::String text);
	bool checkResults(int res, int min, parsecontext_t &context);
};


#endif //SERIALIZEDRIG_H__

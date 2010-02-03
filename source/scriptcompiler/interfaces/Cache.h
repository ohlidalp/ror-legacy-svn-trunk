#ifndef __Cache_H__
#define __Cache_H__

/**
 *  @brief Settings class
 */
class Cache_Entry
{
public:
	std::string minitype;  //!< the mini-preview type, either png or dds
	std::string fname;
	std::string fname_without_uid;
	std::string dname;
	int categoryid;
	std::string categoryname;
	int addtimestamp;
	std::string uniqueid;
	int version;
	std::string fext;
	std::string type;
	std::string dirname;
	std::string hash;
	bool resourceLoaded;
	int number;
	bool changedornew;
	bool deleted;
	int usagecounter;
	std::string filecachename;
	std::string description;
	std::string tags;
	int fileformatversion;
	bool hasSubmeshs;
	int nodecount;
	int beamcount;
	int shockcount;
	int fixescount;
	int hydroscount;
	int wheelcount;
	int propwheelcount;
	int commandscount;
	int flarescount;
	int propscount;
	int wingscount;
	int turbopropscount;
	int turbojetcount;
	int rotatorscount;
	int exhaustscount;
	int flexbodiescount;
	int materialflarebindingscount;
	int soundsourcescount;
	int managedmaterialscount;
	float truckmass;
	float loadmass;
	float minrpm;
	float maxrpm;
	float torque;
	bool customtach;
	bool custom_particles;
	bool forwardcommands;
	bool importcommands;
	bool rollon;
	bool rescuer;
	int driveable;
	int numgears;
	unsigned char enginetype;

	// AS utils
	void addRef(){};
	void release(){};
};

#endif //__Cache_H__

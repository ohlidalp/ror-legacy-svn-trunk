
#include "string.h"

#include "serializer.h"
#include "BeamData.h"
#include "rormemory.h"
#include "serializationmodules.h"

#include "OgreResourceGroupManager.h"

using namespace Ogre;


RoRSerializer::RoRSerializer()
{
	// register all available modules :)
	new GlobalsSerializer(this);
	new NodeSerializer(this);
	new BeamSerializer(this);

	new FileInfoSerializer(this);
	new AuthorSerializer(this);
	new EngineSerializer(this);
	new CamerasSerializer(this);
	new ShocksSerializer(this);
	new HydrosSerializer(this);
}

RoRSerializer::~RoRSerializer()
{
}

int RoRSerializer::loadRig(Ogre::DataStreamPtr ds, rig_t *rig)
{
	//log(INFO, "loading Rig from %s ...", filename);
	if(!rig) return 1;

	// clear rig
	memset(rig, 0, sizeof(rig_t));

	// file things
	char line[1024];
	int linecounter = 0;
	std::string activeSection;

	// first: init rig
	// TODO: IMPORTANT: FIX the rig_t initialization
	rig->patchEngineTorque = false;
	rig->forwardcommands = 0;
	rig->importcommands = 0;
	rig->wheel_contact_requested = false;
	rig->rescuer = false;
	rig->disable_default_sounds = false;

	// read in truckname
	ds->readLine(line, 1023);
	strncpy(rig->realtruckname, line, 255);

	// loop through all the files lines
	SerializationContext ctx("foobar", 0);

	while (!ds->eof())
	{
		size_t ll = ds->readLine(line, 1023);
		linecounter++;
		// ignore comments and empty lines
		if (ll==0 || line[0]==';' || line[0]=='/')
			continue;

		// update context
		ctx.lineNo = linecounter;

		// now process the modules and try to parse the line
		// -1 = no match
		int res = processModules(line, rig, &ctx, activeSection);
		if(!activeSection.empty() && res == 0)
		{
			LogManager::getSingleton().logMessage("line section parsing with no result: " + String(line));
		}
		if(res == -1)
		{
			LogManager::getSingleton().logMessage("line with no match: " + String(line));
		}

	}
	LogManager::getSingleton().logMessage("done loading");
	return 0;
}

int RoRSerializer::saveRig(std::string filename, rig_t *rig)
{
	// XXX: TODO
	return 0;
}

int RoRSerializer::registerModuleSerializer(RoRSerializationModule *module)
{
	modules[module->getName()] = module;
	return 0;
}

int RoRSerializer::processModules(char *line, rig_t *rig, SerializationContext *ctx, std::string &activeSection)
{
	// parse for commands or other sections
	std::map < std::string, RoRSerializationModule *>::iterator it;
	for(it = modules.begin(); it != modules.end() ; it++)
	{
		// check if that command is matched
		std::string *cmd = &it->second->commandTrigger;
		if(cmd->size() && !strncmp(cmd->c_str(), line, cmd->size()))
		{
			// match, using this module
			return it->second->deserialize(line, rig);
		}

		// check for a new section
		std::string *sec = &it->second->sectionTrigger;
		if(sec->size() && !strcmp(sec->c_str(), line))
		{
			// match, using this module
			//set section as active
			activeSection = it->first;
			// parse this as well, could be that the section header contains information as well
			return it->second->deserialize(line, rig);
		}
	}

	// if we are in a section, parse it in its module handler
	if(!activeSection.empty())
	{
		// just try to use that section and ignore the others
		return modules[activeSection]->deserialize(line, rig);
	}

	// no match
	return -1;
}
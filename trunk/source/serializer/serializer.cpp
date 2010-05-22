
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

	new EngineSerializer(this);
	new CamerasSerializer(this);
	new ShocksSerializer(this);
	new HydrosSerializer(this);

	new WheelsSerializer(this);
	new ContactersSerializer(this);

	new BrakesSerializer(this);

	new FileInfoSerializer(this);
	new AuthorSerializer(this);

	new DummySerializer(this);
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
	bool activeSectionExplicit = false;
	
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
		// -1 = error, 0 = no match, > 0 = n matches
		int res = processModules(line, rig, &ctx, activeSection, activeSectionExplicit);
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
	modules.push_back(module);
	return 0;
}
void RoRSerializer::addSectionHandler(std::string section, RoRSerializationModule *module)
{
	sections[section] = module;
}

void RoRSerializer::setSectionExplicit(std::string section, bool value)
{
	if(value)
	{
		// add it to the map
		explictSections[section] = true;
	} else if(!value && explictSections.find(section) != explictSections.end())
	{
		// remove it from the map
		explictSections.erase(explictSections.find(section));
	}
}

void RoRSerializer::addCommandHandler(std::string command, RoRSerializationModule *module)
{
	commands[command] = module;
}

int RoRSerializer::processModules(char *line, rig_t *rig, SerializationContext *ctx, std::string &activeSection, bool &activeSectionExplicit)
{
	std::string linestr = std::string(line);
	// parse for commands
	std::map < std::string, RoRSerializationModule *>::iterator it;
	for(it = commands.begin(); it != commands.end() ; it++)
	{
		if(!it->first.size()) continue;

		// check if that command is matched
		if(linestr.substr(0, it->first.size()+2) == "=="+it->first)
		{
			// new format
			return it->second->deserialize(line+2, rig);
		} else if(linestr.substr(0, it->first.size()) == it->first)
		{
			// old format
			return it->second->deserialize(line, rig);
		}
	}

	// parse for sections
	for(it = sections.begin(); it != sections.end() ; it++)
	{
		if(!it->first.size()) continue;
		// check for a new section
		if(!activeSectionExplicit && it->first == linestr || "=" + it->first == linestr)
		{
			// match, using this module
			//set section as active
			activeSection = it->first;

			// find out whether its explicit
			activeSectionExplicit = (explictSections.find(activeSection) != explictSections.end());

			// parse this as well, could be that the section header contains information as well
			return it->second->deserialize(line, rig, activeSection);
		}

		// check for a section end
		if("end_" + it->first == linestr || "=end_"+it->first == linestr)
		{
			// found section end
			activeSection = std::string();
			activeSectionExplicit = false;
			return 1;
		}
	}

	// if we are in a section, parse it in its module handler
	if(!activeSection.empty())
	{
		// just try to use that section and ignore the others
		return sections[activeSection]->deserialize(line, rig, activeSection);
	}

	// no match
	return -1;
}

int RoRSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	std::vector < RoRSerializationModule *>::iterator it;
	for(it = modules.begin(); it != modules.end() ; it++)
	{
		(*it)->initResources(manager, node, rig);
	}
	return 0;
}

RoRSerializationModule *RoRSerializer::getSectionModule(rig_t *rig, std::string section)
{
	if(sections.find(section) == sections.end())
	{
		// TODO: throw error
		return 0;
	}
	if(!sections[section]->isInitiated())
	{
		sections[section]->initData(rig);
	}
	return sections[section];
}

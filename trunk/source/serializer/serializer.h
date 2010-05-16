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
#ifndef SERIALIZER_H__
#define SERIALIZER_H__

#include <string.h>
#include <map>

#include "RoRPrerequisites.h"
#include "BeamData.h"

#include "OgreDataStream.h"
#include "OgreLogManager.h"

class RoRSerializer;
class RoRSerializationModule;

// TODO: make this class more independend from Ogre

class SerializationContext
{
public:
	SerializationContext(std::string filename, int lineNo) : 
		filename(filename), lineNo(lineNo)
	{
	}
	
	std::string filename;
	int lineNo;
};

class RoRSerializer
{
	friend class RoRSerializationModule;
public:
	RoRSerializer();
	~RoRSerializer();

	int loadRig(Ogre::DataStreamPtr ds, rig_t *rig);
	int saveRig(std::string filename, rig_t *rig);

	int registerModuleSerializer(RoRSerializationModule *module);
	void addSectionHandler(std::string section, RoRSerializationModule *module);
	void addCommandHandler(std::string section, RoRSerializationModule *module);

	int initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig);

	RoRSerializationModule *getSectionModule(rig_t *rig, std::string section);

protected:
	int processModules(char *line, rig_t *rig, SerializationContext *ctx, std::string &activeSection);


	std::map < std::string, RoRSerializationModule *> sections;
	std::map < std::string, RoRSerializationModule *> commands;
	std::vector <RoRSerializationModule *> modules;
};

class RoRSerializationModule
{
	friend class RoRSerializer;
public:
	RoRSerializationModule(RoRSerializer *s) :
		s(s),
		name(),
		initiated(false)
	{
	}

	~RoRSerializationModule() {}

	virtual int deserialize(char *line, rig_t *rig, std::string activeSection = std::string()) = 0;
	virtual int serialize(char *line, rig_t *rig) = 0;
	virtual int initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig) = 0;

	std::string getName() { return name; };
	bool isInitiated() { return initiated; };
protected:
	RoRSerializer *s;
	std::string name;
	bool initiated;

	// some utils
	int checkRes(int minArgs, int result)
	{
		if (result < 1 || result == EOF)
		{
			Ogre::LogManager::getSingleton().logMessage("Error parsing " + this->getName() + "");
			// TODO output more info about the problem
		}
		return result;
	}
	
	virtual void initData(rig_t *rig) = 0;

};

#endif // SERIALIZER_H__

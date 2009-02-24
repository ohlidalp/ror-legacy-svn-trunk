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
// created on 24th of February 2009 by Thomas Fischer
#ifdef ANGELSCRIPT

#ifndef SCRIPTENGINE_H__
#define SCRIPTENGINE_H__

#define AS_INTERFACE_VERSION "0.1.0"

#include "angelscript.h"
#include <string>


#include "Ogre.h"

class ExampleFrameListener;
class GameScript;

class ScriptEngine
{
public:
	ScriptEngine(ExampleFrameListener *efl);
	~ScriptEngine();
	int loadTerrainScript(Ogre::String terrainname);

protected:
    ExampleFrameListener *mefl;
    asIScriptEngine *engine;

    void init();
    
    void msgCallback(const asSMessageInfo *msg, void *param);
	int loadScriptFile(const char *fileName, std::string &script);

};

class GameScript
{
protected:
	ScriptEngine *mse;
	ExampleFrameListener *mefl;

public:
	GameScript(ScriptEngine *se, ExampleFrameListener *efl);
	~GameScript();

	void log(std::string &msg);
	double getTime();
};

#endif

#endif //SCRIPTENGINE_H__

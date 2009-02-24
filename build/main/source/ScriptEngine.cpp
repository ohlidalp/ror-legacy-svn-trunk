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

#include "ScriptEngine.h"
#include "Ogre.h"
#include "ExampleFrameListener.h"
#include "scriptstring/scriptstring.h" // angelscript addon

using namespace Ogre;
using namespace std;

ScriptEngine::ScriptEngine(ExampleFrameListener *efl) : mefl(efl)
{
	init();
}

ScriptEngine::~ScriptEngine()
{
	// Clean up
	engine->Release();
}

int ScriptEngine::loadTerrainScript(Ogre::String scriptname)
{
	// Load the entire script file into the buffer
	int result=0;
	string script;
	result = loadScriptFile(scriptname.c_str(), script);
	if( result )
	{
		LogManager::getSingleton().logMessage("SE| Unkown error while loading script file: "+scriptname);
		return 1;
	}

	// Add the script to the module as a section. If desired, multiple script
	// sections can be added to the same module. They will then be compiled
	// together as if it was one large script. 
	asIScriptModule *mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
	result = mod->AddScriptSection(scriptname.c_str(), script.c_str(), script.length());
	if( result < 0 )
	{
		LogManager::getSingleton().logMessage("SE| Unkown error while adding script section");
		return 1;
	}

	// Build the module
	result = mod->Build();
	if( result < 0 )
	{
		if(result == asINVALID_CONFIGURATION)
		{
			LogManager::getSingleton().logMessage("SE| The engine configuration is invalid.");
			return 1;
		} else if(result == asERROR)
		{
			LogManager::getSingleton().logMessage("SE| The script failed to build. ");
			return 1;
		} else if(result == asBUILD_IN_PROGRESS)
		{
			LogManager::getSingleton().logMessage("SE| Another thread is currently building.");
			return 1;
		}
		LogManager::getSingleton().logMessage("SE| Unkown error while building the script");
		return 1;
	}
    
	// Find the function that is to be called. 
	int funcId = mod->GetFunctionIdByDecl("void main()");
	if( funcId < 0 )
	{
		// The function couldn't be found. Instruct the script writer to include the
		// expected function in the script.
		LogManager::getSingleton().logMessage("SE| The script must have the function 'void main()'. Please add it and try again.");
		return 1;
	}

	// Create our context, prepare it, and then execute
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(funcId);
	result = ctx->Execute();
	if( result != asEXECUTION_FINISHED )
	{
		// The execution didn't complete as expected. Determine what happened.
		if( result == asEXECUTION_EXCEPTION )
		{
			// An exception occurred, let the script writer know what happened so it can be corrected.
			LogManager::getSingleton().logMessage("SE| An exception '" + String(ctx->GetExceptionString()) + "' occurred. Please correct the code in file '" + scriptname + "' and try again.");
		}
	}

	ctx->Release();
	return 0;
}

void ScriptEngine::init()
{
	int result;
	// Create the script engine
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Set the message callback to receive information on errors in human readable form.
	// It's recommended to do this right after the creation of the engine, because if
	// some registration fails the engine may send valuable information to the message
	// stream.
	result = engine->SetMessageCallback(asMETHOD(ScriptEngine,msgCallback), this, asCALL_THISCALL);
	if(result < 0)
	{
		if(result == asINVALID_ARG)
		{
			LogManager::getSingleton().logMessage("SE| One of the arguments is incorrect, e.g. obj is null for a class method.");
			return;
		} else if(result == asNOT_SUPPORTED)
		{
			LogManager::getSingleton().logMessage("SE| 	The arguments are not supported, e.g. asCALL_GENERIC.");
			return;
		}
		LogManager::getSingleton().logMessage("SE| Unkown error while setting up message callback");
		return;
	}

	// AngelScript doesn't have a built-in string type, as there is no definite standard 
	// string type for C++ applications. Every developer is free to register it's own string type.
	// The SDK do however provide a standard add-on for registering a string type, so it's not
	// necessary to register your own string type if you don't want to.
	RegisterScriptString_Native(engine);

	asString<char *>::Register(engine);


	// Register everything
	result = engine->RegisterObjectType("GameScript", sizeof(GameScript), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	result = engine->RegisterObjectMethod("GameScript", "void log(const string &in)", asMETHOD(GameScript,log), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScript", "double getTime()", asMETHOD(GameScript,getTime), asCALL_THISCALL);

	// ARGHHHH, not working :(
	//result = engine->RegisterGlobalFunction("int parseInt(asBSTR *s)", asFUNCTION(Ogre::StringConverter::parseInt), asCALL_CDECL);
	//result = engine->RegisterGlobalFunction("bstr toString(int in)", asFUNCTION(myToString), asCALL_CDECL);

	GameScript *gamescript = new GameScript(this, mefl);
	result = engine->RegisterGlobalProperty("GameScript game", gamescript);
	
	LogManager::getSingleton().logMessage("SE| Registration done");
}

void ScriptEngine::msgCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "Error";
	if( msg->type == asMSGTYPE_INFORMATION )
		type = "Info";
	else if( msg->type == asMSGTYPE_WARNING )
		type = "Warning";

	char tmp[1024]="";
	sprintf(tmp, "SE| %s (%d, %d): %s = %s", msg->section, msg->row, msg->col, type, msg->message);
	LogManager::getSingleton().logMessage(tmp);
}

int ScriptEngine::loadScriptFile(const char *fileName, string &script)
{
	try
	{
		DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource(fileName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		script.resize(ds->size());
		ds->read(&script[0], ds->size());
		return 0;
	} catch(...)
	{
		return 1;
	}
	// DO NOT CLOSE THE STREAM (it closes automatically)
	return 1;
} 


/* class that implements the interface for the scripts */ 
GameScript::GameScript(ScriptEngine *se, ExampleFrameListener *efl) : mse(se), mefl(efl)
{
}

GameScript::~GameScript()
{
}

void GameScript::log(std::string &msg)
{
	Ogre::LogManager::getSingleton().logMessage("SE| LOG: " + msg);
}

double GameScript::getTime()
{
	return this->mefl->getTime();
}

#endif //ANGELSCRIPT

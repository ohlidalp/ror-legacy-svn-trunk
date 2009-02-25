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
#include "scriptmath/scriptmath.h" // angelscript addon
#include "water.h"
#include "Beam.h"

using namespace Ogre;
using namespace std;

ScriptEngine::ScriptEngine(ExampleFrameListener *efl) : mefl(efl), engine(0), context(0), frameStepFunctionPtr(-1)
{
	init();
}

ScriptEngine::~ScriptEngine()
{
	// Clean up
	engine->Release();
	if(context) context->Release();
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
	asIScriptModule *mod = engine->GetModule("terrainScript", asGM_ALWAYS_CREATE);
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

	// get some other optional functions
	frameStepFunctionPtr = mod->GetFunctionIdByDecl("void frameStep(float)");

	// Create our context, prepare it, and then execute
	context = engine->CreateContext();
	
	//context->SetLineCallback(asMETHOD(ScriptEngine,LineCallback), this, asCALL_THISCALL);
	
	// this does not work :(
	//context->SetExceptionCallback(asMETHOD(ScriptEngine,ExceptionCallback), this, asCALL_THISCALL);
	
	context->Prepare(funcId);
	LogManager::getSingleton().logMessage("SE| Executing main()");
	result = context->Execute();
	if( result != asEXECUTION_FINISHED )
	{
		// The execution didn't complete as expected. Determine what happened.
		if( result == asEXECUTION_EXCEPTION )
		{
			// An exception occurred, let the script writer know what happened so it can be corrected.
			LogManager::getSingleton().logMessage("SE| An exception '" + String(context->GetExceptionString()) + "' occurred. Please correct the code in file '" + scriptname + "' and try again.");
		}
	}

	return 0;
}

void ScriptEngine::ExceptionCallback(asIScriptContext *ctx, void *param)
{
	asIScriptEngine *engine = ctx->GetEngine();
	int funcID = ctx->GetExceptionFunction();
	const asIScriptFunction *function = engine->GetFunctionDescriptorById(funcID);
	LogManager::getSingleton().logMessage("--- exception ---");
	LogManager::getSingleton().logMessage("desc: " + String(ctx->GetExceptionString()));
	LogManager::getSingleton().logMessage("func: " + String(function->GetDeclaration()));
	LogManager::getSingleton().logMessage("modl: " + String(function->GetModuleName()));
	LogManager::getSingleton().logMessage("sect: " + String(function->GetScriptSectionName()));
	int col, line = ctx->GetExceptionLineNumber(&col);
	LogManager::getSingleton().logMessage("line: "+StringConverter::toString(line)+","+StringConverter::toString(col));

	// Print the variables in the current function
	PrintVariables(ctx, -1);

	// Show the call stack with the variables
	LogManager::getSingleton().logMessage("--- call stack ---");
	for( int n = 0; n < ctx->GetCallstackSize(); n++ )
	{
		funcID = ctx->GetCallstackFunction(n);
		const asIScriptFunction *func = engine->GetFunctionDescriptorById(funcID);
		line = ctx->GetCallstackLineNumber(n,&col);
		LogManager::getSingleton().logMessage(String(func->GetModuleName()) + ":" + func->GetDeclaration() + ":" + StringConverter::toString(line)+ "," + StringConverter::toString(col));

		PrintVariables(ctx, n);
	}
}

void ScriptEngine::LineCallback(asIScriptContext *ctx, void *param)
{
	char tmp[1024]="";
	asIScriptEngine *engine = ctx->GetEngine();
	int funcID = ctx->GetCurrentFunction();
	int col;
	int line = ctx->GetCurrentLineNumber(&col);
	int indent = ctx->GetCallstackSize();
	for( int n = 0; n < indent; n++ )
		sprintf(tmp+n," ");
	const asIScriptFunction *function = engine->GetFunctionDescriptorById(funcID);
	sprintf(tmp+indent,"%s:%s:%d,%d", function->GetModuleName(),
	                    function->GetDeclaration(),
	                    line, col);

	LogManager::getSingleton().logMessage(tmp);

//	PrintVariables(ctx, -1);
}

void ScriptEngine::PrintVariables(asIScriptContext *ctx, int stackLevel)
{
	char tmp[1024]="";
	asIScriptEngine *engine = ctx->GetEngine();

	int typeId = ctx->GetThisTypeId(stackLevel);
	void *varPointer = ctx->GetThisPointer(stackLevel);
	if( typeId )
	{
		sprintf(tmp," this = 0x%x", varPointer);
		LogManager::getSingleton().logMessage(tmp);
	}

	int numVars = ctx->GetVarCount(stackLevel);
	for( int n = 0; n < numVars; n++ )
	{
		int typeId = ctx->GetVarTypeId(n, stackLevel); 
		void *varPointer = ctx->GetAddressOfVar(n, stackLevel);
		if( typeId == engine->GetTypeIdByDecl("int") )
		{
			sprintf(tmp, " %s = %d", ctx->GetVarDeclaration(n, 0, stackLevel), *(int*)varPointer);
			LogManager::getSingleton().logMessage(tmp);
		}
		else if( typeId == engine->GetTypeIdByDecl("string") )
		{
			CScriptString *str = (CScriptString*)varPointer;
			if( str )
			{			
				sprintf(tmp, " %s = '%s'", ctx->GetVarDeclaration(n, 0, stackLevel), str->buffer.c_str());
				LogManager::getSingleton().logMessage(tmp);
			} else
			{
				sprintf(tmp, " %s = <null>", ctx->GetVarDeclaration(n, 0, stackLevel));
				LogManager::getSingleton().logMessage(tmp);
			}
		LogManager::getSingleton().logMessage(tmp);
		}
	}
}

CScriptString &getTruckName(Beam *beam)
{
	CScriptString *str = new CScriptString(beam->getTruckName());
	return *str;
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
	RegisterScriptMath_Native(engine);

	// Register everything
	result = engine->RegisterObjectType("BeamClass", sizeof(Beam), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", asMETHOD(Beam,scaleTruck), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("BeamClass", "string &getTruckName()", asFUNCTION(getTruckName), asCALL_CDECL_OBJLAST);
	result = engine->RegisterObjectProperty("BeamClass", "float currentScale", offsetof(Beam, currentScale));


	result = engine->RegisterObjectType("GameScriptClass", sizeof(GameScript), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	result = engine->RegisterObjectMethod("GameScriptClass", "void log(const string &in)", asMETHOD(GameScript,log), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "double getTime()", asMETHOD(GameScript,getTime), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonPosition(float, float, float)", asMETHOD(GameScript,setPersonPosition), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "void movePerson(float, float, float)", asMETHOD(GameScript,movePerson), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getCaelumTime()", asMETHOD(GameScript,getCaelumTime), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setCaelumTime(float)", asMETHOD(GameScript,setCaelumTime), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setWaterHeight(float)", asMETHOD(GameScript,setWaterHeight), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getWaterHeight()", asMETHOD(GameScript,getWaterHeight), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getCurrentTruckNumber()", asMETHOD(GameScript,getCurrentTruckNumber), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucks()", asMETHOD(GameScript,getCurrentTruckNumber), asCALL_THISCALL);

	result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass &getCurrentTruck()", asMETHOD(GameScript,getCurrentTruck), asCALL_THISCALL);
	result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass &getTruckByNum(int)", asMETHOD(GameScript,getTruckByNum), asCALL_THISCALL);


	GameScript *gamescript = new GameScript(this, mefl);
	result = engine->RegisterGlobalProperty("GameScriptClass game", gamescript);

	result = engine->RegisterObjectType("Vector3Class", sizeof(Ogre::Vector3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	// TODO: add complete Vector3 class :(
	//result = engine->RegisterObjectMethod("Vector3Class", "...", asMETHOD(Ogre::Vector3,...), asCALL_THISCALL);


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

int ScriptEngine::framestep(Ogre::Real dt)
{
	if(frameStepFunctionPtr<0) return 1;
	context->Prepare(frameStepFunctionPtr);

	// Set the function arguments
	context->SetArgFloat(0, dt);

	LogManager::getSingleton().logMessage("SE| Executing framestep()");
	int r = context->Execute();
	if( r == asEXECUTION_FINISHED )
	{
	  // The return value is only valid if the execution finished successfully
	  asDWORD ret = context->GetReturnDWord();
	}
	return 0;
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

void GameScript::setPersonPosition(float x, float y, float z)
{
	if(mefl && mefl->person) mefl->person->setPosition(Vector3(x, y, z));
}

void GameScript::movePerson(float x, float y, float z)
{
	if(mefl && mefl->person) mefl->person->move(Vector3(x, y, z));
}

float GameScript::getCaelumTime()
{
	if(mefl && mefl->mCaelumSystem) return mefl->mCaelumSystem->getLocalTime();
	return 0;
}

void GameScript::setCaelumTime(float value)
{
	if(mefl && mefl->mCaelumSystem) mefl->mCaelumSystem->setLocalTime(value);
}

void GameScript::setWaterHeight(float value)
{
	if(mefl && mefl->w) mefl->w->setHeight(value);
}

float GameScript::getWaterHeight()
{
	if(mefl && mefl->w) return mefl->w->getHeight();
	return 0;
}

Beam *GameScript::getCurrentTruck()
{
	if(mefl) return mefl->getCurrentTruck();
	return 0;
}

Beam *GameScript::getTruckByNum(int num)
{
	if(mefl) return mefl->getTruck(num);
	return 0;
}

int GameScript::getNumTrucks()
{
	if(mefl) return mefl->getTruckCount();
	return 0;
}

int GameScript::getCurrentTruckNumber()
{
	if(mefl) return mefl->getCurrentTruckNumber();
	return -1;
}
#endif //ANGELSCRIPT

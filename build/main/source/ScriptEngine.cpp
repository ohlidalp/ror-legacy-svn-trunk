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
#include "scriptstdstring/scriptstdstring.h" // angelscript addon
#include "scriptmath3d/scriptmath3d.h" // angelscript addon
#include "scriptmath/scriptmath.h" // angelscript addon
#include "water.h"
#include "Beam.h"
#include "Settings.h"
#include "IngameConsole.h"
#include "CacheSystem.h"
#include "gui_loader.h"
#include "as_ogre.h"

using namespace Ogre;
using namespace std;
using namespace AngelScript;

template<> ScriptEngine *Ogre::Singleton<ScriptEngine>::ms_Singleton=0;

ScriptEngine::ScriptEngine(ExampleFrameListener *efl) : mefl(efl), engine(0), context(0), frameStepFunctionPtr(-1), eventMask(0)
{
	init();
}

ScriptEngine::~ScriptEngine()
{
	// Clean up
	if(engine)  engine->Release();
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
	AngelScript::asIScriptModule *mod = engine->GetModule("terrainScript", asGM_ALWAYS_CREATE);
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
	eventCallbackFunctionPtr = mod->GetFunctionIdByDecl("void eventCallback(int event, int value)");

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
		sprintf(tmp," this = 0x%x", (unsigned int)varPointer);
		LogManager::getSingleton().logMessage(tmp);
	}

	int numVars = ctx->GetVarCount(stackLevel);
	for( int n = 0; n < numVars; n++ )
	{
		int typeId = ctx->GetVarTypeId(n, stackLevel);
		void *varPointer = ctx->GetAddressOfVar(n, stackLevel);
		if( typeId == engine->GetTypeIdByDecl("int") )
		{
			sprintf(tmp, " %s = %d", ctx->GetVarDeclaration(n, stackLevel), *(int*)varPointer);
			LogManager::getSingleton().logMessage(tmp);
		}
		else if( typeId == engine->GetTypeIdByDecl("string") )
		{
			std::string *str = (std::string*)varPointer;
			if( str )
			{
				sprintf(tmp, " %s = '%s'", ctx->GetVarDeclaration(n, stackLevel), str->c_str());
				LogManager::getSingleton().logMessage(tmp);
			} else
			{
				sprintf(tmp, " %s = <null>", ctx->GetVarDeclaration(n, stackLevel));
				LogManager::getSingleton().logMessage(tmp);
			}
		LogManager::getSingleton().logMessage(tmp);
		}
	}
};

// continue with initializing everything
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
	RegisterStdString(engine);
	RegisterScriptMath(engine);
	RegisterScriptMath3D(engine);

	registerOgreObjects(engine);

	// Register everything
	// class Beam
	result = engine->RegisterObjectType("BeamClass", sizeof(Beam), asOBJ_REF); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", asMETHOD(Beam,scaleTruck), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "string getTruckName()", asMETHOD(Beam,getTruckName), asCALL_THISCALL); assert(result>=0);
//	result = engine->RegisterObjectMethod("BeamClass", "void resetPosition(float, float, bool, float)", asMETHOD(Beam,resetPosition), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setDetailLevel(int)", asMETHOD(Beam,setDetailLevel), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void showSkeleton(bool, bool)", asMETHOD(Beam,showSkeleton), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void hideSkeleton(bool)", asMETHOD(Beam,hideSkeleton), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", asMETHOD(Beam,parkingbrakeToggle), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void beaconsToggle()", asMETHOD(Beam,beaconsToggle), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setReplayMode(bool)", asMETHOD(Beam,setReplayMode), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void resetAutopilot()", asMETHOD(Beam,resetAutopilot), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void toggleCustomParticles()", asMETHOD(Beam,toggleCustomParticles), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", asMETHOD(Beam,parkingbrakeToggle), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getDefaultDeformation()", asMETHOD(Beam,getDefaultDeformation), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getNodeCount()", asMETHOD(Beam,getNodeCount), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getTotalMass(bool)", asMETHOD(Beam,getTotalMass), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getWheelNodeCount()", asMETHOD(Beam,getWheelNodeCount), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void recalc_masses()", asMETHOD(Beam,recalc_masses), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setMass(float)", asMETHOD(Beam,setMass), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getBrakeLightVisible()", asMETHOD(Beam,getBrakeLightVisible), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomLightVisible(int)", asMETHOD(Beam,getCustomLightVisible), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setCustomLightVisible(int, bool)", asMETHOD(Beam,setCustomLightVisible), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getBeaconMode()", asMETHOD(Beam,getBeaconMode), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setBlinkType(int)", asMETHOD(Beam,setBlinkType), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getBlinkType()", asMETHOD(Beam,getBlinkType), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", asMETHOD(Beam,getCustomParticleMode), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getLowestNode()", asMETHOD(Beam,getLowestNode), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool setMeshVisibility(bool)", asMETHOD(Beam,setMeshVisibility), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", asMETHOD(Beam,getCustomParticleMode), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", asMETHOD(Beam,getCustomParticleMode), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getReverseLightVisible()", asMETHOD(Beam,getCustomParticleMode), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getHeadingDirectionAngle()", asMETHOD(Beam,getHeadingDirectionAngle), asCALL_THISCALL); assert(result>=0);
	/*
	// offsetof invalid for classes?
	result = engine->RegisterObjectProperty("BeamClass", "float brake", offsetof(Beam, brake)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float currentScale", offsetof(Beam, currentScale)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int nodedebugstate", offsetof(Beam, nodedebugstate)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int debugVisuals", offsetof(Beam, debugVisuals)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool networking", offsetof(Beam, networking)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int label", offsetof(Beam, label)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int trucknum", offsetof(Beam, trucknum)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float WheelSpeed", offsetof(Beam, WheelSpeed)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int skeleton", offsetof(Beam, skeleton)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool replaymode", offsetof(Beam, replaymode)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int replaylen", offsetof(Beam, replaylen)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int replaypos", offsetof(Beam, replaypos)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int locked", offsetof(Beam, locked)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool cparticle_enabled", offsetof(Beam, cparticle_enabled)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int hookId", offsetof(Beam, hookId)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "BeamClass @lockTruck", offsetof(Beam, lockTruck)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_node", offsetof(Beam, free_node)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int dynamicMapMode", offsetof(Beam, dynamicMapMode)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int tied", offsetof(Beam, tied)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int canwork", offsetof(Beam, canwork)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int hashelp", offsetof(Beam, hashelp)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float minx", offsetof(Beam, minx)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float maxx", offsetof(Beam, maxx)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float miny", offsetof(Beam, miny)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float maxy", offsetof(Beam, maxy)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float minz", offsetof(Beam, minz)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float maxz", offsetof(Beam, maxz)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int state", offsetof(Beam, state)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int sleepcount", offsetof(Beam, sleepcount)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int driveable", offsetof(Beam, driveable)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int importcommands", offsetof(Beam, importcommands)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool requires_wheel_contact", offsetof(Beam, requires_wheel_contact)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool wheel_contact_requested", offsetof(Beam, wheel_contact_requested)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool rescuer", offsetof(Beam, rescuer)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int parkingbrake", offsetof(Beam, parkingbrake)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int lights", offsetof(Beam, lights)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int smokeId", offsetof(Beam, smokeId)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int editorId", offsetof(Beam, editorId)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float leftMirrorAngle", offsetof(Beam, leftMirrorAngle)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float refpressure", offsetof(Beam, refpressure)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_pressure_beam", offsetof(Beam, free_pressure_beam)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int done_count", offsetof(Beam, done_count)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_prop", offsetof(Beam, free_prop)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float default_beam_diameter", offsetof(Beam, default_beam_diameter)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float skeleton_beam_diameter", offsetof(Beam, skeleton_beam_diameter)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_aeroengine", offsetof(Beam, free_aeroengine)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float elevator", offsetof(Beam, elevator)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float rudder", offsetof(Beam, rudder)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float aileron", offsetof(Beam, aileron)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int flap", offsetof(Beam, flap)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_wing", offsetof(Beam, free_wing)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float fadeDist", offsetof(Beam, fadeDist)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool disableDrag", offsetof(Beam, disableDrag)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int currentcamera", offsetof(Beam, currentcamera)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int freecinecamera", offsetof(Beam, freecinecamera)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float brakeforce", offsetof(Beam, brakeforce)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool ispolice", offsetof(Beam, ispolice)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int loading_finished", offsetof(Beam, loading_finished)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int freecamera", offsetof(Beam, freecamera)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int first_wheel_node", offsetof(Beam, first_wheel_node)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int netbuffersize", offsetof(Beam, netbuffersize)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int nodebuffersize", offsetof(Beam, nodebuffersize)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float speedoMax", offsetof(Beam, speedoMax)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool useMaxRPMforGUI", offsetof(Beam, useMaxRPMforGUI)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "string realtruckfilename", offsetof(Beam, realtruckfilename)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_wheel", offsetof(Beam, free_wheel)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int airbrakeval", offsetof(Beam, airbrakeval)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int cameranodecount", offsetof(Beam, cameranodecount)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_cab", offsetof(Beam, free_cab)); assert(result>=0);
	// wont work: result = engine->RegisterObjectProperty("BeamClass", "int airbrakeval", offsetof(Beam, airbrakeval)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool meshesVisible", offsetof(Beam, meshesVisible)); assert(result>=0);
	result = engine->RegisterObjectBehaviour("BeamClass", asBEHAVE_ADDREF, "void f()",asMETHOD(Beam,addRef), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectBehaviour("BeamClass", asBEHAVE_RELEASE, "void f()",asMETHOD(Beam,release), asCALL_THISCALL); assert(result>=0);
	*/

	// class Settings
	result = engine->RegisterObjectType("SettingsClass", sizeof(Settings), asOBJ_REF); assert(result>=0);
	result = engine->RegisterObjectMethod("SettingsClass", "string getSetting(const string &in)", asMETHOD(Settings,getSettingScriptSafe), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectBehaviour("SettingsClass", asBEHAVE_ADDREF, "void f()",asMETHOD(Settings,addRef), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectBehaviour("SettingsClass", asBEHAVE_RELEASE, "void f()",asMETHOD(Settings,release), asCALL_THISCALL); assert(result>=0);

	// class Cache_Entry
	result = engine->RegisterObjectType("Cache_EntryClass", sizeof(Cache_Entry), asOBJ_REF); assert(result>=0);
	/*
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string minitype", offsetof(Cache_Entry, minitype)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string fname", offsetof(Cache_Entry, fname)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string fname_without_uid", offsetof(Cache_Entry, fname_without_uid)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string dname", offsetof(Cache_Entry, dname)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int categoryid", offsetof(Cache_Entry, categoryid)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string categoryname", offsetof(Cache_Entry, categoryname)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int addtimestamp", offsetof(Cache_Entry, addtimestamp)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string uniqueid", offsetof(Cache_Entry, uniqueid)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int version", offsetof(Cache_Entry, version)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string fext", offsetof(Cache_Entry, fext)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string type", offsetof(Cache_Entry, type)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string dirname", offsetof(Cache_Entry, dirname)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string hash", offsetof(Cache_Entry, hash)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool resourceLoaded", offsetof(Cache_Entry, resourceLoaded)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int number", offsetof(Cache_Entry, number)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool changedornew", offsetof(Cache_Entry, changedornew)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool deleted", offsetof(Cache_Entry, deleted)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int usagecounter", offsetof(Cache_Entry, usagecounter)); assert(result>=0);
	// TODO: add Cache_Entry::authors
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string filecachename", offsetof(Cache_Entry, filecachename)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string description", offsetof(Cache_Entry, description)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "string tags", offsetof(Cache_Entry, tags)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int fileformatversion", offsetof(Cache_Entry, fileformatversion)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool hasSubmeshs", offsetof(Cache_Entry, hasSubmeshs)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int nodecount", offsetof(Cache_Entry, nodecount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int beamcount", offsetof(Cache_Entry, beamcount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int shockcount", offsetof(Cache_Entry, shockcount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int fixescount", offsetof(Cache_Entry, fixescount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int hydroscount", offsetof(Cache_Entry, hydroscount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int wheelcount", offsetof(Cache_Entry, wheelcount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int propwheelcount", offsetof(Cache_Entry, propwheelcount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int commandscount", offsetof(Cache_Entry, commandscount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int flarescount", offsetof(Cache_Entry, flarescount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int propscount", offsetof(Cache_Entry, propscount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int wingscount", offsetof(Cache_Entry, wingscount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int turbopropscount", offsetof(Cache_Entry, turbopropscount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int turbojetcount", offsetof(Cache_Entry, turbojetcount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int rotatorscount", offsetof(Cache_Entry, rotatorscount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int exhaustscount", offsetof(Cache_Entry, exhaustscount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int flexbodiescount", offsetof(Cache_Entry, flexbodiescount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int materialflarebindingscount", offsetof(Cache_Entry, materialflarebindingscount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int soundsourcescount", offsetof(Cache_Entry, soundsourcescount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int managedmaterialscount", offsetof(Cache_Entry, managedmaterialscount)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "float truckmass", offsetof(Cache_Entry, truckmass)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "float loadmass", offsetof(Cache_Entry, loadmass)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "float minrpm", offsetof(Cache_Entry, minrpm)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "float maxrpm", offsetof(Cache_Entry, maxrpm)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "float torque", offsetof(Cache_Entry, torque)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool customtach", offsetof(Cache_Entry, customtach)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool custom_particles", offsetof(Cache_Entry, custom_particles)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool forwardcommands", offsetof(Cache_Entry, forwardcommands)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool importcommands", offsetof(Cache_Entry, importcommands)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool rollon", offsetof(Cache_Entry, rollon)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "bool rescuer", offsetof(Cache_Entry, rescuer)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int driveable", offsetof(Cache_Entry, driveable)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "int numgears", offsetof(Cache_Entry, numgears)); assert(result>=0);
	result = engine->RegisterObjectProperty("Cache_EntryClass", "uint8 enginetype", offsetof(Cache_Entry, enginetype)); assert(result>=0);
	result = engine->RegisterObjectBehaviour("Cache_EntryClass", asBEHAVE_ADDREF, "void f()",asMETHOD(Cache_Entry,addRef), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectBehaviour("Cache_EntryClass", asBEHAVE_RELEASE, "void f()",asMETHOD(Cache_Entry,release), asCALL_THISCALL); assert(result>=0);
	// TODO: add Cache_Entry::sectionconfigs
	*/

	// todo: add Vector3 classes and other utility classes!

	// class GameScript
	result = engine->RegisterObjectType("GameScriptClass", sizeof(GameScript), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void log(const string &in)", asMETHOD(GameScript,log), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "double getTime()", asMETHOD(GameScript,getTime), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonPosition(vector3)", asMETHOD(GameScript,setPersonPosition), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getPersonPosition()", asMETHOD(GameScript,getPersonPosition), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void movePerson(float, float, float)", asMETHOD(GameScript,movePerson), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getCaelumTime()", asMETHOD(GameScript,getCaelumTime), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setCaelumTime(float)", asMETHOD(GameScript,setCaelumTime), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setWaterHeight(float)", asMETHOD(GameScript,setWaterHeight), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getWaterHeight()", asMETHOD(GameScript,getWaterHeight), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getCurrentTruckNumber()", asMETHOD(GameScript,getCurrentTruckNumber), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucks()", asMETHOD(GameScript,getNumTrucks), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getGravity()", asMETHOD(GameScript,getGravity), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setGravity(float)", asMETHOD(GameScript,setGravity), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void flashMessage(const string &in, float, float)", asMETHOD(GameScript,flashMessage), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setDirectionArrow(const string &in, vector3)", asMETHOD(GameScript,setDirectionArrow), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void hideDirectionArrow()", asMETHOD(GameScript,hideDirectionArrow), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void registerForEvent(int)", asMETHOD(GameScript,registerForEvent), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getCurrentTruck()", asMETHOD(GameScript,getCurrentTruck), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getTruckByNum(int)", asMETHOD(GameScript,getTruckByNum), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getChatFontSize()", asMETHOD(GameScript,getChatFontSize), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setChatFontSize(int)", asMETHOD(GameScript,setChatFontSize), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void showChooser(const string &in, const string &in, const string &in)", asMETHOD(GameScript,showChooser), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void repairVehicle(const string &in, const string &in)", asMETHOD(GameScript,repairVehicle), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void spawnObject(const string &in, const string &in, float, float, float, float, float, float, const string &in, bool)", asMETHOD(GameScript,spawnObject), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialAmbient(const string &in, float, float, float)", asMETHOD(GameScript,setMaterialAmbient), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialDiffuse(const string &in, float, float, float, float)", asMETHOD(GameScript,setMaterialDiffuse), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialSpecular(const string &in, float, float, float, float)", asMETHOD(GameScript,setMaterialSpecular), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialEmissive(const string &in, float, float, float)", asMETHOD(GameScript,setMaterialEmissive), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucksByFlag(int)", asMETHOD(GameScript,getNumTrucksByFlag), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "bool getCaelumAvailable()", asMETHOD(GameScript,getCaelumAvailable), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void startTimer()", asMETHOD(GameScript,startTimer), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void stopTimer()", asMETHOD(GameScript,stopTimer), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float rangeRandom(float, float)", asMETHOD(GameScript,rangeRandom), asCALL_THISCALL); assert(result>=0);

	// class CacheSystem
	result = engine->RegisterObjectType("CacheSystemClass", sizeof(CacheSystem), asOBJ_REF | asOBJ_NOHANDLE);
	result = engine->RegisterObjectMethod("CacheSystemClass", "bool checkResourceLoaded(string &in)", asMETHODPR(CacheSystem, checkResourceLoaded, (Ogre::String &), bool), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("CacheSystemClass", "bool checkResourceLoaded(string &in, string &in)", asMETHODPR(CacheSystem, checkResourceLoaded, (Ogre::String &, Ogre::String &), bool), asCALL_THISCALL); assert(result>=0);
	// unable to register the following:
	//result = engine->RegisterObjectMethod("CacheSystemClass", "bool checkResourceLoaded(Cache_EntryClass)", asMETHODPR(CacheSystem, checkResourceLoaded, (Cache_Entry), bool), asCALL_THISCALL); assert_net(result>=0);
	//result = engine->RegisterObjectMethod("CacheSystemClass", "Cache_EntryClass &getResourceInfo(string)", asMETHODPR(CacheSystem, getResourceInfo, (Cache_Entry), bool), asCALL_THISCALL); assert_net(result>=0);
	//result = engine->RegisterObjectMethod("CacheSystemClass", "Cache_EntryClass &getResourceInfo(string)", asMETHODPR(CacheSystem, getResourceInfo, (Cache_Entry), bool), asCALL_THISCALL); assert_net(result>=0);
	//result = engine->RegisterObjectMethod("CacheSystemClass", "Cache_EntryClass &getResourceInfo(string)", asMETHODPR(CacheSystem, getResourceInfo, (Cache_Entry), bool), asCALL_THISCALL); assert_net(result>=0);
	// these are static methods, special handling for them :)
	result = engine->RegisterGlobalFunction("string stripUIDfromString(string)", asFUNCTION(CacheSystem::stripUIDfromString), asCALL_CDECL); assert(result>=0);
	result = engine->RegisterGlobalFunction("string getUIDfromString(string)", asFUNCTION(CacheSystem::getUIDfromString), asCALL_CDECL); assert(result>=0);
	result = engine->RegisterGlobalFunction("bool stringHasUID(string)", asFUNCTION(CacheSystem::stringHasUID), asCALL_CDECL); assert(result>=0);

	// enum scriptEvents
	result = engine->RegisterEnum("scriptEvents"); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_COLLISION_BOX_ENTER", SE_COLLISION_BOX_ENTER); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_COLLISION_BOX_LEAVE", SE_COLLISION_BOX_LEAVE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENTER", SE_TRUCK_ENTER); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_EXIT", SE_TRUCK_EXIT); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_DIED", SE_TRUCK_ENGINE_DIED); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_FIRE", SE_TRUCK_ENGINE_FIRE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TOUCHED_WATER", SE_TRUCK_TOUCHED_WATER); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEAM_BROKE", SE_TRUCK_BEAM_BROKE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_LOCKED", SE_TRUCK_LOCKED); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_UNLOCKED", SE_TRUCK_UNLOCKED); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_LIGHT_TOGGLE", SE_TRUCK_LIGHT_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_SKELETON_TOGGLE", SE_TRUCK_SKELETON_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TIE_TOGGLE", SE_TRUCK_TIE_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_PARKINGBREAK_TOGGLE", SE_TRUCK_PARKINGBREAK_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEACONS_TOGGLE", SE_TRUCK_BEACONS_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_CPARTICLES_TOGGLE", SE_TRUCK_CPARTICLES_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_GROUND_CONTACT_CHANGED", SE_TRUCK_GROUND_CONTACT_CHANGED); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_NEW_TRUCK", SE_GENERIC_NEW_TRUCK); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_DELETED_TRUCK", SE_GENERIC_DELETED_TRUCK); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_INPUT_EVENT", SE_GENERIC_INPUT_EVENT); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MOUSE_BEAM_INTERACTION", SE_GENERIC_MOUSE_BEAM_INTERACTION); assert(result>=0);

	result = engine->RegisterEnum("truckStates"); assert(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_ACTIVATED", ACTIVATED); assert(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_DESACTIVATED", DESACTIVATED); assert(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_MAYSLEEP", MAYSLEEP); assert(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_GOSLEEP", GOSLEEP); assert(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_SLEEPING", SLEEPING); assert(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_NETWORKED", NETWORKED); assert(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_RECYCLE", RECYCLE); assert(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_DELETED", DELETED); assert(result>=0);

	// now the global instances
	GameScript *gamescript = new GameScript(this, mefl);
	result = engine->RegisterGlobalProperty("GameScriptClass game", gamescript); assert(result>=0);
	result = engine->RegisterGlobalProperty("CacheSystemClass cache", &CacheSystem::Instance()); assert(result>=0);
	result = engine->RegisterGlobalProperty("SettingsClass settings", &SETTINGS); assert(result>=0);








	LogManager::getSingleton().logMessage("SE| Registration done");
}

void ScriptEngine::msgCallback(const asSMessageInfo *msg)
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
	if(!engine) return 0;
	if(!context) context = engine->CreateContext();
	context->Prepare(frameStepFunctionPtr);

	// Set the function arguments
	context->SetArgFloat(0, dt);

	//LogManager::getSingleton().logMessage("SE| Executing framestep()");
	int r = context->Execute();
	if( r == asEXECUTION_FINISHED )
	{
	  // The return value is only valid if the execution finished successfully
	  asDWORD ret = context->GetReturnDWord();
	}
	return 0;
}

int ScriptEngine::executeString(Ogre::String command)
{
	// TOFIX: add proper error output
	if(!engine) return 1;
	if(!context) context = engine->CreateContext();

	int result = engine->ExecuteString("terrainScript", command.c_str(), &context);
	if(result<0)
	{
		LogManager::getSingleton().logMessage("error " + StringConverter::toString(result) + " while executing string: " + command + ".");
	}
	return result;
}

void ScriptEngine::triggerEvent(enum scriptEvents eventnum, int value)
{
	if(!engine) return;
	if(eventCallbackFunctionPtr<0) return;
	if(eventMask & eventnum)
	{
		// script registered for that event, so sent it
		if(!context) context = engine->CreateContext();
		context->Prepare(eventCallbackFunctionPtr);

		// Set the function arguments
		context->SetArgDWord(0, eventnum);
		context->SetArgDWord(1, value);

		int r = context->Execute();
		if( r == asEXECUTION_FINISHED )
		{
		  // The return value is only valid if the execution finished successfully
		  asDWORD ret = context->GetReturnDWord();
		}
		return;
	}
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
	Ogre::LogManager::getSingleton().logMessage("SE| " + msg);
}

double GameScript::getTime()
{
	return this->mefl->getTime();
}

void GameScript::setPersonPosition(Ogre::Vector3 vec)
{
	if(mefl && mefl->person) mefl->person->setPosition(Ogre::Vector3(vec.x, vec.y, vec.z));
}

Ogre::Vector3 GameScript::getPersonPosition()
{
	if(mefl && mefl->person)
	{
		Ogre::Vector3 ov = mefl->person->getPosition();
		return Ogre::Vector3(ov.x, ov.y, ov.z);
	}
	return Ogre::Vector3::ZERO;
}

void GameScript::movePerson(Ogre::Vector3 vec)
{
	if(mefl && mefl->person) mefl->person->move(Ogre::Vector3(vec.x, vec.y, vec.z));
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

bool GameScript::getCaelumAvailable()
{
	if(mefl && mefl->mCaelumSystem) return true;
	return false;
}

void GameScript::stopTimer()
{
	if(mefl) mefl->stopTimer();
}

void GameScript::startTimer()
{
	if(mefl) mefl->startTimer();
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

float GameScript::getGravity()
{
	if(mefl) return mefl->getGravity();
	return 0;
}

void GameScript::setGravity(float value)
{
	if(mefl) mefl->setGravity(value);
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

int GameScript::getNumTrucksByFlag(int flag)
{
	int res = 0;
	for(int i=0; i< mefl->getTruckCount(); i++)
	{
		Beam *truck = mefl->getTruck(i);
		if(!truck && !flag)
			res++;
		if(!truck) continue;
		if(truck->state == flag)
			res++;
	}
	return res;
}

int GameScript::getCurrentTruckNumber()
{
	if(mefl) return mefl->getCurrentTruckNumber();
	return -1;
}

void GameScript::registerForEvent(int eventValue)
{
	if(!mse) return;
	mse->eventMask += eventValue;
}

void GameScript::flashMessage(std::string &txt, float time, float charHeight)
{
	if(mefl) mefl->flashMessage(txt, time, charHeight);
}

void GameScript::setDirectionArrow(std::string &text, Ogre::Vector3 vec)
{
	if(mefl) mefl->setDirectionArrow(const_cast<char*>(text.c_str()), Ogre::Vector3(vec.x, vec.y, vec.z));
}

int GameScript::getChatFontSize()
{
	return NETCHAT.getFontSize();
}

void GameScript::setChatFontSize(int size)
{
	NETCHAT.setFontSize(size);
}

void GameScript::showChooser(string &type, string &instance, string &box)
{
	int ntype=-1;
	if (type == "vehicle")   ntype = GUI_Loader::LT_Vehicle;
	if (type == "truck")     ntype = GUI_Loader::LT_Truck;
	if (type == "car")       ntype = GUI_Loader::LT_Truck;
	if (type == "boat")      ntype = GUI_Loader::LT_Boat;
	if (type == "airplane")  ntype = GUI_Loader::LT_Airplane;
	if (type == "heli")      ntype = GUI_Loader::LT_Heli;
	if (type == "trailer")   ntype = GUI_Loader::LT_Trailer;
	if (type == "load")      ntype = GUI_Loader::LT_Load;
	if (type == "extension") ntype = GUI_Loader::LT_Extension;
	if (ntype!=-1)
		mefl->showLoad(ntype, const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()));
}

void GameScript::repairVehicle(string &instance, string &box)
{
	mefl->repairTruck(const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()));
}


void GameScript::spawnObject(const std::string &objectName, const std::string &instanceName, float px, float py, float pz, float rx, float ry, float rz, const std::string &eventhandler, bool uniquifyMaterials)
{
	asIScriptModule *mod=0;
	try
	{
		mod = mse->getEngine()->GetModule("terrainScript", asGM_ONLY_IF_EXISTS);
	}catch(...)
	{
		return;
	}
	if(!mod) return;
	int functionPtr = mod->GetFunctionIdByName(eventhandler.c_str());

	// trying to create the new object
	SceneNode *bakeNode=mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode();
	mefl->loadObject(const_cast<char*>(objectName.c_str()), px, py, pz, rx, ry, rz, bakeNode, const_cast<char*>(instanceName.c_str()), true, functionPtr, const_cast<char*>(objectName.c_str()), uniquifyMaterials);
}

void GameScript::hideDirectionArrow()
{
	if(mefl) mefl->setDirectionArrow(0, Ogre::Vector3::ZERO);
}

int GameScript::setMaterialAmbient(const std::string &materialName, float red, float green, float blue)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setAmbient(red, green, blue);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

int GameScript::setMaterialDiffuse(const std::string &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setDiffuse(red, green, blue, alpha);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

int GameScript::setMaterialSpecular(const std::string &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setSpecular(red, green, blue, alpha);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

int GameScript::setMaterialEmissive(const std::string &materialName, float red, float green, float blue)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setSelfIllumination(red, green, blue);
	} catch(...)
	{
		return 0;
	}
	return 1;
}

float GameScript::rangeRandom(float from, float to)
{
	return Ogre::Math::RangeRandom(from, to);
}

#endif //ANGELSCRIPT

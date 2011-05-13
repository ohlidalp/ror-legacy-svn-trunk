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
// created on 24th of February 2009 by Thomas Fischer
#ifdef USE_ANGELSCRIPT

#include "ScriptEngine.h"
#include "Ogre.h"
#include "RoRFrameListener.h"
#include "BeamFactory.h"

// AS addons start
#include "scriptstdstring/scriptstdstring.h"
#include "scriptmath3d/scriptmath3d.h"
#include "scriptmath/scriptmath.h"
#include "contextmgr/contextmgr.h"
#include "scriptany/scriptany.h"
#include "scriptarray/scriptarray.h"
#include "scripthelper/scripthelper.h"
#include "scriptstring/scriptstring.h"
// AS addons end

#ifdef USE_CURL
#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#endif //USE_CURL

#include "rornet.h"
#include "water.h"
#include "Beam.h"
#include "Settings.h"
#include "Console.h"
#include "OverlayWrapper.h"
#include "SkyManager.h"
#include "CacheSystem.h"
#include "as_ogre.h"
#include "SelectorWindow.h"
#include "sha1.h"
#include "collisions.h"

//using namespace Ogre;
//using namespace std;
//using namespace AngelScript;

template<> ScriptEngine *Ogre::Singleton<ScriptEngine>::ms_Singleton=0;

char *ScriptEngine::moduleName = "RoRScript";

#define SLOG(x) ScriptEngine::getSingleton().scriptLog->logMessage(x);

// the class implementation

ScriptEngine::ScriptEngine(RoRFrameListener *efl, Collisions *_coll) : mefl(efl), coll(_coll), engine(0), context(0), frameStepFunctionPtr(-1), wheelEventFunctionPtr(-1), eventMask(0), terrainScriptName(), terrainScriptHash(), scriptLog(0)
{
	callbacks["on_terrain_loading"] = std::vector<int>();
	callbacks["frameStep"] = std::vector<int>();
	callbacks["wheelEvents"] = std::vector<int>();
	callbacks["eventCallback"] = std::vector<int>();

	frameStepFunctionPtr = 0;
	wheelEventFunctionPtr = 0;
	eventCallbackFunctionPtr = 0;
	defaultEventCallbackFunctionPtr = 0;

	// create our own log
	scriptLog = LogManager::getSingleton().createLog(SSETTING("Log Path")+"/Angelscript.log", false);
	scriptLog->addListener(this);
	scriptLog->logMessage("ScriptEngine initialized");

	// init not earlier, otherwise crash
	init();
}

ScriptEngine::~ScriptEngine()
{
	// Clean up
	if(engine)  engine->Release();
	if(context) context->Release();
}

void ScriptEngine::messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName )
{
	Console *c = Console::getInstancePtrNoCreation();
	if(c) c->printUTF(message, "Angelscript");
}

void ScriptEngine::ExceptionCallback(AngelScript::asIScriptContext *ctx, void *param)
{
	AngelScript::asIScriptEngine *engine = ctx->GetEngine();
	int funcID = ctx->GetExceptionFunction();
	const AngelScript::asIScriptFunction *function = engine->GetFunctionDescriptorById(funcID);
	SLOG("--- exception ---");
	SLOG("desc: " + String(ctx->GetExceptionString()));
	SLOG("func: " + String(function->GetDeclaration()));
	SLOG("modl: " + String(function->GetModuleName()));
	SLOG("sect: " + String(function->GetScriptSectionName()));
	int col, line = ctx->GetExceptionLineNumber(&col);
	SLOG("line: "+TOSTRING(line)+","+TOSTRING(col));

	// Print the variables in the current function
	//PrintVariables(ctx, -1);

	// Show the call stack with the variables
	SLOG("--- call stack ---");
	char tmp[2048]="";
    for( AngelScript::asUINT n = 1; n < ctx->GetCallstackSize(); n++ )
    {
    	function = ctx->GetFunction(n);
		sprintf(tmp, "%s (%d): %s\n", function->GetScriptSectionName(), ctx->GetLineNumber(n), function->GetDeclaration());
		SLOG(String(tmp));
		//PrintVariables(ctx, n);
    }
}

void ScriptEngine::exploreScripts()
{
	// this shouldnt be used atm
	return;
#if USE_ANGELSCRIPT
	FileInfoListPtr files= ResourceGroupManager::getSingleton().findResourceFileInfo("Scripts", "*.rs", false);
	for (FileInfoList::iterator iterFiles = files->begin(); iterFiles!= files->end(); ++iterFiles)
	{
		loadScript(iterFiles->filename);
	}
#endif //USE_ANGELSCRIPT
}

void ScriptEngine::LineCallback(AngelScript::asIScriptContext *ctx, unsigned long *timeOut)
{
	// If the time out is reached we abort the script
	if(OgreFramework::getSingleton().getTimeSinceStartup() > *timeOut)
		ctx->Abort();

	// It would also be possible to only suspend the script,
	// instead of aborting it. That would allow the application
	// to resume the execution where it left of at a later 
	// time, by simply calling Execute() again.
}

/*
void ScriptEngine::PrintVariables(asIScriptContext *ctx, int stackLevel)
{
	char tmp[1024]="";
	asIScriptEngine *engine = ctx->GetEngine();

	int typeId = ctx->GetThisTypeId(stackLevel);
	void *varPointer = ctx->GetThisPointer(stackLevel);
	if( typeId )
	{
		sprintf(tmp," this = %p", varPointer);
		SLOG(tmp);
	}

	int numVars = ctx->GetVarCount(stackLevel);
	for( int n = 0; n < numVars; n++ )
	{
		int typeId = ctx->GetVarTypeId(n, stackLevel);
		void *varPointer = ctx->GetAddressOfVar(n, stackLevel);
		if( typeId == engine->GetTypeIdByDecl("int") )
		{
			sprintf(tmp, " %s = %d", ctx->GetVarDeclaration(n, stackLevel), *(int*)varPointer);
			SLOG(tmp);
		}
		else if( typeId == engine->GetTypeIdByDecl("string") )
		{
			std::string *str = (std::string*)varPointer;
			if( str )
			{
				sprintf(tmp, " %s = '%s'", ctx->GetVarDeclaration(n, stackLevel), str->c_str());
				SLOG(tmp);
			} else
			{
				sprintf(tmp, " %s = <null>", ctx->GetVarDeclaration(n, stackLevel));
				SLOG(tmp);
			}
		SLOG(tmp);
		}
	}
};
*/

// continue with initializing everything
void ScriptEngine::init()
{
	SLOG("ScriptEngine (SE) initializing ...");
	int result;
	// Create the script engine
	engine = AngelScript::asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Set the message callback to receive information on errors in human readable form.
	// It's recommended to do this right after the creation of the engine, because if
	// some registration fails the engine may send valuable information to the message
	// stream.
	result = engine->SetMessageCallback(AngelScript::asMETHOD(ScriptEngine,msgCallback), this, AngelScript::asCALL_THISCALL);
	if(result < 0)
	{
		if(result == AngelScript::asINVALID_ARG)
		{
			SLOG("One of the arguments is incorrect, e.g. obj is null for a class method.");
			return;
		} else if(result == AngelScript::asNOT_SUPPORTED)
		{
			SLOG("	The arguments are not supported, e.g. asCALL_GENERIC.");
			return;
		}
		SLOG("Unkown error while setting up message callback");
		return;
	}

	// AngelScript doesn't have a built-in string type, as there is no definite standard
	// string type for C++ applications. Every developer is free to register it's own string type.
	// The SDK do however provide a standard add-on for registering a string type, so it's not
	// necessary to register your own string type if you don't want to.
	AngelScript::RegisterScriptArray(engine, true);
	AngelScript::RegisterStdString(engine);
	AngelScript::RegisterScriptMath(engine);
	AngelScript::RegisterScriptMath3D(engine);
	AngelScript::RegisterScriptAny(engine);
	AngelScript::RegisterScriptDictionary(engine);
	//AngelScript::RegisterScriptString(engine);
	//AngelScript::RegisterScriptStringUtils(engine);

	registerOgreObjects(engine);

	// Register everything
	// class Beam
	result = engine->RegisterObjectType("BeamClass", sizeof(Beam), AngelScript::asOBJ_REF); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", AngelScript::asMETHOD(Beam,scaleTruck), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "string getTruckName()", AngelScript::asMETHOD(Beam,getTruckName), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void reset(bool)", AngelScript::asMETHOD(Beam,reset), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setDetailLevel(int)", AngelScript::asMETHOD(Beam,setDetailLevel), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void showSkeleton(bool, bool)", AngelScript::asMETHOD(Beam,showSkeleton), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void hideSkeleton(bool)", AngelScript::asMETHOD(Beam,hideSkeleton), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", AngelScript::asMETHOD(Beam,parkingbrakeToggle), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void tractioncontrolToggle()", AngelScript::asMETHOD(Beam,tractioncontrolToggle), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void antilockbrakeToggle()", AngelScript::asMETHOD(Beam,antilockbrakeToggle), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void beaconsToggle()", AngelScript::asMETHOD(Beam,beaconsToggle), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setReplayMode(bool)", AngelScript::asMETHOD(Beam,setReplayMode), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void resetAutopilot()", AngelScript::asMETHOD(Beam,resetAutopilot), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void toggleCustomParticles()", AngelScript::asMETHOD(Beam,toggleCustomParticles), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", AngelScript::asMETHOD(Beam,parkingbrakeToggle), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getDefaultDeformation()", AngelScript::asMETHOD(Beam,getDefaultDeformation), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getNodeCount()", AngelScript::asMETHOD(Beam,getNodeCount), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getTotalMass(bool)", AngelScript::asMETHOD(Beam,getTotalMass), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getWheelNodeCount()", AngelScript::asMETHOD(Beam,getWheelNodeCount), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void recalc_masses()", AngelScript::asMETHOD(Beam,recalc_masses), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setMass(float)", AngelScript::asMETHOD(Beam,setMass), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getBrakeLightVisible()", AngelScript::asMETHOD(Beam,getBrakeLightVisible), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomLightVisible(int)", AngelScript::asMETHOD(Beam,getCustomLightVisible), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setCustomLightVisible(int, bool)", AngelScript::asMETHOD(Beam,setCustomLightVisible), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getBeaconMode()", AngelScript::asMETHOD(Beam,getBeaconMode), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setBlinkType(int)", AngelScript::asMETHOD(Beam,setBlinkType), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getBlinkType()", AngelScript::asMETHOD(Beam,getBlinkType), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", AngelScript::asMETHOD(Beam,getCustomParticleMode), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getLowestNode()", AngelScript::asMETHOD(Beam,getLowestNode), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool setMeshVisibility(bool)", AngelScript::asMETHOD(Beam,setMeshVisibility), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", AngelScript::asMETHOD(Beam,getCustomParticleMode), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", AngelScript::asMETHOD(Beam,getCustomParticleMode), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getReverseLightVisible()", AngelScript::asMETHOD(Beam,getCustomParticleMode), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getHeadingDirectionAngle()", AngelScript::asMETHOD(Beam,getHeadingDirectionAngle), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool isLocked()", AngelScript::asMETHOD(Beam,isLocked), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getWheelSpeed()", AngelScript::asMETHOD(Beam,getWheelSpeed), AngelScript::asCALL_THISCALL); assert(result>=0);

	
	/*
	// impossible to use offsetof for derived classes
	// unusable, read http://www.angelcode.com/angelscript/sdk/docs/manual/doc_adv_class_hierarchy.html

	result = engine->RegisterObjectProperty("BeamClass", "float WheelSpeed", offsetof(Beam, WheelSpeed)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float brake", offsetof(Beam, brake)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float currentScale", offsetof(Beam, currentScale)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int nodedebugstate", offsetof(Beam, nodedebugstate)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int debugVisuals", offsetof(Beam, debugVisuals)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool networking", offsetof(Beam, networking)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int label", offsetof(Beam, label)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int trucknum", offsetof(Beam, trucknum)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int skeleton", offsetof(Beam, skeleton)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool replaymode", offsetof(Beam, replaymode)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int replaylen", offsetof(Beam, replaylen)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int replaypos", offsetof(Beam, replaypos)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool cparticle_enabled", offsetof(Beam, cparticle_enabled)); assert(result>=0);
	//result = engine->RegisterObjectProperty("BeamClass", "int hookId", offsetof(Beam, hookId)); assert(result>=0);
	//result = engine->RegisterObjectProperty("BeamClass", "BeamClass @lockTruck", offsetof(Beam, lockTruck)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_node", offsetof(Beam, free_node)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int dynamicMapMode", offsetof(Beam, dynamicMapMode)); assert(result>=0);
	//result = engine->RegisterObjectProperty("BeamClass", "int tied", offsetof(Beam, tied)); assert(result>=0);
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
	result = engine->RegisterObjectProperty("BeamClass", "int antilockbrake", offsetof(Beam, antilockbrake)); assert(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int tractioncontrol", offsetof(Beam, tractioncontrol)); assert(result>=0);
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
	*/

	result = engine->RegisterObjectBehaviour("BeamClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(Beam,addRef), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectBehaviour("BeamClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(Beam,release), AngelScript::asCALL_THISCALL); assert(result>=0);

	// class Settings
	result = engine->RegisterObjectType("SettingsClass", sizeof(Settings), AngelScript::asOBJ_REF); assert(result>=0);
	result = engine->RegisterObjectMethod("SettingsClass", "string getSetting(const string &in)", AngelScript::asMETHOD(Settings,getSettingScriptSafe), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectBehaviour("SettingsClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(Settings,addRef), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectBehaviour("SettingsClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(Settings,release), AngelScript::asCALL_THISCALL); assert(result>=0);

	// TODO: add Vector3 classes and other utility classes!

	// class GameScript
	result = engine->RegisterObjectType("GameScriptClass", sizeof(GameScript), AngelScript::asOBJ_VALUE | AngelScript::asOBJ_POD | AngelScript::asOBJ_APP_CLASS); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void log(const string &in)", AngelScript::asMETHOD(GameScript,log), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "double getTime()", AngelScript::asMETHOD(GameScript,getTime), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonPosition(vector3)", AngelScript::asMETHOD(GameScript,setPersonPosition), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void loadTerrain(const string &in)", AngelScript::asMETHOD(GameScript,loadTerrain), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getPersonPosition()", AngelScript::asMETHOD(GameScript,getPersonPosition), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void movePerson(float, float, float)", AngelScript::asMETHOD(GameScript,movePerson), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "string getCaelumTime()", AngelScript::asMETHOD(GameScript,getCaelumTime), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setCaelumTime(float)", AngelScript::asMETHOD(GameScript,setCaelumTime), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setWaterHeight(float)", AngelScript::asMETHOD(GameScript,setWaterHeight), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getWaterHeight()", AngelScript::asMETHOD(GameScript,getWaterHeight), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getGroundHeight(vector3)", AngelScript::asMETHOD(GameScript,getGroundHeight), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getCurrentTruckNumber()", AngelScript::asMETHOD(GameScript,getCurrentTruckNumber), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucks()", AngelScript::asMETHOD(GameScript,getNumTrucks), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getGravity()", AngelScript::asMETHOD(GameScript,getGravity), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setGravity(float)", AngelScript::asMETHOD(GameScript,setGravity), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void flashMessage(const string &in, float, float)", AngelScript::asMETHOD(GameScript,flashMessage), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setDirectionArrow(const string &in, vector3)", AngelScript::asMETHOD(GameScript,setDirectionArrow), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void hideDirectionArrow()", AngelScript::asMETHOD(GameScript,hideDirectionArrow), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void registerForEvent(int)", AngelScript::asMETHOD(GameScript,registerForEvent), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getCurrentTruck()", AngelScript::asMETHOD(GameScript,getCurrentTruck), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getTruckByNum(int)", AngelScript::asMETHOD(GameScript,getTruckByNum), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getChatFontSize()", AngelScript::asMETHOD(GameScript,getChatFontSize), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setChatFontSize(int)", AngelScript::asMETHOD(GameScript,setChatFontSize), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void showChooser(const string &in, const string &in, const string &in)", AngelScript::asMETHOD(GameScript,showChooser), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void repairVehicle(const string &in, const string &in, bool)", AngelScript::asMETHOD(GameScript,repairVehicle), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void removeVehicle(const string &in, const string &in)", AngelScript::asMETHOD(GameScript,removeVehicle), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void spawnObject(const string &in, const string &in, vector3, vector3, const string &in, bool)", AngelScript::asMETHOD(GameScript,spawnObject), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void destroyObject(const string &in)", AngelScript::asMETHOD(GameScript,destroyObject), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialAmbient(const string &in, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialAmbient), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialDiffuse(const string &in, float, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialDiffuse), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialSpecular(const string &in, float, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialSpecular), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialEmissive(const string &in, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialEmissive), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucksByFlag(int)", AngelScript::asMETHOD(GameScript,getNumTrucksByFlag), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "bool getCaelumAvailable()", AngelScript::asMETHOD(GameScript,getCaelumAvailable), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void startTimer()", AngelScript::asMETHOD(GameScript,startTimer), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void stopTimer()", AngelScript::asMETHOD(GameScript,stopTimer), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float rangeRandom(float, float)", AngelScript::asMETHOD(GameScript,rangeRandom), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int useOnlineAPI(const string &in, const dictionary &in, string &out)", AngelScript::asMETHOD(GameScript,useOnlineAPI), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getLoadedTerrain(string &out)", AngelScript::asMETHOD(GameScript,getLoadedTerrain), AngelScript::asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void clearEventCache()", AngelScript::asMETHOD(GameScript,clearEventCache), AngelScript::asCALL_THISCALL); assert(result>=0);

	
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
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TRACTIONCONTROL_TOGGLE", SE_TRUCK_TRACTIONCONTROL_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ANTILOCKBRAKE_TOGGLE", SE_TRUCK_ANTILOCKBRAKE_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEACONS_TOGGLE", SE_TRUCK_BEACONS_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_CPARTICLES_TOGGLE", SE_TRUCK_CPARTICLES_TOGGLE); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_GROUND_CONTACT_CHANGED", SE_TRUCK_GROUND_CONTACT_CHANGED); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_NEW_TRUCK", SE_GENERIC_NEW_TRUCK); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_DELETED_TRUCK", SE_GENERIC_DELETED_TRUCK); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_INPUT_EVENT", SE_GENERIC_INPUT_EVENT); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MOUSE_BEAM_INTERACTION", SE_GENERIC_MOUSE_BEAM_INTERACTION); assert(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_ALL_EVENTS", SE_ALL_EVENTS); assert(result>=0);
	

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
	//result = engine->RegisterGlobalProperty("CacheSystemClass cache", &CacheSystem::Instance()); assert(result>=0);
	result = engine->RegisterGlobalProperty("SettingsClass settings", &SETTINGS); assert(result>=0);

	SLOG("Type registrations done. If you see no error above everything should be working");
}

void ScriptEngine::msgCallback(const AngelScript::asSMessageInfo *msg)
{
	const char *type = "Error";
	if( msg->type == AngelScript::asMSGTYPE_INFORMATION )
		type = "Info";
	else if( msg->type == AngelScript::asMSGTYPE_WARNING )
		type = "Warning";

	char tmp[1024]="";
	sprintf(tmp, "%s (%d, %d): %s = %s", msg->section, msg->row, msg->col, type, msg->message);
	SLOG(tmp);
}

int ScriptEngine::framestep(Ogre::Real dt)
{

#if 0
	Beam **trucks = BeamFactory::getSingleton().getTrucks();
	int free_truck = BeamFactory::getSingleton().getTruckCount();
	// check for all truck wheels
	if(coll && wheelEventFunctionPtr >= 0)
	{
		for(int t = 0; t < free_truck; t++)
		{
			Beam *b = trucks[t];
			if (!b || b->state == SLEEPING || b->state == NETWORKED || b->state == RECYCLE)
				continue;

			bool allwheels=true;
			int handlerid=-1;
			for(int w = 0; w < b->free_wheel; w++)
			{
				if(handlerid == -1 && b->wheels[w].lastEventHandler != -1)
				{
					handlerid = b->wheels[w].lastEventHandler;
					continue;
				}else if(b->wheels[w].lastEventHandler != handlerid)
				{
					allwheels=false;
					break;
				}
			}

			// if the truck is in there, call the functions
			if(allwheels && handlerid >= 0 && handlerid < MAX_EVENTSOURCE)
			{
				eventsource_t *source = coll->getEvent(handlerid);
				if(!engine) return 0;
				if(!context) context = engine->CreateContext();
				context->Prepare(wheelEventFunctionPtr);

				// Set the function arguments
				context->SetArgFloat (0, t);
				context->SetArgObject(1, &std::string("wheels"));
				context->SetArgObject(2, &std::string(source->instancename));
				context->SetArgObject(3, &std::string(source->boxname));

				//SLOG("Executing framestep()");
				int r = context->Execute();
				if( r == AngelScript::asEXECUTION_FINISHED )
				{
				  // The return value is only valid if the execution finished successfully
					AngelScript::asDWORD ret = context->GetReturnDWord();
				}
			}
		}
	}
#endif // 0

	// check if current truck is in an event box
	Beam *truck = BeamFactory::getSingleton().getCurrentTruck();
	if(truck)
	{
		eventsource_t *source = coll->isTruckInEventBox(truck);
		if(source) envokeCallback(source->scripthandler, source, 0, 1);
	}

	// framestep stuff below
	if(frameStepFunctionPtr<0) return 1;
	if(!engine) return 0;
	if(!context) context = engine->CreateContext();
	context->Prepare(frameStepFunctionPtr);

	// Set the function arguments
	context->SetArgFloat(0, dt);

	//SLOG("Executing framestep()");
	int r = context->Execute();
	if( r == AngelScript::asEXECUTION_FINISHED )
	{
	  // The return value is only valid if the execution finished successfully
		AngelScript::asDWORD ret = context->GetReturnDWord();
	}
	return 0;
}


int ScriptEngine::envokeCallback(int functionPtr, eventsource_t *source, node_t *node, int type)
{
	if(!engine) return 0;
	if(functionPtr <= 0 && defaultEventCallbackFunctionPtr > 0)
	{
		// use the default event handler instead then
		functionPtr = defaultEventCallbackFunctionPtr;
	} else if (functionPtr <= 0)
	{
		// no default callback available, discard the event
		return 0;
	}
	if(!context) context = engine->CreateContext();
	context->Prepare(functionPtr);

	// Set the function arguments
	context->SetArgDWord (0, type);
	context->SetArgObject(1, &std::string(source->instancename));
	context->SetArgObject(2, &std::string(source->boxname));
	if(node)
		context->SetArgDWord (3, node->id);
	else
		context->SetArgDWord (3, -1);

	int r = context->Execute();
	if( r == AngelScript::asEXECUTION_FINISHED )
	{
	  // The return value is only valid if the execution finished successfully
		AngelScript::asDWORD ret = context->GetReturnDWord();
	}
	return 0;
}

int ScriptEngine::executeString(Ogre::String command)
{
	if(!engine) return 1;
	if(!context) context = engine->CreateContext();
	AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_CREATE_IF_NOT_EXISTS);
	int result = ExecuteString(engine, command.c_str(), mod, context);
	if(result < 0)
	{
		SLOG("error " + TOSTRING(result) + " while executing string: " + command + ".");
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
		if( r == AngelScript::asEXECUTION_FINISHED )
		{
		  // The return value is only valid if the execution finished successfully
			AngelScript::asDWORD ret = context->GetReturnDWord();
		}
		return;
	}
}

int ScriptEngine::loadScript(Ogre::String scriptname)
{
	// Load the entire script file into the buffer
	int result=0;

	// The builder is a helper class that will load the script file, 
	// search for #include directives, and load any included files as 
	// well.
	OgreScriptBuilder builder;

	AngelScript::asIScriptModule *mod = 0;
	// try to load bytecode
	bool cached = false;
	{
		// the code below should load a compilation result but it crashes for some reason atm ...
		//AngelScript::asIScriptModule *mod = engine->GetModule("RoRScript", AngelScript::asGM_ALWAYS_CREATE);
		/*
		String fn = SSETTING("Cache Path") + "script" + hash + "_" + scriptname + "c";
		CBytecodeStream bstream(fn);
		if(bstream.Existing())
		{
			// CRASHES here :(
			int res = mod->LoadByteCode(&bstream);
			cached = !res;
		}
		*/
	}
	if(!cached)
	{
		// not cached so dynamically load and compile it
		result = builder.StartNewModule(engine, moduleName);
		if( result < 0 )
		{
			SLOG("Failed to start new module");
			return result;
		}

		mod = engine->GetModule(moduleName, AngelScript::asGM_ONLY_IF_EXISTS);

		result = builder.AddSectionFromFile(scriptname.c_str());
		if( result < 0 )
		{
			SLOG("Unkown error while loading script file: "+scriptname);
			SLOG("Failed to add script file");
			return result;
		}
		result = builder.BuildModule();
		if( result < 0 )
		{
			SLOG("Failed to build the module");
			return result;
		}

		// save the bytecode
		string hash = "";
		// TODO: fix hash!
		{
			String fn = SSETTING("Cache Path") + "script" + hash + "_" + scriptname + "c";
			SLOG("saving script bytecode to file " + fn);
			CBytecodeStream bstream(fn);
			mod->SaveByteCode(&bstream);
		}
	}

	// get some other optional functions
	frameStepFunctionPtr = mod->GetFunctionIdByDecl("void frameStep(float)");
	if(frameStepFunctionPtr > 0) callbacks["frameStep"].push_back(frameStepFunctionPtr);
	
	wheelEventFunctionPtr = mod->GetFunctionIdByDecl("void wheelEvents(int, string, string, string)");
	if(wheelEventFunctionPtr > 0) callbacks["wheelEvents"].push_back(wheelEventFunctionPtr);

	eventCallbackFunctionPtr = mod->GetFunctionIdByDecl("void eventCallback(int, int)");
	if(eventCallbackFunctionPtr > 0) callbacks["eventCallback"].push_back(eventCallbackFunctionPtr);

	defaultEventCallbackFunctionPtr = mod->GetFunctionIdByDecl("void defaultEventCallback(int, string, string, int)");
	if(defaultEventCallbackFunctionPtr > 0) callbacks["defaultEventCallback"].push_back(defaultEventCallbackFunctionPtr);

	int cb = mod->GetFunctionIdByDecl("void on_terrain_loading(string lines)");
	if(cb > 0) callbacks["on_terrain_loading"].push_back(cb);

	// Find the function that is to be called.
	int funcId = mod->GetFunctionIdByDecl("void main()");
	if( funcId < 0 )
	{
		// The function couldn't be found. Instruct the script writer to include the
		// expected function in the script.
		SLOG("The script should have the function 'void main()'.");
		return 0;
	}

	// Create our context, prepare it, and then execute
	context = engine->CreateContext();


	unsigned long timeOut = 0;
	/*
	// TOFIX: AS crashes badly when using these :-\
	result = context->SetLineCallback(AngelScript::asMETHOD(ScriptEngine, LineCallback), &timeOut, AngelScript::asCALL_THISCALL);
	if(result < 0)
	{
		SLOG("Failed to set the line callback function.");
		context->Release();
		return -1;
	}

	result = context->SetExceptionCallback(AngelScript::asMETHOD(ScriptEngine,ExceptionCallback), this, AngelScript::asCALL_THISCALL);
	if(result < 0)
	{
		SLOG("Failed to set the exception callback function.");
		context->Release();
		return -1;
	}
	*/

	// Prepare the script context with the function we wish to execute. Prepare()
	// must be called on the context before each new script function that will be
	// executed. Note, that if you intend to execute the same function several 
	// times, it might be a good idea to store the function id returned by 
	// GetFunctionIDByDecl(), so that this relatively slow call can be skipped.
	result = context->Prepare(funcId);
	if(result < 0)
	{
		SLOG("Failed to prepare the context.");
		context->Release();
		return -1;
	}

	// Set the timeout before executing the function. Give the function 1 sec
	// to return before we'll abort it.
	timeOut = OgreFramework::getSingleton().getTimeSinceStartup() + 1000;

	SLOG("Executing main()");
	result = context->Execute();
	if( result != AngelScript::asEXECUTION_FINISHED )
	{
		// The execution didn't complete as expected. Determine what happened.
		if( result == AngelScript::asEXECUTION_ABORTED )
		{
			SLOG("The script was aborted before it could finish. Probably it timed out.");
		}
		else if( result == AngelScript::asEXECUTION_EXCEPTION )
		{
			// An exception occurred, let the script writer know what happened so it can be corrected.
			SLOG("An exception '" + String(context->GetExceptionString()) + "' occurred. Please correct the code in file '" + scriptname + "' and try again.");

			// Write some information about the script exception
			int funcID = context->GetExceptionFunction();
			AngelScript::asIScriptFunction *func = engine->GetFunctionDescriptorById(funcID);
			SLOG("func: " + String(func->GetDeclaration()));
			SLOG("modl: " + String(func->GetModuleName()));
			SLOG("sect: " + String(func->GetScriptSectionName()));
			SLOG("line: " + TOSTRING(context->GetExceptionLineNumber()));
			SLOG("desc: " + String(context->GetExceptionString()));
		} else
		{
			SLOG("The script ended for some unforeseen reason " + TOSTRING(result));
		}
	} else
	{
		SLOG("The script finished successfully.");
	}

	return 0;
}

/* class that implements the interface for the scripts */
GameScript::GameScript(ScriptEngine *se, RoRFrameListener *efl) : mse(se), mefl(efl)
{
}

GameScript::~GameScript()
{
}

void GameScript::log(std::string &msg)
{
	SLOG(msg);
}

double GameScript::getTime()
{
	return this->mefl->getTime();
}

void GameScript::setPersonPosition(Ogre::Vector3 vec)
{
	if(mefl && mefl->person) mefl->person->setPosition(Ogre::Vector3(vec.x, vec.y, vec.z));
}

void GameScript::loadTerrain(std::string &terrain)
{
	if(mefl) mefl->loadTerrain(terrain);
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

std::string GameScript::getCaelumTime()
{
#ifdef USE_CAELUM
	return SkyManager::getSingleton().getPrettyTime();
#else 
	return "";
#endif // USE_CAELUM
}

void GameScript::setCaelumTime(float value)
{
#ifdef USE_CAELUM
	SkyManager::getSingleton().setTimeFactor(value);
#endif // USE_CAELUM
}

bool GameScript::getCaelumAvailable()
{
#ifdef USE_CAELUM
	return SkyManager::getSingletonPtr() != 0;
#endif // USE_CAELUM
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

float GameScript::getGroundHeight(Ogre::Vector3 v)
{
	if(RoRFrameListener::hfinder)
		return RoRFrameListener::hfinder->getHeightAt(v.x, v.z);
	return -1;
}

float GameScript::getWaterHeight()
{
	if(mefl && mefl->w) return mefl->w->getHeight();
	return 0;
}

Beam *GameScript::getCurrentTruck()
{
	return BeamFactory::getSingleton().getCurrentTruck();
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
	return BeamFactory::getSingleton().getTruck(num);
}

int GameScript::getNumTrucks()
{
	return BeamFactory::getSingleton().getTruckCount();
}

int GameScript::getNumTrucksByFlag(int flag)
{
	int res = 0;
	for(int i=0; i< BeamFactory::getSingleton().getTruckCount(); i++)
	{
		Beam *truck = BeamFactory::getSingleton().getTruck(i);
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
	if(mefl) return BeamFactory::getSingleton().getCurrentTruckNumber();
	return -1;
}

void GameScript::registerForEvent(int eventValue)
{
	if(!mse) return;
	mse->eventMask += eventValue;
}

void GameScript::flashMessage(std::string &txt, float time, float charHeight)
{
	if(mefl && mefl->getOverlayWrapper())
		mefl->getOverlayWrapper()->flashMessage(txt, time, charHeight);
}

void GameScript::setDirectionArrow(std::string &text, Ogre::Vector3 vec)
{
	if(mefl) mefl->setDirectionArrow(const_cast<char*>(text.c_str()), Ogre::Vector3(vec.x, vec.y, vec.z));
}

int GameScript::getChatFontSize()
{
	return 0; //NETCHAT.getFontSize();
}

void GameScript::setChatFontSize(int size)
{
	//NETCHAT.setFontSize(size);
}

void GameScript::showChooser(string &type, string &instance, string &box)
{
#ifdef USE_MYGUI
	SelectorWindow::LoaderType ntype = SelectorWindow::LT_None;
	if (type == "vehicle")   ntype = SelectorWindow::LT_Vehicle;
	if (type == "truck")     ntype = SelectorWindow::LT_Truck;
	if (type == "car")       ntype = SelectorWindow::LT_Truck;
	if (type == "boat")      ntype = SelectorWindow::LT_Boat;
	if (type == "airplane")  ntype = SelectorWindow::LT_Airplane;
	if (type == "heli")      ntype = SelectorWindow::LT_Heli;
	if (type == "trailer")   ntype = SelectorWindow::LT_Trailer;
	if (type == "load")      ntype = SelectorWindow::LT_Load;
	if (type == "extension") ntype = SelectorWindow::LT_Extension;
	if (ntype != SelectorWindow::LT_None)
		mefl->showLoad(ntype, const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()));
#endif //USE_MYGUI
}

void GameScript::repairVehicle(string &instance, string &box, bool keepPosition)
{
	BeamFactory::getSingleton().repairTruck(mefl->getSSM(), mefl->getCollisions(), const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()), keepPosition);
}

void GameScript::removeVehicle(string &instance, string &box)
{
	BeamFactory::getSingleton().removeTruck(mefl->getCollisions(), const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()));
}


void GameScript::destroyObject(const std::string &instanceName)
{
	mefl->unloadObject(const_cast<char*>(instanceName.c_str()));
}

void GameScript::spawnObject(const std::string &objectName, const std::string &instanceName, Ogre::Vector3 pos, Ogre::Vector3 rot, const std::string &eventhandler, bool uniquifyMaterials)
{
	AngelScript::asIScriptModule *mod=0;
	try
	{
		mod = mse->getEngine()->GetModule(mse->moduleName, AngelScript::asGM_ONLY_IF_EXISTS);
	}catch(std::exception e)
	{
		SLOG("Exception in spawnObject(): " + String(e.what()));
		return;
	}
	if(!mod) return;
	int functionPtr = mod->GetFunctionIdByName(eventhandler.c_str());

	// trying to create the new object
	SceneNode *bakeNode=mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode();
	mefl->loadObject(const_cast<char*>(objectName.c_str()), pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, bakeNode, const_cast<char*>(instanceName.c_str()), true, functionPtr, const_cast<char*>(objectName.c_str()), uniquifyMaterials);
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
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialAmbient(): " + e.getFullDescription());
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
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialDiffuse(): " + e.getFullDescription());
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
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialSpecular(): " + e.getFullDescription());
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
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialEmissive(): " + e.getFullDescription());
		return 0;
	}
	return 1;
}

float GameScript::rangeRandom(float from, float to)
{
	return Ogre::Math::RangeRandom(from, to);
}

int GameScript::getLoadedTerrain(std::string &result)
{
	result = mefl->loadedTerrain;
	return 0;
}

void GameScript::clearEventCache()
{
	mefl->getCollisions()->clearEventCache();
}

#ifdef USE_CURL
//hacky hack to fill memory with data for curl
// from: http://curl.haxx.se/libcurl/c/getinmemory.html
static size_t curlWriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct curlMemoryStruct *mem = (struct curlMemoryStruct *)data;

  mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }

  memcpy(&(mem->memory[mem->size]), ptr, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}
#endif //USE_CURL

int GameScript::useOnlineAPI(const std::string &apiquery, const AngelScript::CScriptDictionary &d, std::string &result)
{
#ifdef USE_CURL
	struct curlMemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	// construct post fields
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	curl_global_init(CURL_GLOBAL_ALL);

	std::map<std::string, AngelScript::CScriptDictionary::valueStruct>::const_iterator it;
	for(it = d.dict.begin(); it != d.dict.end(); it++)
	{
		int typeId = it->second.typeId;
		if(typeId == mse->getEngine()->GetTypeIdByDecl("string"))
		{
			// its a string
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, ((std::string *)it->second.valueObj)->c_str(), CURLFORM_END);
		}
		else if(typeId == AngelScript::asTYPEID_INT8 \
			|| typeId == AngelScript::asTYPEID_INT16 \
			|| typeId == AngelScript::asTYPEID_INT32 \
			|| typeId == AngelScript::asTYPEID_INT64)
		{
			// its an integer
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((int)it->second.valueInt).c_str(), CURLFORM_END);
		}
		else if(typeId == AngelScript::asTYPEID_UINT8 \
			|| typeId == AngelScript::asTYPEID_UINT16 \
			|| typeId == AngelScript::asTYPEID_UINT32 \
			|| typeId == AngelScript::asTYPEID_UINT64)
		{
			// its an unsigned integer
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((unsigned int)it->second.valueInt).c_str(), CURLFORM_END);
		}
		else if(typeId == AngelScript::asTYPEID_FLOAT || typeId == AngelScript::asTYPEID_DOUBLE)
		{
			// its a float or double
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((float)it->second.valueFlt).c_str(), CURLFORM_END);
		}
	}

	// add some hard coded values
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_Name", CURLFORM_COPYCONTENTS, mse->getTerrainName().c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_Hash", CURLFORM_COPYCONTENTS, SSETTING("TerrainHash").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ScriptHash", CURLFORM_COPYCONTENTS, mse->getTerrainScriptHash().c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_NickName", CURLFORM_COPYCONTENTS, SSETTING("Nickname").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Language", CURLFORM_COPYCONTENTS, SSETTING("Language").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Token", CURLFORM_COPYCONTENTS, SSETTING("User Token Hash").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionString", CURLFORM_COPYCONTENTS, ROR_VERSION_STRING, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVN", CURLFORM_COPYCONTENTS, SVN_REVISION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVNID", CURLFORM_COPYCONTENTS, SVN_ID, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_ProtocolVersion", CURLFORM_COPYCONTENTS, RORNET_VERSION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_BinaryHash", CURLFORM_COPYCONTENTS, SSETTING("BinaryHash").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_GUID", CURLFORM_COPYCONTENTS, SSETTING("GUID").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerName", CURLFORM_COPYCONTENTS, SSETTING("Server name").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerPort", CURLFORM_COPYCONTENTS, SSETTING("Server port").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_NetworkEnabled", CURLFORM_COPYCONTENTS, SSETTING("Network enable").c_str(), CURLFORM_END);

	if(BeamFactory::getSingleton().getCurrentTruck())
	{
		Beam *truck = BeamFactory::getSingleton().getCurrentTruck();

		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_Name", CURLFORM_COPYCONTENTS, truck->getTruckName().c_str(), CURLFORM_END);
		//curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_ModHash", CURLFORM_COPYCONTENTS, truck->cacheEntryInfo.hash.c_str(), CURLFORM_END);
		//curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_ModFileName", CURLFORM_COPYCONTENTS, truck->cacheEntryInfo.fname.c_str(), CURLFORM_END);
	}

	const RenderTarget::FrameStats& stats = mefl->getRenderWindow()->getStatistics();
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "AVG_FPS", CURLFORM_COPYCONTENTS, TOSTRING(stats.avgFPS).c_str(), CURLFORM_END);



	CURLcode res;
	CURL *curl = curl_easy_init();
	if(!curl)
	{
		result = "ERROR: failed to init curl";
		return 1;
	}

	char *curl_err_str[CURL_ERROR_SIZE];
	memset(curl_err_str, 0, CURL_ERROR_SIZE);

	string url = "http://" + string(REPO_SERVER) + apiquery;
	curl_easy_setopt(curl, CURLOPT_URL,              url.c_str());

	/* send all data to this function  */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	// set post options
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

	// logging stuff
	//curl_easy_setopt(curl, CURLOPT_STDERR,           LogManager::getsin InstallerLog::getSingleton()->getLogFilePtr());
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,      curl_err_str[0]);

	// http related settings
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,   1); // follow redirects
	curl_easy_setopt(curl, CURLOPT_AUTOREFERER,      1); // set the Referrer: field in requests where it follows a Location: redirect.
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS,        20);
	curl_easy_setopt(curl, CURLOPT_USERAGENT,        "RoR");
	curl_easy_setopt(curl, CURLOPT_FILETIME,         1);

	// TO BE DONE: ADD SSL
	// see: http://curl.haxx.se/libcurl/c/simplessl.html
	// curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	//printf("%lu bytes retrieved\n", (long)chunk.size);

	curl_formfree(formpost);

	if(chunk.memory)
	{
		// convert memory into std::string now
		result = string(chunk.memory);

		// then free
		free(chunk.memory);
	}

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	if(res != CURLE_OK)
	{
		const char *errstr = curl_easy_strerror(res);
		result = "ERROR: " + string(errstr);
		return 1;
	}

	return 0;
#endif //USE_CURL
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// CBytecodeStream
CBytecodeStream::CBytecodeStream(std::string filename) : f(0)
{
	f = fopen(filename.c_str(), "wb");
}

CBytecodeStream::~CBytecodeStream()
{
	if(f)
		fclose(f);
}

void CBytecodeStream::Write(const void *ptr, AngelScript::asUINT size)
{
	if(!f) return;
	fwrite(ptr, size, 1, f);
}

void CBytecodeStream::Read(void *ptr, AngelScript::asUINT size)
{
	if(!f) return;
	fread(ptr, size, 1, f);
}

bool CBytecodeStream::Existing()
{
	return (f != 0);
}


// OgreScriptBuilder
int OgreScriptBuilder::LoadScriptSection(const char *filename)
{
	// Open the script file
	string scriptFile = filename;

	DataStreamPtr ds;
	try
	{
		ds = ResourceGroupManager::getSingleton().openResource(scriptFile, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

	} catch(Ogre::Exception e)
	{
		SLOG("exception upon loading script file: " + e.getFullDescription());
		return -1;
	}

	// Read the entire file
	string code;
	code.resize(ds->size());
	ds->read(&code[0], ds->size());

	// TODO: fix the script hashes
	/*
	// using SHA1 here is stupid, we need to replace it with something better
	// then hash it
	char hash_result[250];
	memset(hash_result, 0, 249);
	RoR::CSHA1 sha1;
	sha1.UpdateHash((uint8_t *)script.c_str(), script.size());
	sha1.Final();
	sha1.ReportHash(hash_result, RoR::CSHA1::REPORT_HEX_SHORT);
	hash = string(hash_result);
	*/

	return ProcessScriptSection(code.c_str(), filename);
}

#endif //ANGELSCRIPT

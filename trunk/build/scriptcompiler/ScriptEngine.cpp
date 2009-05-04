#include "ScriptEngine.h"
#include <assert.h>  // assert()

// interfaces
#include "Beam.h"
#include "GameScript.h"
#include "scriptEvents.h"
#include <algorithm> //std::replace

using namespace std;

ScriptEngine::ScriptEngine() : engine(0), context(0)
{
	init();
}

ScriptEngine::~ScriptEngine()
{
	// Clean up
	engine->Release();
	if(context) context->Release();
}

int ScriptEngine::loadScriptFile(const char *fileName, string &script)
{
	//printf("loading file %s...\n", fileName);
	FILE *f = fopen(fileName, "rb");
	if(!f)
	{
		printf("error: could not open file.\n");
		return -1;
	}

	// Determine the size of the file
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);

	// Load the entire file in one call
	script.resize(len);
	fread(&script[0], len, 1, f);

	fclose(f);

	printf("file loaded: %s. size: %d bytes / %d kB\n", fileName, len, len/1024);
	return 0;
}

int ScriptEngine::loadFileInMemory(std::string scriptname)
{
	FILE *fd = fopen(scriptname.c_str(), "r");;
	if(!fd) return 1;
	char line[4096];
	int i=1;

	while(fgets(line, 4096, fd) != NULL)
	{
		int lpos = strlen(line)-1;
		if(line[lpos] == '\n') line[lpos] = 0;
		fileContents[i] = std::string(line);
		std::replace(fileContents[i].begin(),fileContents[i].end(), '\t',' ');
		//printf("%d| %s\n", i, line);
		i++;
	}

	fclose(fd);
	return 0;
}

int ScriptEngine::loadFile(std::string scriptname)
{
	// Load the entire script file into the buffer
	int result=0;
	string script;
	result = loadScriptFile(scriptname.c_str(), script);
	result &= loadFileInMemory(scriptname.c_str());
	if(result)
		return 1;

	// Add the script to the module as a section. If desired, multiple script
	// sections can be added to the same module. They will then be compiled
	// together as if it was one large script.
	asIScriptModule *mod = engine->GetModule("terrainScript", asGM_ALWAYS_CREATE);
	result = mod->AddScriptSection(scriptname.c_str(), script.c_str(), script.length());
	if( result < 0 )
	{
		printf("Unkown error while adding script section\n");
		return 1;
	}

	// Build the module
	result = mod->Build();
	if( result < 0 )
	{
		if(result == asINVALID_CONFIGURATION)
		{
			printf("The engine configuration is invalid.\n");
			return 1;
		} else if(result == asERROR)
		{
			printf("The script failed to build. \n");
			return 1;
		} else if(result == asBUILD_IN_PROGRESS)
		{
			printf("Another thread is currently building.\n");
			return 1;
		}
		printf("Unkown error while building the script\n");
		return 1;
	}

	// Find the function that is to be called.
	int funcId = mod->GetFunctionIdByDecl("void main()");
	if( funcId < 0 )
	{
		// The function couldn't be found. Instruct the script writer to include the
		// expected function in the script.
		printf("The script must have the function 'void main()'. Please add it and try again.\n");
		return 1;
	}

	// get some other optional functions
	frameStepFunctionPtr = mod->GetFunctionIdByDecl("void frameStep(float)");
	eventCallbackFunctionPtr = mod->GetFunctionIdByDecl("void eventCallback(int event, int value)");

	// Create our context, prepare it, and then execute
	context = engine->CreateContext();

	//context->SetLineCallback(asMETHOD(ScriptEngine,LineCallback), this, asCALL_THISCALL);

	// this does not work :(
	context->SetExceptionCallback(asMETHOD(ScriptEngine,ExceptionCallback), this, asCALL_THISCALL);

	if(funcId >= 0)
	{
		printf("Executing main()\n");
		context->Prepare(funcId);
		result = context->Execute();
		if( result != asEXECUTION_FINISHED )
		{
			// The execution didn't complete as expected. Determine what happened.
			if( result == asEXECUTION_EXCEPTION )
			{
				// An exception occurred, let the script writer know what happened so it can be corrected.
				printf("An exception '%s' occurred. Please correct the code in file '%s' and try again.\n", context->GetExceptionString(), scriptname);
			}
		}
	}

	if(frameStepFunctionPtr>=0)
	{
		printf("Executing frameStep()\n");
		context->Prepare(frameStepFunctionPtr);
		context->SetArgFloat(0, 0.1f);
		result = context->Execute();
		if( result != asEXECUTION_FINISHED )
		{
			// The execution didn't complete as expected. Determine what happened.
			if( result == asEXECUTION_EXCEPTION )
			{
				// An exception occurred, let the script writer know what happened so it can be corrected.
				printf("An exception '%s' occurred. Please correct the code in file '%s' and try again.\n", context->GetExceptionString(), scriptname);
			}
		}
	}

	if(eventCallbackFunctionPtr>=0)
	{
		printf("Executing eventCallback()\n");
		context->Prepare(eventCallbackFunctionPtr);
		context->SetArgByte(0, 0);
		context->SetArgByte(1, 0);
		result = context->Execute();
		if( result != asEXECUTION_FINISHED )
		{
			// The execution didn't complete as expected. Determine what happened.
			if( result == asEXECUTION_EXCEPTION )
			{
				// An exception occurred, let the script writer know what happened so it can be corrected.
				printf("An exception '%s' occurred. Please correct the code in file '%s' and try again.\n", context->GetExceptionString(), scriptname);
			}
		}
	}

	return 0;
}

void ScriptEngine::ExceptionCallback(asIScriptContext *ctx, void *param)
{
	asIScriptEngine *engine = ctx->GetEngine();
	int funcID = ctx->GetExceptionFunction();
	const asIScriptFunction *function = engine->GetFunctionDescriptorById(funcID);
	printf("--- exception ---\n");
	printf("desc: %s\n",ctx->GetExceptionString());
	printf("func: %s\n",function->GetDeclaration());
	printf("modl: %s\n",function->GetModuleName());
	printf("sect: %s\n",function->GetScriptSectionName());
	int col, line = ctx->GetExceptionLineNumber(&col);
	printf("line: %d, %d\n", line, col);
	printLineError(line, col);

	// Print the variables in the current function
	PrintVariables(ctx, -1);

	// code below crashing, commented
	// Show the call stack with the variables
	printf("--- call stack ---\n");
	for( int n = 0; n < ctx->GetCallstackSize(); n++ )
	{
		funcID = ctx->GetCallstackFunction(n);
		const asIScriptFunction *func = engine->GetFunctionDescriptorById(funcID);
		line = ctx->GetCallstackLineNumber(n,&col);
		printf("%s:%s:%d,%d\n", func->GetModuleName(), func->GetDeclaration(), line, col);

		PrintVariables(ctx, n);
	}

	// force exit now
	exit(1);
}

void ScriptEngine::printLineError(int row, int col)
{
	if(!fileContents.size() || (int)fileContents.size() < row)
		return;

	if(row > 1) printf("% 3d| %s\n", row-1, fileContents[row-1].c_str());
	printf("% 3d| %s\n", row, fileContents[row].c_str());

	const char *p = fileContents[row].c_str();
	int tabs = 0;
	while(*p)
	{
		if(*p=='\t') tabs++;
		p++;
	}
	if(col!=0) col++;
	printf("   %*c_^_\n", col-1, ' ');
	printf("-------------------------------------------------------------------------------\n");
	//if(row < (int)fileContents.size()-1) printf("% 3d| %s\n", row+1, fileContents[row+1].c_str());
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
	sprintf(tmp+indent,"%s:%s:%d,%d", function->GetModuleName(), function->GetDeclaration(), line, col);
	printf("%s\n", tmp);

//	PrintVariables(ctx, -1);
}

void ScriptEngine::PrintVariables(asIScriptContext *ctx, int stackLevel)
{
	asIScriptEngine *engine = ctx->GetEngine();

	int typeId = ctx->GetThisTypeId(stackLevel);
	void *varPointer = ctx->GetThisPointer(stackLevel);
	if( typeId )
		printf(" this = 0x%x\n", varPointer);

	int numVars = ctx->GetVarCount(stackLevel);
	for( int n = 0; n < numVars; n++ )
	{
		int typeId = ctx->GetVarTypeId(n, stackLevel);
		void *varPointer = ctx->GetAddressOfVar(n, stackLevel);
		if( typeId == engine->GetTypeIdByDecl("int") )
		{
			printf(" %s = %d\n", ctx->GetVarDeclaration(n, stackLevel), *(int*)varPointer);
		}
		else if( typeId == engine->GetTypeIdByDecl("string") )
		{
			std::string *str = (std::string*)varPointer;
			if( str )
			{
				printf(" %s = '%s'\n", ctx->GetVarDeclaration(n, stackLevel), str->c_str());
			} else
			{
				printf(" %s = <null>\n", ctx->GetVarDeclaration(n, stackLevel));
			}
		}
	}
};

// wrappers for functions that are not directly usable

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
			printf("One of the arguments is incorrect, e.g. obj is null for a class method.\n");
			return;
		} else if(result == asNOT_SUPPORTED)
		{
			printf("The arguments are not supported, e.g. asCALL_GENERIC.\n");
			return;
		}
		printf("Unkown error while setting up message callback\n");
		return;
	}

	// AngelScript doesn't have a built-in string type, as there is no definite standard
	// string type for C++ applications. Every developer is free to register it's own string type.
	// The SDK do however provide a standard add-on for registering a string type, so it's not
	// necessary to register your own string type if you don't want to.
	RegisterStdString(engine);
	RegisterScriptMath_Native(engine);

	// Register everything
	// class Beam
	result = engine->RegisterObjectType("BeamClass", sizeof(Beam), asOBJ_REF); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", asMETHOD(Beam,scaleTruck), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "string &getTruckName()", asMETHOD(Beam,getTruckName), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void resetPosition(float, float, float, bool, float)", asMETHOD(Beam,resetPosition), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setDetailLevel(int)", asMETHOD(Beam,setDetailLevel), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void showSkeleton(bool, bool)", asMETHOD(Beam,showSkeleton), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void hideSkeleton(bool)", asMETHOD(Beam,hideSkeleton), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", asMETHOD(Beam,parkingbrakeToggle), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void beaconsToggle()", asMETHOD(Beam,beaconsToggle), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setReplayMode(bool)", asMETHOD(Beam,setReplayMode), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void resetAutopilot()", asMETHOD(Beam,resetAutopilot), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void toggleCustomParticles()", asMETHOD(Beam,toggleCustomParticles), asCALL_THISCALL); assert(result>=0);
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
	result = engine->RegisterObjectMethod("BeamClass", "bool getReverseLightVisible()", asMETHOD(Beam,getReverseLightVisible), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getHeadingDirectionAngle()", asMETHOD(Beam,getHeadingDirectionAngle), asCALL_THISCALL); assert(result>=0);
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

	// class Cache_Entry
	/*
	result = engine->RegisterObjectType("Cache_EntryClass", sizeof(Cache_Entry), asOBJ_REF); assert(result>=0);
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
	result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonPosition(float, float, float)", asMETHOD(GameScript,setPersonPosition), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void movePerson(float, float, float)", asMETHOD(GameScript,movePerson), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getCaelumTime()", asMETHOD(GameScript,getCaelumTime), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setCaelumTime(float)", asMETHOD(GameScript,setCaelumTime), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setWaterHeight(float)", asMETHOD(GameScript,setWaterHeight), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getWaterHeight()", asMETHOD(GameScript,getWaterHeight), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getCurrentTruckNumber()", asMETHOD(GameScript,getCurrentTruckNumber), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucks()", asMETHOD(GameScript,getCurrentTruckNumber), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float getGravity()", asMETHOD(GameScript,getGravity), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setGravity(float)", asMETHOD(GameScript,setGravity), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void flashMessage(const string &in, float, float)", asMETHOD(GameScript,flashMessage), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setDirectionArrow(const string &in, float, float, float)", asMETHOD(GameScript,setDirectionArrow), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void registerForEvent(int)", asMETHOD(GameScript,registerForEvent), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getCurrentTruck()", asMETHOD(GameScript,getCurrentTruck), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getTruckByNum(int)", asMETHOD(GameScript,getTruckByNum), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "int getChatFontSize()", asMETHOD(GameScript,getChatFontSize), asCALL_THISCALL); assert(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setChatFontSize(int)", asMETHOD(GameScript,setChatFontSize), asCALL_THISCALL); assert(result>=0);

	// class CacheSystem
//	result = engine->RegisterObjectType("CacheSystemClass", sizeof(CacheSystem), asOBJ_REF | asOBJ_NOHANDLE);
//	result = engine->RegisterObjectMethod("CacheSystemClass", "bool checkResourceLoaded(string &in)", asMETHODPR(CacheSystem, checkResourceLoaded, (Ogre::String &), bool), asCALL_THISCALL); assert(result>=0);
//	result = engine->RegisterObjectMethod("CacheSystemClass", "bool checkResourceLoaded(string &in, string &in)", asMETHODPR(CacheSystem, checkResourceLoaded, (Ogre::String &, Ogre::String &), bool), asCALL_THISCALL); assert(result>=0);
	// unable to register the following:
	//result = engine->RegisterObjectMethod("CacheSystemClass", "bool checkResourceLoaded(Cache_EntryClass)", asMETHODPR(CacheSystem, checkResourceLoaded, (Cache_Entry), bool), asCALL_THISCALL); assert(result>=0);
//	result = engine->RegisterObjectMethod("CacheSystemClass", "Cache_EntryClass &getResourceInfo(string)", asMETHODPR(CacheSystem, getResourceInfo, (Cache_Entry), bool), asCALL_THISCALL); assert(result>=0);
//	result = engine->RegisterObjectMethod("CacheSystemClass", "Cache_EntryClass &getResourceInfo(string)", asMETHODPR(CacheSystem, getResourceInfo, (Cache_Entry), bool), asCALL_THISCALL); assert(result>=0);
//	result = engine->RegisterObjectMethod("CacheSystemClass", "Cache_EntryClass &getResourceInfo(string)", asMETHODPR(CacheSystem, getResourceInfo, (Cache_Entry), bool), asCALL_THISCALL); assert(result>=0);
	// these are static methods, special handling for them :)
//	result = engine->RegisterGlobalFunction("string stripUIDfromString(string)", asFUNCTION(CacheSystem::stripUIDfromString), asCALL_CDECL); assert(result>=0);
//	result = engine->RegisterGlobalFunction("string getUIDfromString(string)", asFUNCTION(CacheSystem::getUIDfromString), asCALL_CDECL); assert(result>=0);
//	result = engine->RegisterGlobalFunction("bool stringHasUID(string)", asFUNCTION(CacheSystem::stringHasUID), asCALL_CDECL); assert(result>=0);

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

	// now the global instances
	GameScript *gamescript = new GameScript();
	result = engine->RegisterGlobalProperty("GameScriptClass game", gamescript); assert(result>=0);

//	result = engine->RegisterGlobalProperty("CacheSystemClass cache", &CacheSystem::Instance()); assert(result>=0);

//	result = engine->RegisterObjectType("Vector3Class", sizeof(Ogre::Vector3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS); assert(result>=0);
	// TODO: add complete Vector3 class :(
	//result = engine->RegisterObjectMethod("Vector3Class", "...", asMETHOD(Ogre::Vector3,...), asCALL_THISCALL);


	//printf("Registration done\n");
}

void ScriptEngine::msgCallback(const asSMessageInfo *msg)
{
	const char *type = "Error";
	if( msg->type == asMSGTYPE_INFORMATION )
		type = "Info";
	else if( msg->type == asMSGTYPE_WARNING )
		type = "Warning";

	printf("* %s (Line %d, Col %d): %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
	
	if(msg->type == asMSGTYPE_WARNING || msg->type == asMSGTYPE_ERROR)
		printLineError(msg->row, msg->col);
}


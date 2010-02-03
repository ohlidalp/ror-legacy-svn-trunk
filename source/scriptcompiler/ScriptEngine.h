#ifndef SCRIPTENGINE_H__
#define SCRIPTENGINE_H__

#include <string>
#include <map>
#include "angelscript.h"
#include "scriptstdstring/scriptstdstring.h" // angelscript addon
#include "scriptmath/scriptmath.h" // angelscript addon
#include <string.h>

class ScriptEngine
{
public:
	ScriptEngine();
	~ScriptEngine();

	int loadFile(std::string filename);


protected:
    asIScriptEngine *engine;                //!< instance of the scripting engine
	asIScriptContext *context;              //!< context in which all scripting happens
	int frameStepFunctionPtr;               //!< script function pointer to the frameStep function
	int eventCallbackFunctionPtr;           //!< script function pointer to the event callback function

	std::map<int, std::string> fileContents;

    void init();
    void msgCallback(const asSMessageInfo *msg);
	
	int loadScriptFile(const char *fileName, std::string &script);
	int loadFileInMemory(std::string scriptname);

	void printLineError(int row, int col);

	void ExceptionCallback(asIScriptContext *ctx, void *param);
	void PrintVariables(asIScriptContext *ctx, int stackLevel);
	void LineCallback(asIScriptContext *ctx, void *param);
};


#endif //SCRIPTENGINE_H__

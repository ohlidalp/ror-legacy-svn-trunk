
#include "resource.h"
#include "ScriptEngine.h"

void usage(char *name)
{
	printf("usage: %s <filename>", name);
}

int main(int argc, char *argv[])
{
	if(argc != 2) 
	{
		usage(argv[0]);
		return 1;
	}
	ScriptEngine *se = new ScriptEngine();
	return se->loadFile(argv[1]);
	return 0;
}
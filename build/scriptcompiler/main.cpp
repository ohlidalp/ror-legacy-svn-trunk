#include "ScriptEngine.h"

void usage(char *name)
{
	printf("usage: %s <filename>", name);
	exit(1);
}

int main(int argc, char *argv[])
{
	if(argc != 2) usage(argv[0]);
	ScriptEngine *se = new ScriptEngine();
	return se->loadFile(argv[1]);
	return 0;
}
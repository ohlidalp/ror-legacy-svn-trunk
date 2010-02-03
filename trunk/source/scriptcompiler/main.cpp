
#ifdef WIN32
# include <conio.h> // for getch
#endif

#include <stdio.h> // printf
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
	int res = se->loadFile(argv[1]);
#ifdef WIN32
	// wait for key press
	printf("Press any key to continue...\n");
	_getch();
#endif
	return res;
}

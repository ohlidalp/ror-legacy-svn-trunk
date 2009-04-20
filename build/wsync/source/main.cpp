#include "wsync.h"

#ifdef WIN32
#include <conio.h> // for getch
#endif

using namespace std; 
using namespace boost::filesystem; 

void usage()
{
	printf("usage: wsync <mode> <args>\n");
	printf("\n");
	printf("wsync create <local path> <mode>\n");
	printf(" * creates a new file index for the specified directory\n");
	printf("   mode can be 'full' or 'not-delete'\n");
	printf("\n");
	printf("wsync sync\n");
	printf(" * shortcut with predefined paths for RoR\n");
	printf("\n");
	printf("wsync sync <local path> <remote server> <remote path>\n");
	printf(" * syncs the specified directory with the server\n");
}

int main(int argc, char **argv)
{

	if(argc == 4 && !strcmp(argv[1], "create"))
	{
		path local_path = path(string(argv[2]), native);
		string mode = string(argv[3]);
		int modeNumber = 0;
		if(mode == "full")          modeNumber |= WMO_FULL;
		else if(mode == "nodelete") modeNumber |= WMO_NODELETE;

		WSync *w = new WSync();
		if(w->createIndex(local_path, modeNumber))
			printf("error creating index\n");
		else
			printf("index created: %s\n", (local_path / INDEXFILENAME).string().c_str());

	} else if(argc == 5 && !strcmp(argv[1], "sync"))
	{
		string local_path = string(argv[2]);
		string remote_server = string(argv[3]);
		string remote_path = string(argv[4]);
		WSync *w = new WSync();
		w->sync(local_path, remote_server, remote_path);
	} else if((argc == 2 && !strcmp(argv[1], "sync")) || (argc == 1) )
	{
		// shortcut
		string local_path = ".";
		string remote_server = "wsync.rigsofrods.com";
		string remote_path = "/";
		WSync *w = new WSync();
		w->sync(local_path, remote_server, remote_path);
		if(argc == 1)
		{
#ifdef WIN32
			// wait for key press
			printf("Press any key to continue...\n");
			_getch();
#endif
		}
/*
	} else if(argc == 2 && !strcmp(argv[1], "test"))
	{
		std::vector< std::vector< std::string > > list;
		std::vector< std::vector< std::string > >::iterator it1;
		std::vector< std::string >::iterator it2;
		WSync *w = new WSync();
		w->downloadConfigFile("wsync.rigsofrods.com", "/streams.index", list);
		for(it1=list.begin();it1!=list.end();it1++)
		{
			printf("====\n");
			for(it2=it1->begin();it2!=it1->end();it2++)
			{
				printf("'%s'\n", it2->c_str());
			}
		}
*/
	} else
	{
		usage();
	}
	return 0;
}

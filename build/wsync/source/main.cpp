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

	} else if(argc == 4 && (!strcmp(argv[1], "dlmod")))
	{
		// short version that is less verbose
		string modname = argv[2];
		string path = argv[3];
		WSync *w = new WSync();
		return w->downloadMod(modname, path, true);
	} else if(argc == 4 && (!strcmp(argv[1], "downloadmod")))
	{
		string modname = argv[2];
		string path = argv[3];
		WSync *w = new WSync();
		return w->downloadMod(modname, path);
	} else if(argc == 5 && (!strcmp(argv[1], "sync") || !strcmp(argv[1], "sync2")))
	{
		printf("syncing...\n");
		string local_path = string(argv[2]);
		string remote_server = string(argv[3]);
		string remote_path = string(argv[4]);
		WSync *w = new WSync();
		bool deleteOK = exists(path("RoR.exe"));
		if(!deleteOK) printf("RoR.exe not detected, wont delete any files in here!\n");
		w->sync(local_path, remote_server, remote_path, true, deleteOK);

#ifdef WIN32
		path exe_path = system_complete("update.exe");
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		DWORD               dwCode  =   0;
		memset(&si, 0, sizeof(STARTUPINFO));
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));
		si.cb           = sizeof(STARTUPINFO);
		si.dwFlags      = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.hStdInput    = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput   = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError    = GetStdHandle(STD_ERROR_HANDLE);
		si.lpTitle      = "sync";
		si.wShowWindow  = SW_SHOW;

		char path[2048];
		sprintf(path, "%s postsync", exe_path.file_string().c_str());

		//printf("cmdline2: '%s'\n", path);
		CreateProcess(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
		exit(0);
	} else if(argc == 2 && !strcmp(argv[1], "postsync"))
	{
		Sleep(100);
		path local_path = boost::filesystem::current_path();
		path exe_path = system_complete(path(argv[0]));
		path exe_temp = exe_path;
		exe_temp.replace_extension(".temp.exe");
		if(exists(exe_temp))
			remove(exe_temp);
		// wait for key press
		printf("Press any key to continue...\n");
		_getch();
		exit(0);
	} else if(argc == 1)
	{
		printf("duplicating...\n");
		path local_path = boost::filesystem::current_path();
		path exe_path = system_complete(path(argv[0]));
		path exe_temp = exe_path;
		exe_temp.replace_extension(".temp.exe");

		//printf("this exe path: %s\n", exe_path.string().c_str());
		//printf("temporary exe path: %s\n", exe_temp.string().c_str());
		//printf("path to update: %s\n", local_path.string().c_str());


		// 3. copy self to temp file
		try
		{
			if(exists(exe_temp))
				remove(exe_temp);
			if(exists(exe_temp))
			{
				printf("error removing tempfile!\n");
				return 1;
			}
			boost::filesystem::copy_file(exe_path, exe_temp);
		} catch(...)
		{
			printf("error duplicating self!\n");
			printf("this step is required to possibly update the updater\n");
			printf("ensure that you run this application with administrator\n");
			printf("privileges to be able to create files in the current directory.\n");
			return 1;
		}

		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		DWORD               dwCode  =   0;
		memset(&si, 0, sizeof(STARTUPINFO));
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));
		si.cb           = sizeof(STARTUPINFO);
		si.dwFlags      = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.hStdInput    = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput   = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError    = GetStdHandle(STD_ERROR_HANDLE);
		si.lpTitle      = "sync";
		si.wShowWindow  = SW_SHOW;


		char path[2048];
		sprintf(path, "%s sync2 \"%s\" wsync.rigsofrods.com /", exe_temp.file_string().c_str(), local_path.file_string().c_str());

		//printf("cmdline1: '%s'\n", path);
		CreateProcess(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
		exit(0);
	} else if((argc == 2 && !strcmp(argv[1], "sync")))
	{
		// shortcut
		string local_path = ".";
		string remote_server = "wsync.rigsofrods.com";
		string remote_path = "/";
		WSync *w = new WSync();
		bool deleteOK = exists(path("RoR.exe"));
		if(!deleteOK) printf("RoR.exe not detected, wont delete any files in here!\n");
		w->sync(local_path, remote_server, remote_path, true, deleteOK);
		if(argc == 1)
		{
			// wait for key press
			printf("Press any key to continue...\n");
			_getch();
		}
#endif

#if 0
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
#endif
	} else
	{
		usage();
	}
	return 0;
}

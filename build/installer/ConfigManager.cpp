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
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
//#include "winsock2.h"
#endif //WIN32

#include "ConfigManager.h"


#include "boost/asio.hpp"
#include "boost/filesystem.hpp"

//icons must be 50x50
#include "unknown.xpm"
#include "mainpack.xpm"
#include "extrapack.xpm"
#include "wsync.h"
#include <string>

// string conversion utils
inline wxString conv(const char *s)
{
	return wxString(s, wxConvUTF8);
}

inline wxString conv(const std::string& s)
{
	return wxString(s.c_str(), wxConvUTF8);
}

inline std::string conv(const wxString& s)
{
	return std::string(s.mb_str(wxConvUTF8));
}

ConfigManager::ConfigManager()
{
	streamset=0;
	//add default streams
	//appendStream(_T("Base game"), _T("The minimum you need to run the game."), wxBitmap(mainpack_xpm), true, true);
	//appendStream(_T("Standard media pack"), _T("The best terrains and vehicles, highly recommended!"), wxBitmap(extrapack_xpm), true, false);
	//for (int i=0; i<5; i++) 
	//	appendStream(_T("Test pack"), _T("This is a test"), wxBitmap(unknown_xpm), false, false);
}

int ConfigManager::getOnlineStreams()
{
	// assume the linked list is empty
	//FILE *f2 = fopen("log.txt", "w");
	WSync *w = new WSync();
	boost::filesystem::path tempfile;
	w->getTempFilename(tempfile);
	int res = w->downloadFile(tempfile, "wsync.rigsofrods.com", "/streams.index");
	if(res)
	{
		//fprintf(f2, "error downloading file: error code %d\n", res);
		//fclose(f2);
		// error
		return 1;
	}
	//fprintf(f2, "file '%s'\n", tempfile.string().c_str());
	FILE *f = fopen(tempfile.string().c_str(), "r");
	if (!f)
	{
		//fprintf(f2, "error opening file '%s'\n", tempfile.string().c_str());
		//fclose(f2);
		return -1;
	}
	while(!feof(f))
	{
		char name[256]="";
		char version[20]="";
		char description[4096]="";
		int type=0, checked=0, disabled=0;
		int res = fscanf(f, "%d %d %d %s %s %s\n", &type, &checked, &disabled, name, version, description);
		//fprintf(f2, "### 8 : %d\n", res);
		if(res < 6)
			break;
		// convert underscores back
		char *p2=name;
		while(*p2){ p2++; if(*p2=='_') *p2=' '; }
		p2=description;
		while(*p2){ p2++; if(*p2=='_') *p2=' '; }
		appendStream(conv(name), conv(description), ((type==0)?wxBitmap(mainpack_xpm):wxBitmap(extrapack_xpm)), checked!=0, disabled!=0);
		//fprintf(f2, "appending stream...\n");
	}
	fclose (f);
	//if(f2) fclose (f2);
	boost::filesystem::remove(tempfile);
	return 0;

}

bool ConfigManager::isFirstInstall()
{
	return true;
}

bool ConfigManager::isLicenceAccepted()
{
	return !isFirstInstall();
}

void ConfigManager::setAction(int ac)
{
}

void ConfigManager::setPath(wxString pth)
{
}

stream_desc_t* ConfigManager::getStreamset()
{
	return streamset;
}

void ConfigManager::appendStream(wxString title, wxString desc, wxBitmap icon, bool checked, bool disabled)
{
	stream_desc_t** spt=&streamset;
	//go to the end
	while (*spt) spt=&((*spt)->next);
	//allocate
	*spt=new stream_desc_t;
	(*spt)->title=title;
	(*spt)->desc=desc;
	(*spt)->icon=icon;
	(*spt)->checked=checked;
	(*spt)->disabled=disabled;
	(*spt)->next=NULL;
}

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
#include "wx/msw/private.h"
#include "wx/msw/registry.h"
#include <shellapi.h> // needed for SHELLEXECUTEINFO


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
	streams.clear();
}

int ConfigManager::getOnlineStreams()
{
	//clear list
	clearStreamset();
	
	// now get the available streams list
	WSync *w = new WSync();

	std::vector< std::map< std::string, std::string > > olist;
	if(!w->downloadAdvancedConfigFile("wsync.rigsofrods.com", "/streams.index", olist))
	{
		if(olist.size() > 0)
		{
			for(int i=0;i<(int)olist.size();i++)
			{
				stream_desc_t s;
				s.title     = conv(olist[i]["title"]);
				s.desc      = conv(olist[i]["description"]);
				s.group     = conv(olist[i]["group"]);
				s.path      = conv(olist[i]["path"]);

				s.icon      = (olist[i]["type"]=="0")?wxBitmap(mainpack_xpm):wxBitmap(extrapack_xpm);
				s.checked   = (olist[i]["checked"] == "1");
				s.disabled  = (olist[i]["checked"] == "1");
				s.beta      = (olist[i]["beta"] == "1");
				s.del       = (olist[i]["delete"] == "1");
				s.overwrite = (olist[i]["overwrite"] == "1");
				s.overwrite = (olist[i]["overwrite"] == "1");
				conv(olist[i]["size"]).ToULong(&s.size);

				if(!s.title.size()) continue;

				streams.push_back(s);
			}

		} else
			return 1;
	}
	

	
	//add default streams
	//appendStream(_T("Base game"), _T("The minimum you need to run the game."), wxBitmap(mainpack_xpm), true, true);
	//appendStream(_T("Standard media pack"), _T("The best terrains and vehicles, highly recommended!"), wxBitmap(extrapack_xpm), true, false);
	//for (int i=0; i<5; i++) 
	//	appendStream(_T("Test pack"), _T("This is a test"), wxBitmap(unknown_xpm), false, false);

	return 0;

}

wxString ConfigManager::getInstallationPath()
{
#ifdef WIN32
	wxString path;
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods"));
	if(!pRegKey->Exists())
		return wxString();
	
	pRegKey->QueryValue(wxT("InstallPath"), path);
	return path + wxT("\\");
	// see http://docs.wxwidgets.org/stable/wx_wxregkey.html
	//wxMessageBox(path,wxT("Registry Value"),0);
#else
	return wxString();
#endif //WIN32
}


void ConfigManager::setInstallationPath()
{
#ifdef WIN32
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods"));
	if(!pRegKey->Exists())
		pRegKey->Create();
	pRegKey->SetValue(wxT("InstallPath"), installPath);
#else
	// TODO: implement
#endif //WIN32
}

bool ConfigManager::isFirstInstall()
{
	wxString path = getInstallationPath();
	installPath = path; //dont use setter, because it would write into the registry again
#ifdef WIN32
	if(path.empty())
		return true;
#endif //WIN32
	return !wxFileExists(path + wxT("RoR.exe"));
}

bool ConfigManager::isLicenceAccepted()
{
	return !isFirstInstall();
}

void ConfigManager::setAction(int ac)
{
	installeraction = ac;
}

int ConfigManager::getAction()
{
	return installeraction;
}

void ConfigManager::setInstallPath(wxString pth)
{
	installPath=pth;
	setInstallationPath();
}

wxString ConfigManager::getInstallPath()
{
	return installPath;
}

std::vector < stream_desc_t > *ConfigManager::getStreamset()
{
	return &streams;
}

void ConfigManager::clearStreamset()
{
	streams.clear();
}

void ConfigManager::setStreamSelection(stream_desc_t* desc, bool selection)
{
	for(std::vector < stream_desc_t >::iterator it=streams.begin(); it!=streams.end(); it++)
	{
		// we use the path, since it is unique
		if(it->path == desc->path)
		{
			if (!it->disabled)
				it->checked=selection;
			return;
		}
	}
	//if we are here, an invalid desc has been provided! better safe than sorry.
}

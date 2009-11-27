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
#if PLATFORM == PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "wx/msw/private.h"
#include "wx/msw/registry.h"
#include <wx/msgdlg.h>
#include <shellapi.h> // needed for SHELLEXECUTEINFO
#include <shlobj.h>


std::string wstrtostr(const std::wstring &wstr)
{
    // Convert a Unicode string to an ASCII string
    std::string strTo;
    char *szTo = new char[wstr.length() + 1];
    szTo[wstr.size()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL);
    strTo = szTo;
    delete[] szTo;
    return strTo;
}

std::wstring strtowstr(const std::string &str)
{
    // Convert an ASCII string to a Unicode String
    std::wstring wstrTo;
    wchar_t *wszTo = new wchar_t[str.length() + 1];
    wszTo[str.size()] = L'\0';
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wszTo, (int)str.length());
    wstrTo = wszTo;
    delete[] wszTo;
    return wstrTo;
}

//#include "winsock2.h"
#endif //WINDOWS
#include <wx/filename.h>
#include <wx/dir.h>

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
	dlerror=0;
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
				s.conflict  = conv(olist[i]["conflict"]);
				s.group     = conv(olist[i]["group"]);
				s.path      = conv(olist[i]["path"]);

				s.icon      = (olist[i]["type"]=="0")?wxBitmap(mainpack_xpm):wxBitmap(extrapack_xpm);
				s.checked   = (olist[i]["checked"] == "1");
				s.forcecheck= (olist[i]["forcecheck"] == "1");
				s.hidden    = (olist[i]["hidden"] == "1");
				s.disabled  = (olist[i]["disabled"] == "1");
				s.beta      = (olist[i]["beta"] == "1");
				s.del       = (olist[i]["delete"] == "1");
				s.overwrite = (olist[i]["overwrite"] == "1");
				//s.overwrite = (olist[i]["overwrite"] == "1");
				conv(olist[i]["size"]).ToULong(&s.size);

				if(!s.title.size()) continue;

				streams.push_back(s);
			}

		} else
			return 1;
	}

	if(!streams.size() && dlerror < 3)
	{
		// try to download three times...
		dlerror++;
		return getOnlineStreams();
	}


	//add default streams
	//appendStream(_T("Base game"), _T("The minimum you need to run the game."), wxBitmap(mainpack_xpm), true, true);
	//appendStream(_T("Standard media pack"), _T("The best terrains and vehicles, highly recommended!"), wxBitmap(extrapack_xpm), true, false);
	//for (int i=0; i<5; i++)
	//	appendStream(_T("Test pack"), _T("This is a test"), wxBitmap(unknown_xpm), false, false);

	loadStreamSubscription();

	return 0;

}

wxString ConfigManager::getInstallationPath()
{
#if PLATFORM == PLATFORM_WINDOWS
	wxString path;
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods"));
	if(!pRegKey->Exists())
		return wxString();

	if(!pRegKey->HasValue(wxT("InstallPath")))
		return wxString();

	pRegKey->QueryValue(wxT("InstallPath"), path);
	path += wxT("\\");
	// check if RoR.exe exists
	bool exists = wxFileExists(path+wxT("RoR.exe"));
	if(!exists)
		return wxString();
	// existing, everything correct.
	return path;
	// see http://docs.wxwidgets.org/stable/wx_wxregkey.html
	//wxMessageBox(path,wxT("Registry Value"),0);
#else
	return wxString();
#endif //WIN32
}

int ConfigManager::uninstall(bool deleteUserFolder)
{
	// TODO: implement uninstall for non-windows versions!
#if WIN32
	wxString ipath = getInstallPath();
	if(ipath.empty())
		return 1;

	wxString mtxt = _T("Will now:\n\n");
	mtxt += wxT("remove the main installation directory recursivly:\n") + ipath + wxT("\n\n");

	wxString userPath;
	if(deleteUserFolder)
	{
		LPWSTR wuser_path = new wchar_t[1024];
		if (SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, wuser_path)!=S_OK)
			return 3;
		GetShortPathName(wuser_path, wuser_path, 512); //this is legal
		std::string user_path_str = wstrtostr(std::wstring(wuser_path));
		user_path_str += "\\Rigs of Rods\\";
		userPath = conv(user_path_str);
		mtxt += wxT("remove the Rigs of Rods user content directory recursivly:\n") + userPath + wxT("\n\n");
	}


	// remove shortcuts
	wxString startmenuDir, desktopDir, workingDirectory = ipath, desktopLink;
	if(!SHGetSpecialFolderPath(0, wxStringBuffer(startmenuDir, MAX_PATH), CSIDL_COMMON_PROGRAMS, FALSE))
		return 8;
	if(!SHGetSpecialFolderPath(0, wxStringBuffer(desktopDir, MAX_PATH), CSIDL_DESKTOP, FALSE))
		return 9;

	if(workingDirectory.size() > 3 && workingDirectory.substr(workingDirectory.size()-1,1) != wxT("\\"))
		workingDirectory += wxT("\\");

	startmenuDir += wxT("\\Rigs of Rods");
	if (!wxDir::Exists(startmenuDir))
		startmenuDir = wxString();

	desktopLink = desktopDir + wxT("\\Rigs of Rods.lnk");
	if(!wxFileName::FileExists(desktopLink))
		desktopLink = wxString();

	if(!startmenuDir.empty())
		mtxt += wxT("remove the Rigs of Rods start menu directory:\n") + startmenuDir + wxT("\n\n");
	if(!desktopLink.empty())
		mtxt += wxT("remove the Rigs of Rods Desktop Link:\n") + desktopLink + wxT("\n");

	mtxt += wxT("remove the Rigs of Rods Registry entry:\nHKEY_LOCAL_MACHINE\\Software\\RigsOfRods \n\n");
	mtxt += wxT("do you want to continue?");
	int res = wxMessageBox(mtxt, _T("Uninstall?"), wxICON_QUESTION | wxYES_NO);
	if(res != wxYES)
	{
		return 10;
	}

	// this is called upon uninstall to clean the system from meta things
	wxFileName *f = new wxFileName(ipath);
	// wxPATH_RMDIR_RECURSIVE is available in wxWidgets >= 2.9.0
#if wxCHECK_VERSION(2, 9, 0)
	bool rmres = f->Rmdir(wxPATH_RMDIR_RECURSIVE);
#else
	#error You need at least wxWidgets version 2.9.0 in order to compile the installer correctly!
#endif
	if(!rmres)
		return 2;

	if(deleteUserFolder && !userPath.empty())
	{
		wxFileName *f = new wxFileName(userPath);
#if wxCHECK_VERSION(2, 9, 0)
		bool res = f->Rmdir(wxPATH_RMDIR_RECURSIVE);
#else
		#error You need at least wxWidgets version 2.9.0 in order to compile the installer correctly!
#endif
		if(!res)
			return 4;
	}
	// remove registry keys
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods"));
	if(pRegKey->Exists())
		pRegKey->DeleteSelf();

	// remove shortcuts
	if(!startmenuDir.empty())
#if wxCHECK_VERSION(2, 9, 0)
		wxFileName::Rmdir(startmenuDir, wxPATH_RMDIR_RECURSIVE);
#else
		#error You need at least wxWidgets version 2.9.0 in order to compile the installer correctly!
#endif

	if(!desktopLink.empty())
		wxRemoveFile(desktopLink);
#endif // WIN32
	return 0;
}

void ConfigManager::saveStreamSubscription()
{
	if(!streams.size()) return;
	for(std::vector < stream_desc_t >::iterator it=streams.begin(); it!=streams.end(); it++)
	{
#if PLATFORM == PLATFORM_WINDOWS
		wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\streams"));
		if(!pRegKey->Exists())
			pRegKey->Create();
		pRegKey->SetValue(it->path, it->checked?wxT("yes"):wxT("no"));
#else
	// TODO: implement
#endif //WIN32
	}
}

void ConfigManager::loadStreamSubscription()
{
	if(!streams.size()) return;
	for(std::vector < stream_desc_t >::iterator it=streams.begin(); it!=streams.end(); it++)
	{
#if PLATFORM == PLATFORM_WINDOWS
		if(it->forcecheck) continue; // we enforce the value of this
		wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\streams"));
		if(!pRegKey->Exists())
			return;

		wxString enabled;
		if(!pRegKey->HasValue(it->path)) continue;
		pRegKey->QueryValue(it->path, enabled);
		if(enabled == wxT("yes"))
			it->checked = true;
		else if(enabled == wxT("no"))
			it->checked = false;
#else
		// TODO: implement
#endif //WIN32
	}
}


void ConfigManager::setPersistentConfig(wxString name, wxString value)
{
#if PLATFORM == PLATFORM_WINDOWS
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\config"));
	if(!pRegKey->Exists())
		pRegKey->Create();
	pRegKey->SetValue(name, value);
#else
	// TODO: implement
#endif //WIN32
}

wxString ConfigManager::getPersistentConfig(wxString name)
{
#if PLATFORM == PLATFORM_WINDOWS
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\config"));
	if(!pRegKey->Exists())
		return wxT("");

	if(!pRegKey->HasValue(name))
		return wxT("");

	wxString result;
	pRegKey->QueryValue(name, result);
	return result;
#else
	// TODO: implement
#endif //WIN32
}
void ConfigManager::setInstallationPath()
{
#if PLATFORM == PLATFORM_WINDOWS
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
#if PLATFORM == PLATFORM_WINDOWS
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
			//if (!it->disabled)
			it->checked=selection;
			return;
		}
	}
	//if we are here, an invalid desc has been provided! better safe than sorry.
}

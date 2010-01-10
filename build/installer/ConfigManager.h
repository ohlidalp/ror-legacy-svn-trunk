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

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#include <vector>
#include "boost/asio.hpp"
#include "boost/filesystem.hpp"

// for all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/frame.h"
    #include "wx/stattext.h"
    #include "wx/log.h"
    #include "wx/app.h"
	#include "wx/statbmp.h"
#endif

#define CONFIG ConfigManager::getSingleton()

typedef struct stream_desc_t_
{
	wxString title;
	wxString path;
	wxString conflict;
	wxString platform;
	wxString desc;
	wxString group;
	wxBitmap icon;
	bool checked;
	bool forcecheck;
	bool hidden;
	bool disabled;
	bool del;
	bool beta;
	bool overwrite;
	unsigned long size;
} stream_desc_t;

class ConfigManager
{
public:
	ConfigManager();
	bool isFirstInstall();
	bool isLicenceAccepted();

	void setAction(int ac);
	int getAction();


	int uninstall(bool deleteUserFolder=false);

	// stream things
	std::vector < stream_desc_t > *getStreamset();
	int getOnlineStreams();
	void setStreamSelection(stream_desc_t* desc, bool selection);
	void loadStreamSubscription();
	void saveStreamSubscription();

	// registry things
	wxString getPersistentConfig(wxString name);
	void setPersistentConfig(wxString name, wxString value);
	void setInstallPath(wxString pth);
	wxString getInstallationPath();

	void setStartupMode(int val) { this->statupMode = val; };
	int getStartupMode() { return this->statupMode; };

	static ConfigManager *getSingleton() { return instance; };
	static ConfigManager *instance;



	void updateUserConfigFile(std::string filename, boost::filesystem::path iPath, boost::filesystem::path uPath);
	void updateUserConfigs();
	void installRuntime();
	void startConfigurator();
	void viewManual();
	void executeBinary(wxString filename, wxString action = wxT("runas"), wxString parameters = wxString(), wxString cwDir = wxString("cwd"), bool prependCWD=true);
	int createShortcut(wxString linkTarget, wxString workingDirectory, wxString linkFile, wxString linkDescription);
	void createProgramLinks(bool desktop, bool startmenu);


private:
	int statupMode;
	wxString installPath;
	int installeraction;
	int dlerror;
	std::vector < stream_desc_t > streams;
	void clearStreamset();
	void setInstallationPath();
};

#endif // CONFIG_MANAGER_H

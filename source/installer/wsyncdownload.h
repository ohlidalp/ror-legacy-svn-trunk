#ifndef WSYNCDOWNLOAD_H__
#define WSYNCDOWNLOAD_H__

#include <wx/event.h>

#include "cevent.h"
#include "Timer.h"

#include "boost/asio.hpp"
#include "boost/filesystem.hpp"

class WsyncDownload
{
public:
	WsyncDownload(wxEvtHandler* parent=0);

	// main functions
	int downloadFile(int jobID, boost::filesystem::path localFile, std::string server, std::string remoteDir, boost::uintmax_t predDownloadSize=0, boost::uintmax_t *fileSize=0, bool showProgress=false);
	int downloadAdvancedConfigFile(std::string server, std::string url, std::vector< std::map< std::string, std::string > > &list, bool showProgress=false);
	int downloadConfigFile(std::string server, std::string url, std::vector< std::vector< std::string > > &list, bool showProgress=false);

	// utils
	static void tryRemoveFile(boost::filesystem::path filename);

	static void increaseServerStats(std::string server, boost::uintmax_t bytes);

	void setDownloadMessage(wxString msg) { mDownloadMessage = msg; };

protected:
	wxEvtHandler* parent;
	static std::map < std::string, boost::uintmax_t > traffic_stats;

	void reportDownloadProgress(int jobID, Timer dlStartTime, boost::uintmax_t predDownloadSize, boost::uintmax_t downloaded);
	void updateCallback(int jobID, int type, std::string txt = std::string(), float percent=-1);

	wxString mDownloadMessage;
};

#endif //WSYNCDOWNLOAD_H__
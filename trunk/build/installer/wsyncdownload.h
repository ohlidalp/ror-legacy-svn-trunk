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
	int downloadFile(boost::filesystem::path localFile, std::string server, std::string remoteDir, boost::uintmax_t predDownloadSize=0, boost::uintmax_t *fileSize=0);
	int downloadAdvancedConfigFile(std::string server, std::string url, std::vector< std::map< std::string, std::string > > &list);
	int downloadConfigFile(std::string server, std::string url, std::vector< std::vector< std::string > > &list);

	// utils
	static int cleanURL(std::string &url);
	static int ensurePathExist(boost::filesystem::path &path);
	static void tryRemoveFile(boost::filesystem::path filename);
	static int getTempFilename(boost::filesystem::path &tempfile);

	static void increaseServerStats(std::string server, boost::uintmax_t bytes);
	static std::string formatFilesize(boost::uintmax_t size);
	static std::string formatSeconds(float seconds);
protected:
	wxEvtHandler* parent;
	static std::map < std::string, boost::uintmax_t > traffic_stats;

	void reportDownloadProgress(Timer dlStartTime, boost::uintmax_t predDownloadSize, boost::uintmax_t downloaded);
	void updateCallback(int type, std::string txt = std::string(), float percent=-1);
};

#endif //WSYNCDOWNLOAD_H__
#ifndef WTHREAD_H__
#define WTHREAD_H__

#include <wx/thread.h>
#include <wx/event.h>
#include "wsync.h"
#include <string.h>
#include "ConfigManager.h" //only use for stream_desc_t, dont use functions, as its not thread safe!

enum { CMD_THREAD_DONE };

class DownloadPage;

class WsyncThread : public wxThread
{
public:
	WsyncThread(DownloadPage *handler, wxString ipath, std::vector < stream_desc_t > streams);
	~WsyncThread();

protected:
	virtual ExitCode Entry();
	DownloadPage *handler;
	std::string mainserver, server, mainserverdir, serverdir;
	boost::filesystem::path ipath; // installation path, what we work on
	std::vector < stream_desc_t > streams;
	boost::uintmax_t predDownloadSize;
	std::map < std::string, unsigned int > traffic_stats;
	clock_t dlStartTime;
	bool dlStarted;

	// helper to construct event
	void updateCallback(int type, std::string txt = std::string(), float percent=-1);
	
	// special sync with visual event feedback
	int sync();
	int downloadFile(WSync *w, boost::filesystem::path localFile, std::string server, std::string path);
	void downloadProgress(WSync *w);

	// wsync related functions
	int getSyncData();
	void recordDataUsage();
	int findMirror(bool probeForBest=false);
	std::string formatSeconds(float seconds);

	WSync *w;
	std::map < std::string, Hashentry> hashMapLocal;
	std::map < std::string, std::map<std::string, Hashentry> > hashMapRemote; // stream hashmaps, first key is stream path

};
#endif //WTHREAD_H__

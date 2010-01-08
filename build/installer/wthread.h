#ifndef WTHREAD_H__
#define WTHREAD_H__

#include <wx/thread.h>
#include <wx/event.h>
#include "wsync.h"
#include <string.h>
#include "Timer.h"
#include "ConfigManager.h" //only use for stream_desc_t, dont use functions, as its not thread safe!

enum { CMD_THREAD_DONE };

class DownloadPage;

class WsyncThread : public wxThread
{
public:
	WsyncThread(bool debug, DownloadPage *handler, wxString ipath, std::vector < stream_desc_t > streams);
	~WsyncThread();

protected:
	virtual ExitCode Entry();
	DownloadPage *handler;
	std::string mainserver, server, mainserverdir, serverdir;
	boost::filesystem::path ipath; // installation path, what we work on
	std::vector < stream_desc_t > streams;
	boost::uintmax_t predDownloadSize;
	std::map < std::string, unsigned int > traffic_stats;
	Timer dlStartTime;
	bool dlStarted;
	FILE *df;
	void dprintf(char* fmt, ...);


	// helper to construct event
	void updateCallback(int type, std::string txt = std::string(), float percent=-1);

	// special sync with visual event feedback
	int sync();
	int downloadFile(WSync *w, boost::filesystem::path localFile, std::string server, std::string path);
	int buildFileIndex(WSync *w, boost::filesystem::path &outfilename, boost::filesystem::path &path, boost::filesystem::path &rootpath, std::map<std::string, Hashentry> &hashMap, bool writeFile, int mode);
	void downloadProgress(WSync *w);
	void debugOutputHashMap(std::map<std::string, Hashentry> &hashMap);

	// wsync related functions
	int getSyncData();
	void recordDataUsage();
	int findMirror(bool probeForBest=false);
	std::string formatSeconds(float seconds);
	void tryRemoveFile(boost::filesystem::path filename);

	WSync *w;
	std::map < std::string, Hashentry> hashMapLocal;
	std::map < std::string, std::map<std::string, Hashentry> > hashMapRemote; // stream hashmaps, first key is stream path

};
#endif //WTHREAD_H__

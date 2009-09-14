#ifndef WTHREAD_H__
#define WTHREAD_H__

#include <wx/thread.h>
#include <wx/event.h>
#include "wsync.h"
#include <string.h>

enum { CMD_THREAD_DONE };

class DownloadPage;

class WsyncThread : public wxThread
{
public:
	WsyncThread(DownloadPage *handler, wxString ipath, wxString url, wxString rpath, bool del);
	~WsyncThread();

protected:
	virtual ExitCode Entry();
	DownloadPage *handler;
	wxString ipath, url, rpath;
	bool del;

	// helper to construct event
	void updateCallback(int type, std::string txt = std::string(), float percent1=-1, float percent2=-1);
	
	// special sync with visual event feedback
	int sync(boost::filesystem::path localDir, std::string server, std::string remoteDir, bool useMirror, bool deleteOk);
};
#endif //WTHREAD_H__

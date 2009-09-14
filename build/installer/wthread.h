#ifndef WTHREAD_H__
#define WTHREAD_H__

#include <wx/thread.h>
#include <wx/event.h>

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
};
#endif //WTHREAD_H__

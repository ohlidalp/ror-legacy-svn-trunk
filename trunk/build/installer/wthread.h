#ifndef WTHREAD_H__
#define WTHREAD_H__

#include <wx/thread.h>
#include <wx/event.h>

class DownloadPage;

DECLARE_EVENT_TYPE ( WsyncThreadUpdate, -1 )
DECLARE_EVENT_TYPE ( WsyncThreadDone, -1 )


class WsyncThread : public wxThread
{
public:
	WsyncThread(DownloadPage *handler) : wxThread(wxTHREAD_DETACHED), m_pHandler(handler)
	{
	}

	~WsyncThread();


	wxThreadError Create(wxString _url, wxString _rpath);

protected:
	virtual ExitCode Entry();
	DownloadPage *m_pHandler;
	wxString url, rpath;
};
#endif //WTHREAD_H__

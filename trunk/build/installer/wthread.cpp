
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#endif //WIN32

#include "wthread.h"
#include "wsync.h"
#include "wizard.h"
#include "cevent.h"

WsyncThread::WsyncThread(DownloadPage *_handler, wxString _ipath, wxString _url, wxString _rpath, bool _del) : wxThread(wxTHREAD_DETACHED), handler(_handler), ipath(_ipath), url(_url), rpath(_rpath), del(_del)
{
	// initialization
}


WsyncThread::~WsyncThread()
{
	wxCriticalSectionLocker enter(handler->m_pThreadCS);

	// the thread is being destroyed; make sure not to leave dangling pointers around
	handler->m_pThread = NULL;
}


WsyncThread::ExitCode WsyncThread::Entry()
{
	//WSync *w = new WSync();
	//w->sync(conv(ipath), "wsync.rigsofrods.com", conv(rpath), true, del);
	//delete w;

	// send event
	MyStatusEvent ev( MyStatusCommandEvent, SE_STARTING);
	ev.SetText(wxT("This is a Foo_DoFirstThing event"));
	handler->AddPendingEvent(ev);

	return (WsyncThread::ExitCode)0;     // success
}

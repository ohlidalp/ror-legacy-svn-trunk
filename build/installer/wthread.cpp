
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#endif //WIN32

#include "wthread.h"
#include "wizard.h"


DEFINE_EVENT_TYPE(WsyncThreadUpdate)
DEFINE_EVENT_TYPE(WsyncThreadDone)

//TODO: add custom event that transmit a status message and some progress bar values

WsyncThread::ExitCode WsyncThread::Entry()
{
	while (!TestDestroy())
	{
		// ... do a bit of work...

		// TODO: put actual wsync code in here


		wxCommandEvent ev( WsyncThreadUpdate, 1 ); 
		wxPostEvent(m_pHandler, ev);
	}

	// signal the event handler that this thread is going to be destroyed
	// NOTE: here we assume that using the m_pHandler pointer is safe,
	//       (in this case this is assured by the MyFrame destructor)
	//wxQueueEvent(m_pHandler, new wxThreadEvent(wxEVT_COMMAND_MYTHREAD_COMPLETED));

	return (WsyncThread::ExitCode)0;     // success
}
wxThreadError WsyncThread::Create(wxString _url, wxString _rpath)
{
	url = _url;
	rpath = _rpath;
	return wxThread::Create();
}

WsyncThread::~WsyncThread()
{
	wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);

	// the thread is being destroyed; make sure not to leave dangling pointers around
	m_pHandler->m_pThread = NULL;
}

#ifndef THREADPOOL_H__
#define THREADPOOL_H__
// wx stuff
#include <wx/frame.h>
#include <wx/thread.h>
#include <wx/menu.h>
#include <wx/app.h>

// misc stuff
#include <stdlib.h>
#include <assert.h>
#include <map>
#include <list>

// events for worker thread
DECLARE_EVENT_TYPE(wxEVT_THREAD, -1)


class WsyncJob
{
public:
	enum job_commands // list of commands that are currently implemented
	{
	  eID_THREAD_EXIT=wxID_EXIT, // thread should exit or wants to exit
	  eID_THREAD_NULL=wxID_HIGHEST+1, // dummy command
	  eID_THREAD_STARTED, // worker thread has started OK
	  eID_THREAD_JOB, // process normal job
	  eID_THREAD_JOBERR // process errorneous job after which thread likes to exit
	};

	WsyncJob();
	
	WsyncJob(job_commands cmd, const wxString& url, const wxString& file);

	job_commands getCommand();
	wxString getURL();
	wxString getFile();
protected:
	job_commands m_cmd;
	wxString m_URL;
	wxString m_File;
};

class ThreadQueue
{
public:
	enum job_priority { eHIGHEST, eHIGHER, eNORMAL, eBELOW_NORMAL, eLOW, eIDLE }; // priority classes
	
	ThreadQueue(wxEvtHandler* pParent);
	void addJob(const WsyncJob& job, const job_priority& priority=eNORMAL);
	WsyncJob Pop();
	void Report(const WsyncJob::job_commands& cmd, const wxString& sArg=wxEmptyString, int iArg=0);
	size_t Stacksize();

private:
	wxEvtHandler* m_pParent;
	std::multimap<job_priority, WsyncJob> m_Jobs; // multimap to reflect prioritization: values with lower keys come first, newer values with same key are appended
	wxMutex m_MutexQueue; // protects queue access
	wxSemaphore m_QueueCount; // semaphore count reflects number of queued jobs
};

class WsyncWorkerThread : public wxThread
{
public:
	WsyncWorkerThread(ThreadQueue* pQueue, int id=0);

private:
	ThreadQueue* m_pQueue;
	int m_ID;

	virtual wxThread::ExitCode Entry();
	virtual void downloadFile(WsyncJob job);
	virtual void OnJob();
};

class WsyncDownloadManager : public wxEvtHandler
{
public:
	WsyncDownloadManager();
	~WsyncDownloadManager();
	void startThreads();
	void addURL(wxString url, wxString filename);
	void onThread(wxCommandEvent& event);
	void destroyThreads();

private:
	ThreadQueue* m_pQueue;
	std::list<int> m_Threads;
	DECLARE_DYNAMIC_CLASS(WsyncDownloadManager)
	//DECLARE_CLASS(WsyncDownloadManager);

	DECLARE_EVENT_TABLE()
};

#endif //THREADPOOL_H__
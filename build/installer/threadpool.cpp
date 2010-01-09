#include "threadpool.h"

//// WsyncJob
WsyncJob::WsyncJob() : m_cmd(eID_THREAD_NULL)
{
}
	
WsyncJob::WsyncJob(job_commands cmd, const wxString& url, const wxString& file) : m_cmd(cmd), m_URL(url), m_File(file)
{
}

WsyncJob::job_commands WsyncJob::getCommand()
{
	return m_cmd;
}

wxString WsyncJob::getURL()
{
	return m_URL;
}

wxString WsyncJob::getFile()
{
	return m_File;
}

//// ThreadQueue

ThreadQueue::ThreadQueue(wxEvtHandler* pParent) : m_pParent(pParent)
{
}
	
void ThreadQueue::addJob(const WsyncJob& job, const ThreadQueue::job_priority& priority) // push a job with given priority class onto the FIFO
{
	wxMutexLocker lock(m_MutexQueue); // lock the queue
	m_Jobs.insert(std::pair<job_priority, WsyncJob>(priority, job)); // insert the prioritized entry into the multimap
		m_QueueCount.Post(); // new job has arrived: increment semaphore counter
}

WsyncJob ThreadQueue::Pop()
{
	WsyncJob element;
	m_QueueCount.Wait(); // wait for semaphore (=queue count to become positive)
	m_MutexQueue.Lock(); // lock queue
	element=(m_Jobs.begin())->second; // get the first entry from queue (higher priority classes come first)
	m_Jobs.erase(m_Jobs.begin()); // erase it
	m_MutexQueue.Unlock(); // unlock queue
	return element; // return job entry
}

void ThreadQueue::Report(const WsyncJob::job_commands& cmd, const wxString& sArg, int iArg) // report back to parent
{
	wxCommandEvent evt(wxEVT_THREAD, cmd); // create command event object
	evt.SetString(sArg); // associate string with it
	evt.SetInt(iArg);
	m_pParent->AddPendingEvent(evt); // and add it to parent's event queue
}

size_t ThreadQueue::Stacksize() // helper function to return no of pending jobs
{
	wxMutexLocker lock(m_MutexQueue); // lock queue until the size has been read
	return m_Jobs.size();
}

//// WsyncWorkerThread
WsyncWorkerThread::WsyncWorkerThread(ThreadQueue* pQueue, int id) : m_pQueue(pQueue), m_ID(id)
{
	assert(pQueue);
	wxThread::Create();
}


wxThread::ExitCode WsyncWorkerThread::Entry()
{
	WsyncJob::job_commands iErr;
	m_pQueue->Report(WsyncJob::eID_THREAD_STARTED, wxEmptyString, m_ID); // tell main thread that worker thread has successfully started
	try
	{
		// this is the main loop: process jobs until a job handler throws
		while(true)
			OnJob();
	} catch(WsyncJob::job_commands& i)
	{
		// catch return value from error condition
		m_pQueue->Report(iErr=i, wxEmptyString, m_ID);
	} 
	return (wxThread::ExitCode)iErr; // and return exit code
}

void WsyncWorkerThread::downloadFile(WsyncJob job)
{
#if 0
	int stat = downloadFile(localfile, server_use_file, url);
	if(stat == -404 && retrycount < 2)
	{
		dprintf("  result: %d, retrycount: %d, falling back to main server\n", stat, retrycount);
		// fallback to main server (for this single file only!)
		//printf("falling back to main server.\n");
		server_use_file = mainserver;
		dir_use_file = mainserverdir;
		retrycount++;
		goto retry;
	}
	if(stat)
	{
		dprintf("  unable to create file: %s\n", itf->filename.c_str());
		//printf("\nunable to create file: %s\n", itf->filename.c_str());
	} else
	{
		string checkHash = w->generateFileHash(localfile);
		string hash_remote = w->findHashInHashmap(hashMapRemote, itf->filename);
		if(hash_remote == checkHash)
		{
			dprintf("DLFile| file hash ok\n");
			//printf(" OK                                             \n");
		} else
		{
			//printf(" OK                                             \n");
			//printf(" hash is: '%s'\n", checkHash.c_str());
			//printf(" hash should be: '%s'\n", hash_remote.c_str());
			dprintf("DLFile| file hash wrong, is: '%s', should be: '%s'\n", checkHash.c_str(), hash_remote.c_str());
			tryRemoveFile(localfile);
			if(retrycount < 2)
			{
				// fallback to main server!
				//printf(" hash wrong, falling back to main server.\n");
				//printf(" probably the mirror is not in sync yet\n");
				server_use_file = mainserver;
				dir_use_file = mainserverdir;
				retrycount++;
				goto retry;
			}
		}
	}
#endif //0
}

void WsyncWorkerThread::OnJob()
{
	WsyncJob job = m_pQueue->Pop(); // pop a job from the queue. this will block the worker thread if queue is empty
	switch(job.getCommand())
	{
	case WsyncJob::eID_THREAD_EXIT: // thread should exit
		throw WsyncJob::eID_THREAD_EXIT; // confirm exit command
	case WsyncJob::eID_THREAD_JOB: // process a standard job
		// XXX DOWNLOAD HERE!
		m_pQueue->Report(WsyncJob::eID_THREAD_JOB, wxString::Format(wxT("Job #%s done."), job.getURL().c_str()), m_ID); // report successful completion
		break;
	case WsyncJob::eID_THREAD_JOBERR: // process a job that terminates with an error
		m_pQueue->Report(WsyncJob::eID_THREAD_JOB, wxString::Format(wxT("Job #%s errorneous."), job.getURL().c_str()), m_ID);
		Sleep(1000);
		throw WsyncJob::eID_THREAD_EXIT; // report exit of worker thread
		break;
	case WsyncJob::eID_THREAD_NULL: // dummy command
	default:
		break;
	}
}

//// WsyncDownloadManager

WsyncDownloadManager::WsyncDownloadManager() : m_pQueue(NULL)
{
	m_pQueue=new ThreadQueue(this);
}
	
WsyncDownloadManager::~WsyncDownloadManager()
{
	destroyThreads();
	delete m_pQueue;
}

void WsyncDownloadManager::startThreads()
{
	int num_threads = 5;
	for(int i=0;i<num_threads;i++)
	{
		int id = m_Threads.empty()?1:m_Threads.back() + 1;
		m_Threads.push_back(id);
		WsyncWorkerThread* pThread = new WsyncWorkerThread(m_pQueue, id); // create a new worker thread, increment thread counter (this implies, thread will start OK)
		pThread->Run();
	}
}
	
void WsyncDownloadManager::addURL(wxString url, wxString filename)
{
	int iJob=rand();
	m_pQueue->addJob(WsyncJob(WsyncJob::eID_THREAD_JOB, url, filename));

	// start the threads if not already done	
	if(!m_Threads.size())
		startThreads();
}

void WsyncDownloadManager::onThread(wxCommandEvent& event) // handler for thread notifications
{
	switch(event.GetId())
	{
		case WsyncJob::eID_THREAD_JOB:
			//wxMessageBox(wxString::Format(wxT("[%i]: %s"), event.GetInt(), event.GetString()));
			break; 
		case WsyncJob::eID_THREAD_EXIT:
			//wxMessageBox(wxString::Format(wxT("[%i]: Stopped."), event.GetInt()));
			m_Threads.remove(event.GetInt()); // thread has exited: remove thread ID from list
			break;
		case WsyncJob::eID_THREAD_STARTED:
			//wxMessageBox(wxString::Format(wxT("[%i]: Ready."), event.GetInt()));
			break; 
		default:
			event.Skip();
	}
}

void WsyncDownloadManager::destroyThreads()
{
	if(m_Threads.empty()) 
		return;
	for(size_t t=0; t<m_Threads.size(); ++t)
	{
		m_pQueue->addJob(WsyncJob(WsyncJob::eID_THREAD_EXIT, wxEmptyString, wxEmptyString), ThreadQueue::eHIGHEST); // send all running threads the "EXIT" signal
	}
}

//// EVENTS
IMPLEMENT_DYNAMIC_CLASS(WsyncDownloadManager, wxEvtHandler)

DEFINE_EVENT_TYPE(wxEVT_THREAD)

//// EVENT TABLE

BEGIN_EVENT_TABLE(WsyncDownloadManager, wxEvtHandler)
  EVT_COMMAND(wxID_ANY, wxEVT_THREAD, WsyncDownloadManager::onThread)
END_EVENT_TABLE()

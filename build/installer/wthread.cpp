
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#endif //WIN32

#include "wthread.h"
#include "wizard.h"
#include "cevent.h"

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::filesystem; 
using namespace std; 

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
	sync(conv(ipath), "wsync.rigsofrods.com", conv(rpath), true, del);

	return (WsyncThread::ExitCode)0;     // success
}

void WsyncThread::updateCallback(int type, std::string txt, float percent1, float percent2)
{
	// send event
	MyStatusEvent ev(MyStatusCommandEvent, type);
	ev.setText(conv(txt));
	ev.setProgress(0, percent1);
	ev.setProgress(1, percent2);
	handler->AddPendingEvent(ev);
}

int WsyncThread::sync(boost::filesystem::path localDir, string server, string remoteDir, bool useMirror, bool deleteOk)
{
	char tmp[512] = "";
	memset(tmp, 0, 512);

	updateCallback(MSE_STARTING);
	WSync *w = new WSync();

	// download remote currrent file first
	int res = 0;
	path remoteFileIndex;
	if(WSync::getTempFilename(remoteFileIndex))
	{
		updateCallback(MSE_ERROR, "error creating tempfile!");
		delete w;
		return -1;
	}

	updateCallback(MSE_UPDATING, "downloading file list...");
	string url = "/" + remoteDir + "/" + INDEXFILENAME;
	if(w->downloadFile(remoteFileIndex.string(), server, url))
	{
		updateCallback(MSE_ERROR, "error downloading file index from " + url);
		delete w;
		return -1;
	}
	//printf("downloaded FileIndex\n");

	//printf("generating local FileIndex\n");
	path myFileIndex = localDir / INDEXFILENAME;

	std::map<string, Hashentry> hashMapLocal;
	if(w->buildFileIndex(myFileIndex, localDir, localDir, hashMapLocal, true, 1))
	{
		updateCallback(MSE_ERROR, "error while generating local FileIndex");
		delete w;
		return -1;
	}
	//printf("#1: %s\n", myFileIndex.string().c_str());
	//printf("#2: %s\n", remoteFileIndex.string().c_str());


	string hashMyFileIndex = w->generateFileHash(myFileIndex);
	string hashRemoteFileIndex = w->generateFileHash(remoteFileIndex);
	//printf("#2: CWD:%s \n", boost::filesystem::current_path().string().c_str());
	//printf("#3: %s = %s\n", myFileIndex.string().c_str(), hashMyFileIndex.c_str());
	//printf("#4: %s = %s\n", remoteFileIndex.string().c_str(), hashRemoteFileIndex.c_str());

	// remove temp file again
	remove(INDEXFILENAME);
	if(hashMyFileIndex == hashRemoteFileIndex)
	{
		updateCallback(MSE_DONE, "Files are up to date, no sync needed");
		delete w;
		return 0;
	}

	// now find out what files differ
	std::map<string, Hashentry> hashMapRemote;
	int modeNumber = 0;
	if(w->loadHashMapFromFile(remoteFileIndex, hashMapRemote, modeNumber))
	{
		updateCallback(MSE_ERROR, "error reading remote file index");
		delete w;
		return -2;
	}

	if(hashMapRemote.size() == 0)
	{
		updateCallback(MSE_ERROR, "remote file index is invalid\nConnection Problems / Server down?");
		delete w;
		return -3;
	}
	// remove that temp file as well
	remove(remoteFileIndex);

	std::vector<Fileentry> deletedFiles;
	std::vector<Fileentry> changedFiles;
	std::vector<Fileentry> newFiles;

	std::map<string, Hashentry>::iterator it;
	//first, detect deleted or changed files
	for(it = hashMapLocal.begin(); it != hashMapLocal.end(); it++)
	{
		// filter some utility files
		if(it->first == string("/update.temp.exe")) continue;
		if(it->first == string("/stream.info")) continue;
		if(it->first == string("/version")) continue;


		//if(hashMapRemote[it->first] == it->second)
			//printf("same: %s %s==%s\n", it->first.c_str(), hashMapRemote[it->first].c_str(), it->second.c_str());
		if(hashMapRemote.find(it->first) == hashMapRemote.end())
			deletedFiles.push_back(Fileentry(it->first, it->second.filesize));
		else if(hashMapRemote[it->first].hash != it->second.hash)
			changedFiles.push_back(Fileentry(it->first, hashMapRemote[it->first].filesize));
	}
	// second, detect new files
	for(it = hashMapRemote.begin(); it != hashMapRemote.end(); it++)
	{
		// filter some files
		if(it->first == string("/stream.info")) continue;
		if(it->first == string("/version")) continue;
		
		
		if(hashMapLocal.find(it->first) == hashMapLocal.end())
			newFiles.push_back(Fileentry(it->first, it->second.filesize));
	}
	// done comparing


	std::vector<Fileentry>::iterator itf;
	int changeCounter = 0, changeMax = changedFiles.size() + newFiles.size() + deletedFiles.size();
	int filesToDownload = newFiles.size() + changedFiles.size();
	int predDownloadSize = 0;
	for(itf=newFiles.begin(); itf!=newFiles.end(); itf++)
		predDownloadSize += (int)itf->filesize;
	for(itf=changedFiles.begin(); itf!=changedFiles.end(); itf++)
		predDownloadSize += (int)itf->filesize;

	// security check in order not to delete the entire harddrive
	if(deletedFiles.size() > 1000)
	{
		updateCallback(MSE_ERROR, "error: would delete more than 1000 files, aborting");
		delete w;
		return -1;
	}

	if(changeMax)
	{
		string server_use = server, dir_use = remoteDir;
		bool use_mirror = false;
		if(filesToDownload)
		{
			sprintf(tmp, "downloading %d files now (%s) ..\n", filesToDownload, WSync::formatFilesize(predDownloadSize).c_str());
			updateCallback(MSE_UPDATING, string(tmp));

			// now find a suitable mirror
			string mirror_server="", mirror_dir="", mirror_info="";
			if(useMirror)
			{
				//printf("searching for suitable mirror...\n");
				std::vector< std::vector< std::string > > list;		
				if(!w->downloadConfigFile(API_SERVER, API_MIRROR, list))
				{
					if(list.size()>0 && list[0].size() > 2)
					{
						if(list[0][0] == "failed")
						{
							//printf("failed to get mirror, using main server\n");
						} else
						{
							mirror_server = list[0][0];
							mirror_dir = list[0][1];
							mirror_info = list[0][2];
							use_mirror=true;
							//printf("using mirror server: %s, %s\n", mirror_server.c_str(), mirror_info.c_str());
						}
					}// else
						//printf("wrong API response2\n");
				}// else
				//	printf("wrong API response\n");
			}

			if(use_mirror)
			{
				server_use = mirror_server;
				dir_use = mirror_dir;
			}
		}

		// do things now!	
		if(newFiles.size())
		{
			for(itf=newFiles.begin();itf!=newFiles.end();itf++, changeCounter++)
			{
				int retrycount = 0;
// ARGHHHH GOTO D:
retry:
				//progressOutputShort(float(changeCounter)/float(changeMax));
				sprintf(tmp, "downloading new file: %s (%s) ", itf->filename.c_str(), WSync::formatFilesize(itf->filesize).c_str());
				updateCallback(MSE_UPDATING, string(tmp), float(changeCounter)/float(changeMax), float(w->getDownloadSize())/float(predDownloadSize));
				path localfile = localDir / itf->filename;
				string url = "/" + dir_use + "/" + itf->filename;
				int stat = w->downloadFile(localfile, server_use, url, true);
				if(stat == -404 && retrycount < 2)
				{
					// fallback to main server!
					//printf("falling back to main server.\n");
					server_use = server;
					dir_use = remoteDir;
					retrycount++;
					goto retry;
				}
				if(stat)
				{
					//printf("\nunable to create file: %s\n", itf->filename.c_str());
				} else
				{
					string checkHash = w->generateFileHash(localfile);
					string hash_remote = w->findHashInHashmap(hashMapRemote, itf->filename);
					if(hash_remote == checkHash)
					{
						//printf(" OK                                             \n");
					} else
					{
						//printf(" OK                                             \n");
						//printf(" hash is: '%s'\n", checkHash.c_str());
						//printf(" hash should be: '%s'\n", hash_remote.c_str());
						remove(localfile);
						if(retrycount < 2)
						{
							// fallback to main server!
							//printf(" hash wrong, falling back to main server.\n");
							//printf(" probably the mirror is not in sync yet\n");
							server_use = server;
							dir_use = remoteDir;
							retrycount++;
							goto retry;
						}
					}
				}
			}
		}

		if(changedFiles.size())
		{
			for(itf=changedFiles.begin();itf!=changedFiles.end();itf++, changeCounter++)
			{
				int retrycount = 0;
// ARGHHHH GOTO D:
retry2:
				//progressOutputShort(float(changeCounter)/float(changeMax));
				sprintf(tmp, "updating changed file: %s (%s) ", itf->filename.c_str(), WSync::formatFilesize(itf->filesize).c_str());
				updateCallback(MSE_UPDATING, string(tmp), float(changeCounter)/float(changeMax), float(w->getDownloadSize())/float(predDownloadSize));
				path localfile = localDir / itf->filename;
				string url = "/" + dir_use + "/" + itf->filename;
				int stat = w->downloadFile(localfile, server_use, url, true);
				if(stat == -404 && retrycount < 2)
				{
					// fallback to main server!
					//printf("falling back to main server.\n");
					server_use = server;
					dir_use = remoteDir;
					retrycount++;
					goto retry2;
				}
				if(stat)
				{
					//printf("\nunable to update file: %s\n", itf->filename.c_str());
				} else
				{
					string checkHash = w->generateFileHash(localfile);
					string hash_remote = w->findHashInHashmap(hashMapRemote, itf->filename);
					if(hash_remote == checkHash)
					{
						//printf(" OK                                             \n");
					} else
					{
						//printf(" OK                                             \n");
						//printf(" hash is: '%s'\n", checkHash.c_str());
						//printf(" hash should be: '%s'\n", hash_remote.c_str());
						remove(localfile);
						if(retrycount < 2)
						{
							// fallback to main server!
							//printf(" hash wrong, falling back to main server.\n");
							//printf(" probably the mirror is not in sync yet\n");
							server_use = server;
							dir_use = remoteDir;
							retrycount++;
							goto retry2;
						}
					}
				}
			}
		}
		
		if(deleteOk && deletedFiles.size() && !(modeNumber & WMO_NODELETE))
		{
			for(itf=deletedFiles.begin();itf!=deletedFiles.end();itf++, changeCounter++)
			{
				//progressOutputShort(float(changeCounter)/float(changeMax));
				sprintf(tmp, "deleting file: %s (%s)\n", itf->filename.c_str(), WSync::formatFilesize(itf->filesize).c_str());
				updateCallback(MSE_UPDATING, string(tmp), float(changeCounter)/float(changeMax), float(w->getDownloadSize())/float(predDownloadSize));
				path localfile = localDir / itf->filename;
				try
				{
					boost::filesystem::remove(localfile);
					//if(exists(localfile))
					//	printf("unable to delete file: %s\n", localfile.string().c_str());
				} catch(...)
				{
					//printf("unable to delete file: %s\n", localfile.string().c_str());
				}
			}
		} else if (!deleteOk && deletedFiles.size())
		{
			//printf("wont delete any files during this run\n");
		}

		sprintf(tmp, "sync complete, downloaded %s\n", WSync::formatFilesize(w->getDownloadSize()).c_str());
		res = 1;
	} else
	{
		sprintf(tmp, "sync complete (already up to date), downloaded %s\n", WSync::formatFilesize(w->getDownloadSize()).c_str());
	}

	delete w;
	//remove temp files again
	remove(remoteFileIndex);
	updateCallback(MSE_DONE, string(tmp), 1, 1);
	return res;
}
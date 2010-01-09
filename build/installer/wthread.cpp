#include "wthread.h"
#include "wizard.h"
#include "cevent.h"
#include "SHA1.h"
#include "installerlog.h"
#include "utils.h"
#include "wsyncdownload.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#include <windows.h>
	#include <conio.h> // for getch
#endif // OGRE_PLATFORM

#include "Timer.h"
#include <boost/algorithm/string.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::filesystem;
using namespace std;

WsyncThread::WsyncThread(wxEvtHandler *parent, wxString _ipath, std::vector < stream_desc_t > _streams) : \
	wxThread(wxTHREAD_DETACHED), 
	parent(parent), 
	ipath(conv(_ipath)), 
	streams(_streams),
	predDownloadSize(0),
	dlm(new WsyncDownloadManager())
{
	// main server
	mainserver = server = "wsync.rigsofrods.com";
	mainserverdir = serverdir = "/";
}

WsyncThread::~WsyncThread()
{
}

WsyncThread::ExitCode WsyncThread::Entry()
{
	// update path target
	updateCallback(MSE_UPDATE_PATH, ipath.string());

	getSyncData();
	sync();
	recordDataUsage();

	return (WsyncThread::ExitCode)0;     // success
}

void WsyncThread::updateCallback(int type, std::string txt, float percent)
{
	// send event
	MyStatusEvent ev(MyStatusCommandEvent, type);
	ev.text = conv(txt);
	ev.progress = percent;
	parent->AddPendingEvent(ev);
}

void WsyncThread::listFiles(const path &dir_path, std::vector<std::string> &files)
{
	if (!exists(dir_path))
		return;
	directory_iterator end_itr; // default construction yields past-the-end
	for (directory_iterator itr(dir_path); itr != end_itr; ++itr)
	{
		if (is_directory(itr->status()))
		{
			listFiles(itr->path(), files);
			continue;
		}
		string entry = itr->path().string();
		files.push_back(entry);
	}
}

int WsyncThread::buildFileIndex(boost::filesystem::path &outfilename, boost::filesystem::path &path, boost::filesystem::path &rootpath, std::map<string, Hashentry> &hashMap, bool writeFile, int mode)
{
	vector<string> files;
	listFiles(path, files);
	char tmp[256] = "";
	sprintf(tmp, "indexing %d files ...", files.size());
	LOG("%s\n", tmp);
	updateCallback(MSE_STARTING, string(tmp));
	int counter = 0, counterMax = files.size();
	for(vector<string>::iterator it=files.begin(); it!=files.end(); it++, counter++)
	{
		updateCallback(MSE_UPDATE_PROGRESS, "", float(counter)/float(counterMax));
		// cut out root path
		string respath = *it;
		if(respath.substr(0, rootpath.string().size()) == rootpath.string())
		{
			// this ensures that all paths start with /
			int start = rootpath.string().size();
			if(respath.substr(rootpath.string().size()-1,1) == "/")
				start -= 1;
			respath = respath.substr(start);
		}

		// do not use the file index in itself!
		if(respath == (string("/") + string(INDEXFILENAME)))
			continue;

		size_t found = respath.find(".svn");
		if (found != string::npos)
			continue;

		found = respath.find(".temp.");
		if (found != string::npos)
			continue;

		string resultHash = generateFileHash(it->c_str());

		Hashentry entry(resultHash, file_size(*it));
		hashMap[respath] = entry;
	}
	if(writeFile)
		return saveHashMapToFile(outfilename, hashMap, mode);
	return 0;
}

int WsyncThread::saveHashMapToFile(boost::filesystem::path &filename, std::map<std::string, Hashentry> &hashMap, int mode)
{
	ensurePathExist(filename);
	const char *cfile = filename.string().c_str();
	FILE *f = fopen(cfile, "wb");
	if(!f)
	{
		printf("error opening file: %s\n", cfile);
		return -1;
	}

	for(std::map<string, Hashentry>::iterator it=hashMap.begin(); it!=hashMap.end(); it++)
	{
		int fsize = (int)it->second.filesize;
		fprintf(f, "%s : %d : %s\n", it->first.c_str(), fsize, it->second.hash.c_str());
	}
	// add custom keys
	fprintf(f, "|MODE : %d : 0\n", mode);
	fclose(f);
	return 0;
}

std::string WsyncThread::generateFileHash(boost::filesystem::path file)
{
	CSHA1 sha1;
	bool res = sha1.HashFile(const_cast<char*>(file.string().c_str()));
	if(!res) return string("");
	sha1.Final();
	char resultHash[256] = "";
	sha1.ReportHash(resultHash, CSHA1::REPORT_HEX_SHORT);
	return string(resultHash);
}

void WsyncThread::debugOutputHashMap(std::map<string, Hashentry> &hashMap)
{
	for(std::map<string, Hashentry>::iterator it=hashMap.begin(); it!=hashMap.end(); it++)
	{
		int fsize = (int)it->second.filesize;
		LOG(" - %s : %d : %s\n", it->first.c_str(), fsize, it->second.hash.c_str());
	}
}

int WsyncThread::getSyncData()
{
	// generating local FileIndex
	path myFileIndex = ipath / INDEXFILENAME;

	hashMapLocal.clear();
	if(buildFileIndex(myFileIndex, ipath, ipath, hashMapLocal, true, 1))
	{
		LOG("error while generating local FileIndex\n");
		updateCallback(MSE_ERROR, "error while generating local FileIndex");
		return -1;
	}
	LOG("local hashmap building done:\n");
	debugOutputHashMap(hashMapLocal);

	// now fetch remote file-indexes
	for(std::vector < stream_desc_t >::iterator it = streams.begin(); it!=streams.end(); it++)
	{
		// only use selected streams
		if(!it->checked) continue;

		path remoteFileIndex;
		if(getTempFilename(remoteFileIndex))
		{
			LOG("error creating tempfile\n");
			updateCallback(MSE_ERROR, "error creating tempfile!");
			return -1;
		}

		updateCallback(MSE_UPDATE_TEXT, "downloading file list ...");
		LOG("downloading file list to file %s ...\n", remoteFileIndex.string().c_str());
		string url = "/" + conv(it->path) + "/" + INDEXFILENAME;
		//if(downloadFile(remoteFileIndex.string(), mainserver, url))
		{
			updateCallback(MSE_ERROR, "error downloading file index from http://" + mainserver + url);
			LOG("error downloading file index from http://%s%s\n", mainserver.c_str(), url.c_str());
			return -1;
		}

		/*
		// this is not usable with overlaying streams anymore :(
		string hashMyFileIndex = generateFileHash(myFileIndex);
		string hashRemoteFileIndex = generateFileHash(remoteFileIndex);

		tryRemoveFile(INDEXFILENAME);
		if(hashMyFileIndex == hashRemoteFileIndex)
		{
			updateCallback(MSE_DONE, "Files are up to date, no sync needed");
			delete w;
			return 0;
		}
		*/

		// now read in the remote index
		std::map<string, Hashentry> temp_hashMapRemote;
		int modeNumber = 0; // pseudo, not used, to be improved
		int res = loadHashMapFromFile(remoteFileIndex, temp_hashMapRemote, modeNumber);
		// remove that temp file
		WsyncDownload::tryRemoveFile(remoteFileIndex);
		if(res)
		{
			updateCallback(MSE_ERROR, "error reading remote file index");
			LOG("error reading remote file index\n");
			return -2;
		}

		if(temp_hashMapRemote.size() == 0)
		{
			updateCallback(MSE_ERROR, "remote file index is invalid\nConnection Problems / Server down?");
			LOG("remote file index is invalid\nConnection Problems / Server down?\n");
			return -3;
		}
		LOG("remote hashmap reading done [%s]:\n", conv(it->path).c_str());
		debugOutputHashMap(temp_hashMapRemote);

		// add it to our map
		hashMapRemote[conv(it->path)] = temp_hashMapRemote;
	}
	LOG("all sync data received\n");
	return 0;
}



int WsyncThread::loadHashMapFromFile(boost::filesystem::path &filename, std::map<std::string, Hashentry> &hashMap, int &mode)
{
	FILE *f = fopen(filename.string().c_str(), "r");
	if (!f)
	{
		printf("error opening file '%s'", filename.string().c_str());
		return -1;
	}
	while(!feof(f))
	{
		char file[2048]="";
		char filehash[256]="";
		int scansize=0;
		boost::uintmax_t filesize = 0;
		int res = fscanf(f, "%s : %d : %s\n", file, &scansize, filehash);
		if(res < 2)
			continue;
		filesize = scansize;
		if(file[0] == '|')
		{
			// its actually an option
			if(!strncmp(file, "|MODE", 5))
				mode = (int)filesize;
			continue;
		}
		Hashentry entry(filehash, filesize);
		hashMap[file] = entry;
	}
	fclose (f);
	return 0;
}

int WsyncThread::sync()
{
	int res=0;
	char tmp[512] = "";
	memset(tmp, 0, 512);

	LOG("==== starting sync\n");

	std::vector<Fileentry> deletedFiles;
	std::vector<Fileentry> changedFiles;
	std::vector<Fileentry> newFiles;

	std::map<string, Hashentry>::iterator it;
	std::map<string, std::map<string, Hashentry> >::iterator itr;

	std::string deletedKeyword = "._deleted_";
	// walk all remote hashmaps
	for(itr = hashMapRemote.begin(); itr != hashMapRemote.end(); itr++)
	{
		//first, detect deleted or changed files
		LOG("* detecting deleted or changed files for stream %s...\n", itr->first.c_str());
		for(it = hashMapLocal.begin(); it != hashMapLocal.end(); it++)
		{
			// filter some utility files
			if(it->first == string("/update.temp.exe")) continue;
			if(it->first == string("/stream.info")) continue;
			if(it->first == string("/version")) continue;

			if(it->first.find(deletedKeyword) != string::npos)
			{
				// check if the file is deleted already
				std::string realfn = it->first.substr(0, it->first.size() - deletedKeyword.size());
				path localfile = ipath / realfn;
				if(exists(localfile))
					deletedFiles.push_back(Fileentry(itr->first, it->first, it->second.filesize));
				continue;
			}

			//if(hashMapRemote[it->first] == it->second)
				//printf("same: %s %s==%s\n", it->first.c_str(), hashMapRemote[it->first].c_str(), it->second.c_str());

			// old deletion method
			if(itr->second.find(it->first) == itr->second.end())
				// this file is not in this stream, ignore it for now
				continue;
			else if(itr->second[it->first].hash != it->second.hash)
				changedFiles.push_back(Fileentry(itr->first, it->first, itr->second[it->first].filesize));
		}
		// second, detect new files
		LOG("* detecting new files for stream %s...\n", itr->first.c_str());
		for(it = itr->second.begin(); it != itr->second.end(); it++)
		{
			// filter some files
			if(it->first == string("/stream.info")) continue;
			if(it->first == string("/version")) continue;

			// add deleted files if they still exist
			if(it->first.find(deletedKeyword) != string::npos)
			{
				// check if the file is deleted already
				std::string realfn = it->first.substr(0, it->first.size() - deletedKeyword.size());
				path localfile = ipath / realfn;
				if(exists(localfile))
					deletedFiles.push_back(Fileentry(itr->first, it->first, it->second.filesize));
				continue;
			}

			if(hashMapLocal.find(it->first) == hashMapLocal.end())
				newFiles.push_back(Fileentry(itr->first, it->first, it->second.filesize));
		}
	}

	// rename the installer if required:
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	//specific things to rename the installer on the fly in order to allow its update
	bool updateInstaller = false;
	for(std::vector<Fileentry>::iterator itf=changedFiles.begin();itf!=changedFiles.end();itf++)
	{
		if(itf->filename == "/installer.exe")
		{
			LOG("* will be updating the installer\n");
			updateInstaller=true;
			break;
		}
	}
	if(updateInstaller)
	{
		path installer_from = ipath / "installer.exe";
		path installer_to = ipath / "installer.exe.old";
		if(exists(installer_from))
		{
			try
			{
				if(boost::filesystem::exists(installer_to))
				{
					// we dont need the old installer, remove it.
					WsyncDownload::tryRemoveFile(installer_to);
				}
				LOG("trying to rename installer from %s to %s\n", installer_from.string().c_str(), installer_to.string().c_str());
				boost::filesystem::rename(installer_from, installer_to);
			} catch (std::exception& e)
			{
				LOG("error while renaming installer: %s\n", std::string(e.what()).c_str());
			}
		}
	}
#endif // OGRE_PLATFORM

	// done comparing, now summarize the changes
	std::vector<Fileentry>::iterator itf;
	int changeCounter = 0, changeMax = changedFiles.size() + newFiles.size() + deletedFiles.size();
	int filesToDownload = newFiles.size() + changedFiles.size();

	LOG("==== Changes:\n");
	predDownloadSize = 0; // predicted download size	
	for(itf=newFiles.begin(); itf!=newFiles.end(); itf++)
	{
		predDownloadSize += (int)itf->filesize;
		LOG("> A path:%s, file: %s, size:%d\n", itf->stream_path.c_str(), itf->filename.c_str(), itf->filesize);
	}
	if(!newFiles.size()) LOG("> no files added");


	for(itf=changedFiles.begin(); itf!=changedFiles.end(); itf++)
	{
		predDownloadSize += (int)itf->filesize;
		LOG("> U path:%s, file: %s, size:%d\n", itf->stream_path.c_str(), itf->filename.c_str(), itf->filesize);
	}
	if(!changedFiles.size()) LOG("> no files changed");

	for(itf=deletedFiles.begin(); itf!=deletedFiles.end(); itf++)
	{
		LOG("> D path:%s, file: %s, size:%d\n", itf->stream_path.c_str(), itf->filename.c_str(), itf->filesize);
	}
	if(!deletedFiles.size()) LOG("> no files deleted");
	

	if(predDownloadSize > 0)
	{
		LOG("==== finding mirror\n");
		bool measureMirrorSpeed = (predDownloadSize > 10485760); // 10 MB
		if(findMirror(measureMirrorSpeed) && measureMirrorSpeed)
		{
			// try to find a mirror without measuring the speed
			findMirror();
		}
	}
	// reset progress bar
	updateCallback(MSE_UPDATE_PROGRESS, "", 0);

	LOG("==== starting download\n");

	// security check in order not to delete the entire harddrive
	if(deletedFiles.size() > 1000)
	{
		LOG("would delete more than 1000 files, aborting\n");
		updateCallback(MSE_ERROR, "would delete more than 1000 files, aborting");
		return -1;
	}

	// if there are files to be updated
	if(changeMax)
	{
		string server_use = server, dir_use = serverdir;

		// do things now!
		if(newFiles.size())
		{
			for(itf=newFiles.begin();itf!=newFiles.end();itf++, changeCounter++)
			{
				int retrycount = 0;
// ARGHHHH GOTO D:
				string server_use_file = server_use, dir_use_file = dir_use;
retry:
				updateCallback(MSE_UPDATE_SERVER, server_use_file);
				//progressOutputShort(float(changeCounter)/float(changeMax));
				sprintf(tmp, "downloading new file: %s (%s) ", itf->filename.c_str(), formatFilesize(itf->filesize).c_str());
				updateCallback(MSE_UPDATE_TEXT, string(tmp));
				LOG("%s\n", tmp);
				path localfile = ipath / itf->filename;
				string url = dir_use_file + itf->stream_path + itf->filename;
				int stat = 0; //downloadFile(localfile, server_use_file, url);
				if(stat == -404 && retrycount < 2)
				{
					LOG("  result: %d, retrycount: %d, falling back to main server\n", stat, retrycount);
					// fallback to main server (for this single file only!)
					//printf("falling back to main server.\n");
					server_use_file = mainserver;
					dir_use_file = mainserverdir;
					retrycount++;
					goto retry;
				}
				if(stat)
				{
					LOG("  unable to create file: %s\n", itf->filename.c_str());
					//printf("\nunable to create file: %s\n", itf->filename.c_str());
				} else
				{
					string checkHash = generateFileHash(localfile);
					string hash_remote = findHashInHashmap(hashMapRemote, itf->filename);
					if(hash_remote == checkHash)
					{
						LOG("DLFile| file hash ok\n");
						//printf(" OK                                             \n");
					} else
					{
						//printf(" OK                                             \n");
						//printf(" hash is: '%s'\n", checkHash.c_str());
						//printf(" hash should be: '%s'\n", hash_remote.c_str());
						LOG("DLFile| file hash wrong, is: '%s', should be: '%s'\n", checkHash.c_str(), hash_remote.c_str());
						WsyncDownload::tryRemoveFile(localfile);
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
			}
		}

		if(changedFiles.size())
		{
			for(itf=changedFiles.begin();itf!=changedFiles.end();itf++, changeCounter++)
			{
				int retrycount = 0;
// ARGHHHH GOTO D:
				string server_use_file = server_use, dir_use_file = dir_use;
retry2:
				updateCallback(MSE_UPDATE_SERVER, server_use_file);
				//progressOutputShort(float(changeCounter)/float(changeMax));
				sprintf(tmp, "updating changed file: %s (%s) ", itf->filename.c_str(), formatFilesize(itf->filesize).c_str());
				updateCallback(MSE_UPDATE_TEXT, string(tmp));
				LOG("%s\n", tmp);
				path localfile = ipath / itf->filename;
				string url = dir_use_file + itf->stream_path + itf->filename;
				int stat = 0;//downloadFile(localfile, server_use_file, url);
				if(stat == -404 && retrycount < 2)
				{
					// fallback to main server (for this file only)
					//printf("falling back to main server.\n");
					server_use_file = mainserver;
					dir_use_file = mainserverdir;
					retrycount++;
					goto retry2;
				}
				if(stat)
				{
					LOG("  unable to update file: %s\n", itf->filename.c_str());
					//printf("\nunable to update file: %s\n", itf->filename.c_str());
				} else
				{
					string checkHash = generateFileHash(localfile);
					string hash_remote = findHashInHashmap(hashMapRemote, itf->filename);
					if(hash_remote == checkHash)
					{
						LOG("DLFile| file hash ok\n");
						//printf(" OK                                             \n");
					} else
					{
						//printf(" OK                                             \n");
						//printf(" hash is: '%s'\n", checkHash.c_str());
						//printf(" hash should be: '%s'\n", hash_remote.c_str());
						LOG("DLFile| file hash wrong, is: '%s', should be: '%s'\n", checkHash.c_str(), hash_remote.c_str());
						WsyncDownload::tryRemoveFile(localfile);
						if(retrycount < 2)
						{
							// fallback to main server!
							//printf(" hash wrong, falling back to main server.\n");
							//printf(" probably the mirror is not in sync yet\n");
							server_use_file = mainserver;
							dir_use_file = mainserverdir;
							retrycount++;
							goto retry2;
						}
					}
				}
			}
		}

		if(deletedFiles.size())
		{
			for(itf=deletedFiles.begin();itf!=deletedFiles.end();itf++, changeCounter++)
			{
				string filename = itf->filename;
				if(filename.find(deletedKeyword) != string::npos)
				{
					// check if the file is deleted already
					filename = filename.substr(0, filename.size() - deletedKeyword.size());
				}

				//progressOutputShort(float(changeCounter)/float(changeMax));
				sprintf(tmp, "deleting file: %s\n", filename.c_str()); //, formatFilesize(itf->filesize).c_str());
				updateCallback(MSE_UPDATE_TEXT, string(tmp));
				LOG("%s\n", tmp);
				path localfile = ipath / filename;
				try
				{
					WsyncDownload::tryRemoveFile(localfile);
					//if(exists(localfile))
					//	printf("unable to delete file: %s\n", localfile.string().c_str());
				} catch(...)
				{
					//printf("unable to delete file: %s\n", localfile.string().c_str());
				}
			}
		}
		//sprintf(tmp, "sync complete, downloaded %s\n", formatFilesize(getDownloadSize()).c_str());
		res = 1;
	} else
	{
		sprintf(tmp, "sync complete (already up to date)\n");
	}

	// update progress for the last time
	//downloadProgress();
	// user may proceed
	updateCallback(MSE_DONE, string(tmp), 1);
	LOG("%s\n", tmp);

	return res;
}

std::string WsyncThread::findHashInHashmap(std::map<std::string, Hashentry> hashMap, std::string filename)
{
	std::map<string, Hashentry>::iterator it;
	for(it = hashMap.begin(); it != hashMap.end(); it++)
	{
		if(it->first == filename)
			return it->second.hash;
	}
	return "";
}

std::string WsyncThread::findHashInHashmap(std::map < std::string, std::map < std::string, Hashentry > > hashMap, std::string filename)
{
	std::map<string, Hashentry>::iterator it;
	std::map<std::string, std::map<std::string, Hashentry > >::iterator itm;
	for(itm = hashMap.begin(); itm != hashMap.end(); itm++)
	{
		for(it = itm->second.begin(); it != itm->second.end(); it++)
		{
			if(it->first == filename)
				return it->second.hash;
		}
	}
	return "";
}

void WsyncThread::recordDataUsage()
{
	LOG("==== reporting used up bandwidth\n");
	if(traffic_stats.size() == 0) return;
	std::map < std::string, unsigned int >::iterator itt;
	for(itt=traffic_stats.begin();itt!=traffic_stats.end(); itt++)
	{
		std::vector< std::vector< std::string > > list;
		char tmp[256]="";
		sprintf(tmp, API_RECORDTRAFFIC, itt->first.c_str(), itt->second);
		//responseLessRequest(API_SERVER, string(tmp));
		
		string sizeStr = formatFilesize(itt->second);
		LOG("%s : %d bytes / %s\n", itt->first.c_str(), itt->second, sizeStr.c_str());
	}
}

int WsyncThread::findMirror(bool probeForBest)
{
	std::vector< std::vector< std::string > > list;
	updateCallback(MSE_STARTING, "finding suitable mirror ...");
	LOG("finding suitable mirror ...\n");
	if(!probeForBest)
	{
		updateCallback(MSE_STARTING, "getting random mirror ...");
		LOG("getting random mirror ...\n");
		// just collect a best fitting server by geolocating this client's IP
		WsyncDownload *wsdl = new WsyncDownload();
		int res = wsdl->downloadConfigFile(API_SERVER, API_MIRROR, list);
		delete(wsdl);
		if(!res)
		{
			if(list.size()>0 && list[0].size() > 2)
			{
				if(list[0][0] == "failed")
				{
					//printf("failed to get mirror, using main server\n");
					return -1;
				} else
				{
					server = list[0][0];
					serverdir = list[0][1];
				}
			}
		}
		string stxt = server + serverdir + " (random)";
		updateCallback(MSE_UPDATE_SERVER, stxt);
		return 0;
	} else
	{
		// probe servers :D
		// get some random servers and test their speeds
		LOG("getting fastest mirror ...\n");
		WsyncDownload *wsdl = new WsyncDownload();
		int res = wsdl->downloadConfigFile(API_SERVER, API_MIRROR_NOGEO, list);
		delete(wsdl);

		if(!res)
		{
			if(list.size() > 0)
			{
				if(list[0][0] == "failed")
					return -1;

				double bestTime = 99999;
				int bestServer = -1;
				for(int i=0; i<(int)list.size(); i++)
				{
					updateCallback(MSE_STARTING, "testing speed of mirror: " + list[i][0]);
					updateCallback(MSE_UPDATE_PROGRESS, "", float(i)/float(list.size()));

					double tdiff = measureDownloadSpeed(list[i][0], list[i][1]+"../speedtest.bin");
					if(tdiff < 0)
					{
						// error, skip this server
						//LOG("mirror speed: % 30s : error: %s\n", list[i][0].c_str(), w->getLastError().c_str());
						continue;
					}
					if(tdiff >=0 && tdiff < bestTime)
					{
						bestTime = tdiff;
						bestServer = i;
					}

					char tmp[255]="";
					sprintf(tmp, "%6.2f: kB/s", (10240.0f / tdiff) / 1024.0f);
					updateCallback(MSE_STARTING, "speed of " + list[i][0] + std::string(tmp));
					LOG("mirror speed: % 30s : %s\n", list[i][0].c_str(), tmp);
				}
				if(bestServer != -1)
				{
					server = list[bestServer][0];
					serverdir = list[bestServer][1];
					string stxt = server + serverdir + " (fastest)";
					updateCallback(MSE_UPDATE_SERVER, stxt);
				} else
					return -1;
			}
		}

	}
	return 0;
}

double WsyncThread::measureDownloadSpeed(std::string server, std::string url)
{
	path tempfile;
	if(getTempFilename(tempfile))
	{
		printf("error creating tempfile!\n");
		return -1;
	}

	int filesize=0;
	Timer timer = Timer();
	//if(downloadFile(tempfile, server, url, &filesize))
	{
		return -2;
	}
	double tdiff = timer.elapsed();
	printf("mirror speed: %s : %dkB in %0.2f seconds = %0.2f kB/s\n", server.c_str(), (int)(filesize/1024.0f), tdiff, (filesize/1024.0f)/(float)tdiff);
	WsyncDownload::tryRemoveFile(tempfile);
	return tdiff;
}

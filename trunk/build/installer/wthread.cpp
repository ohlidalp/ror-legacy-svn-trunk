
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

WsyncThread::WsyncThread(DownloadPage *_handler, wxString _ipath, std::vector < stream_desc_t > _streams) : wxThread(wxTHREAD_DETACHED), handler(_handler), ipath(conv(_ipath)), streams(_streams), predDownloadSize(0), dlStartTime(std::time(0)), dlStarted(false), w(new WSync())
{
	// main server
	mainserver = server = "wsync.rigsofrods.com";
	mainserverdir = serverdir = "/";
}

WsyncThread::~WsyncThread()
{
	if(w) delete w;
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
	handler->AddPendingEvent(ev);
}

int WsyncThread::getSyncData()
{
	// generating local FileIndex
	path myFileIndex = ipath / INDEXFILENAME;
	updateCallback(MSE_STARTING, "indexing files ...");

	hashMapLocal.clear();
	if(w->buildFileIndex(myFileIndex, ipath, ipath, hashMapLocal, true, 1))
	{
		updateCallback(MSE_ERROR, "error while generating local FileIndex");
		return -1;
	}

	// now fetch remote file-indexes
	for(std::vector < stream_desc_t >::iterator it = streams.begin(); it!=streams.end(); it++)
	{
		// only use selected streams
		if(!it->checked) continue;

		path remoteFileIndex;
		if(WSync::getTempFilename(remoteFileIndex))
		{
			updateCallback(MSE_ERROR, "error creating tempfile!");
			return -1;
		}

		updateCallback(MSE_UPDATE_TEXT, "downloading file list...");
		string url = "/" + conv(it->path) + "/" + INDEXFILENAME;
		if(this->downloadFile(w, remoteFileIndex.string(), mainserver, url))
		{
			updateCallback(MSE_ERROR, "error downloading file index from http://" + mainserver + url);
			return -1;
		}

		/*
		// this is not usable with overlaying streams anymore :(
		string hashMyFileIndex = w->generateFileHash(myFileIndex);
		string hashRemoteFileIndex = w->generateFileHash(remoteFileIndex);
		
		remove(INDEXFILENAME);
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
		int res = w->loadHashMapFromFile(remoteFileIndex, temp_hashMapRemote, modeNumber);
		// remove that temp file
		remove(remoteFileIndex);
		if(res)
		{
			updateCallback(MSE_ERROR, "error reading remote file index");
			return -2;
		}

		if(temp_hashMapRemote.size() == 0)
		{
			updateCallback(MSE_ERROR, "remote file index is invalid\nConnection Problems / Server down?");
			return -3;
		}

		// add it to our map
		hashMapRemote[conv(it->path)] = temp_hashMapRemote;
	}
	return 0;
}

int WsyncThread::sync()
{
	int res=0;
	char tmp[512] = "";
	memset(tmp, 0, 512);

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
		for(it = hashMapLocal.begin(); it != hashMapLocal.end(); it++)
		{
			// filter some utility files
			if(it->first == string("/update.temp.exe")) continue;
			if(it->first == string("/stream.info")) continue;
			if(it->first == string("/version")) continue;

			if(it->first.find(deletedKeyword) != string::npos)
			{
				// check if the file is deleted already
				path localfile = ipath / it->first;
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
		for(it = itr->second.begin(); it != itr->second.end(); it++)
		{
			// filter some files
			if(it->first == string("/stream.info")) continue;
			if(it->first == string("/version")) continue;
					
			// add deleted files if they still exist
			if(it->first.find(deletedKeyword) != string::npos)
			{
				// check if the file is deleted already
				path localfile = ipath / it->first;
				if(exists(localfile))
					deletedFiles.push_back(Fileentry(itr->first, it->first, it->second.filesize));
				continue;
			}

			if(hashMapLocal.find(it->first) == hashMapLocal.end())
				newFiles.push_back(Fileentry(itr->first, it->first, it->second.filesize));
		}
	}

	// done comparing, now summarize the changes
	std::vector<Fileentry>::iterator itf;
	int changeCounter = 0, changeMax = changedFiles.size() + newFiles.size() + deletedFiles.size();
	int filesToDownload = newFiles.size() + changedFiles.size();
	
	predDownloadSize = 0; // predicted download size
	// XXX DEBUG
	//FILE *f=fopen("log.txt", "w");
	for(itf=newFiles.begin(); itf!=newFiles.end(); itf++)
	{
		predDownloadSize += (int)itf->filesize;
		// DEBUG
		//fprintf(f, "> A path:%s, file: %s, size:%d\n", itf->stream_path.c_str(), itf->filename.c_str(), itf->filesize);
	}
	
	for(itf=changedFiles.begin(); itf!=changedFiles.end(); itf++)
	{
		predDownloadSize += (int)itf->filesize;
		//fprintf(f, "> U path:%s, file: %s, size:%d\n", itf->stream_path.c_str(), itf->filename.c_str(), itf->filesize);
	}
	// XXX DEBUG
	//fclose(f);

	if(predDownloadSize > 0)
	{
		bool measureMirrorSpeed = (predDownloadSize > 10485760); // 10 MB
		if(findMirror(measureMirrorSpeed) && measureMirrorSpeed)
		{
			// try to find a mirror without measuring the speed
			findMirror();
		}
	}

	// security check in order not to delete the entire harddrive
	if(deletedFiles.size() > 1000)
	{
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
retry:
				//progressOutputShort(float(changeCounter)/float(changeMax));
				sprintf(tmp, "downloading new file: %s (%s) ", itf->filename.c_str(), WSync::formatFilesize(itf->filesize).c_str());
				updateCallback(MSE_UPDATE_TEXT, string(tmp));
				path localfile = ipath / itf->filename;
				string url = dir_use + itf->stream_path + itf->filename;
				int stat = this->downloadFile(w, localfile, server_use, url);
				if(stat == -404 && retrycount < 2)
				{
					// fallback to main server!
					//printf("falling back to main server.\n");
					server_use = mainserver;
					dir_use = mainserverdir;
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
							server_use = mainserver;
							dir_use = mainserverdir;
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
				updateCallback(MSE_UPDATE_TEXT, string(tmp));
				path localfile = ipath / itf->filename;
				string url = dir_use + itf->stream_path + itf->filename;
				int stat = this->downloadFile(w, localfile, server_use, url);
				if(stat == -404 && retrycount < 2)
				{
					// fallback to main server!
					//printf("falling back to main server.\n");
					server_use = mainserver;
					dir_use = mainserverdir;
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
							server_use = mainserver;
							dir_use = mainserverdir;
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
				//progressOutputShort(float(changeCounter)/float(changeMax));
				sprintf(tmp, "deleting file: %s (%s)\n", itf->filename.c_str(), WSync::formatFilesize(itf->filesize).c_str());
				updateCallback(MSE_UPDATE_TEXT, string(tmp));
				path localfile = ipath / itf->filename;
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
		}
		sprintf(tmp, "sync complete, downloaded %s\n", WSync::formatFilesize(w->getDownloadSize()).c_str());
		res = 1;
	} else
	{
		sprintf(tmp, "sync complete (already up to date), downloaded %s\n", WSync::formatFilesize(w->getDownloadSize()).c_str());
	}

	// update progress for the last time
	downloadProgress(w);
	// user may proceed
	updateCallback(MSE_DONE, string(tmp), 1);
	return res;
}



int WsyncThread::downloadFile(WSync *w, boost::filesystem::path localFile, string server, string path)
{
	//wxMessageBox(_("download of http://") + conv(server) + conv(path) + _(" to ") + conv(localFile.string()), _("download"));
	if(!dlStarted)
	{
		dlStarted=1;
		dlStartTime = clock();
	}
	// remove '//' and '///' from url
	WSync::cleanURL(path);
	try
	{
		clock_t time = clock();

		std::ofstream myfile;
		WSync::ensurePathExist(localFile);

		boost::asio::io_service io_service;

		// Get a list of endpoints corresponding to the server name.
		tcp::resolver resolver(io_service);
		char *host = const_cast<char*>(server.c_str());
		tcp::resolver::query query(tcp::v4(), host, "80");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;

		// Try each endpoint until we successfully establish a connection.
		tcp::socket socket(io_service);
		boost::system::error_code error = boost::asio::error::host_not_found;
		while (error && endpoint_iterator != end)
		{
			socket.close();
			socket.connect(*endpoint_iterator++, error);
		}
		if (error)
			throw boost::system::system_error(error);

		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << "GET " << path << " HTTP/1.0\r\n";
		request_stream << "Host: " << server << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: close\r\n\r\n";

		// Send the request.
		boost::asio::write(socket, request);

		// Read the response status line.
		boost::asio::streambuf response;
		boost::asio::streambuf data;
		boost::asio::read_until(socket, response, "\r\n");

		// Check that response is OK.
		std::istream response_stream(&response);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		std::string status_message;
		std::getline(response_stream, status_message);
		if (!response_stream || http_version.substr(0, 5) != "HTTP/")
		{
			socket.close();
			std::cout << endl << "Error: Invalid response\n";
			printf("download URL: http://%s%s\n", server.c_str(), path.c_str());
			return 1;
		}
		if (status_code == 302)
		{
			// catch redirects
			socket.close();
			boost::asio::read_until(socket, response, "\r\n\r\n");
			std::string line="",new_url="";
			size_t counter = 0;
			size_t reported_filesize = 0;
			{
				while (std::getline(response_stream, line) && line != "\r")
				{
					if(line.substr(0, 10) == "location: ")
						new_url = line.substr(10).c_str();
				}
			}
			// check protocol
			if(new_url.substr(0, 7) != "http://")
			{
				std::cout << endl << "Error: redirection uses unkown protocol: " << new_url << "\n";
				return 1;
			}
			// trim line
			new_url = new_url.substr(0, new_url.find_first_of("\r"));

			// separate URL into server and path
			string new_server = new_url.substr(7, new_url.find_first_of("/", 7)-7);
			string new_path = new_url.substr(new_url.find_first_of("/", 7));

			//std::cout << "redirect to : '" << new_url << "'\n";
			//std::cout << "server : '" << new_server << "'\n";
			//std::cout << "path : '" << new_path << "'\n";

			return downloadFile(w, localFile, new_server, new_path);
		}
		if (status_code != 200)
		{
			std::cout << endl << "Error: Response returned with status code " << status_code << "\n";
			printf("download URL: http://%s%s\n", server.c_str(), path.c_str());
			return -(int)status_code;
		}

		// Read the response headers, which are terminated by a blank line.
		boost::asio::read_until(socket, response, "\r\n\r\n");

		// Process the response headers.
		std::string line;
		size_t counter = 0;
		size_t reported_filesize = 0;
		{
			while (std::getline(response_stream, line) && line != "\r")
			{
				if(line.substr(0, 15) == "Content-Length:")
					reported_filesize = atoi(line.substr(16).c_str());
			}
		}
		//printf("filesize: %d bytes\n", reported_filesize);

		// open local file late -> prevent creating emoty files
		myfile.open(localFile.string().c_str(), ios::out | ios::binary);
		if(!myfile.is_open())
		{
			printf("error opening file: %s\n", localFile.string().c_str());
			return 2;
		}

		// Write whatever content we already have to output.
		if (response.size() > 0)
			myfile << &response;

		// Read until EOF, writing data to output as we go.
		boost::uintmax_t datacounter=0;
		boost::uintmax_t dataspeed=0;
		boost::uintmax_t filesize_left=reported_filesize;
		//boost::uintmax_t datacounter_before = w->getDownloadSize();
		while (boost::asio::read(socket, data, boost::asio::transfer_at_least(1), error))
		{
			double tdiff = (double(clock())-double(time))/CLOCKS_PER_SEC;
			if(tdiff >= 1)
			{
				w->increaseDownloadSize(dataspeed);
				downloadProgress(w);
				dataspeed=0;
				time = clock();
			}

			dataspeed += data.size();
			datacounter += data.size();
			filesize_left -= data.size();
			myfile << &data;
		}

		// dont loose some last bytes
		if(dataspeed>0)
			w->increaseDownloadSize(dataspeed);

		if (error != boost::asio::error::eof)
			throw boost::system::system_error(error);

		myfile.close();
		boost::uintmax_t fileSize = file_size(localFile);
		socket.close();
		if(reported_filesize != 0 && fileSize != reported_filesize)
		{
			printf("\nError: file size is different: should be %d, is %d. removing file.\n", reported_filesize, fileSize);
			printf("download URL: http://%s%s\n", server.c_str(), path.c_str());
			remove(localFile);
			return 1;
			// remove file data transfer again
			//setDownloadSize(datacounter_before);
		}
		
		// traffic stats tracking
		if(traffic_stats.find(server) == traffic_stats.end())
			traffic_stats[server] = 0;
		// add filesize to traffic stats
		traffic_stats[server] += fileSize;
	}
	catch (std::exception& e)
	{
		std::cout << endl << "Error: " << e.what() << "\n";
		printf("download URL: http://%s%s\n", server.c_str(), path.c_str());
		return 1;
	}
	return 0;
}

std::string WsyncThread::formatSeconds(float seconds)
{
	char tmp[255]="";
	if(seconds > 0 && seconds < 1)
	{
		sprintf(tmp, "%.0f milli seconds", (seconds*1000.0f));
	}
	else if(seconds >= 1 && seconds < 1000000)
	{
		if(seconds<60)
			sprintf(tmp, "%.0f seconds", seconds);
		else
			sprintf(tmp, "%.0f minutes, %.0f seconds", seconds/60, seconds - (((int)(seconds/60)) * 60));
	}
	return std::string(tmp);
}

void WsyncThread::downloadProgress(WSync *w)
{
	// this function will format and send some info to the gui
	unsigned int size_left = predDownloadSize - w->getDownloadSize();
	unsigned int size_done = w->getDownloadSize();
	double tdiff = (double(clock())-double(dlStartTime))/CLOCKS_PER_SEC;
	float speed = (float)(size_done / tdiff);
	float eta = size_left / speed;
	if(predDownloadSize == 0) eta = 0;
	float progress = ((float)w->getDownloadSize()) /((float)predDownloadSize);
	char tmp[255]="";

	// update the progress bar
	updateCallback(MSE_UPDATE_PROGRESS, "", progress);

	string sizeDone = WSync::formatFilesize(size_done);
	string sizePredicted = WSync::formatFilesize(predDownloadSize);
	
	string timestr = formatSeconds(tdiff);
	updateCallback(MSE_UPDATE_TIME, timestr);
	
	if(eta > 10)
	{
		timestr = formatSeconds(eta);
		updateCallback(MSE_UPDATE_TIME_LEFT, timestr);
	} else if (eta != 0)
	{
		timestr = conv(_("less than 10 seconds"));
		updateCallback(MSE_UPDATE_TIME_LEFT, timestr);
	}

	string speedstr = WSync::formatFilesize((int)speed) + "/s";
	updateCallback(MSE_UPDATE_SPEED, speedstr);
	
	if(progress<1.0f)
	{
		char trafstr[256] = "";
		sprintf(trafstr, "%s / %s (%0.0f%%)", sizeDone.c_str(), sizePredicted.c_str(), progress * 100);
		updateCallback(MSE_UPDATE_TRAFFIC, string(trafstr));
	} else
	{
		string sizeOverhead = WSync::formatFilesize(size_done-predDownloadSize);
		char trafstr[256] = "";
		sprintf(trafstr, "%s (%s overhead)", sizeDone.c_str(), sizeOverhead.c_str());
		updateCallback(MSE_UPDATE_TRAFFIC, string(trafstr));
	}
}

void WsyncThread::recordDataUsage()
{
	if(traffic_stats.size() == 0) return;
	std::map < std::string, unsigned int >::iterator itt;
	for(itt=traffic_stats.begin();itt!=traffic_stats.end(); itt++)
	{
		std::vector< std::vector< std::string > > list;
		char tmp[256]="";
		sprintf(tmp, API_RECORDTRAFFIC, itt->first.c_str(), itt->second);
		w->responseLessRequest(API_SERVER, string(tmp));
	}
}

int WsyncThread::findMirror(bool probeForBest)
{
	std::vector< std::vector< std::string > > list;		
	updateCallback(MSE_STARTING, "finding suitable mirror ...");
	if(!probeForBest)
	{
		updateCallback(MSE_STARTING, "getting random mirror ...");
		// just collect a best fitting server by geolocating this client's IP
		if(!w->downloadConfigFile(API_SERVER, API_MIRROR, &list))
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
		if(!w->downloadConfigFile(API_SERVER, API_MIRROR_NOGEO, &list))
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
					double tdiff = w->measureDownloadSpeed(list[i][0], list[i][1]+"../speedtest.bin");
					if(tdiff >=0 && tdiff < bestTime)
					{
						bestTime = tdiff;
						bestServer = i;
					}
					/*
					char tmp[255]="";
					sprintf(tmp, "%6.2f kB/s", (10240.0f / tdiff) / 1024.0f);
					updateCallback(MSE_STARTING, "speed of " + list[i][0] + std::string(tmp));
					Sleep(1000);
					*/
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
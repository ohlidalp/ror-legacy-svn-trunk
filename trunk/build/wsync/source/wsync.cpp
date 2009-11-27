// thomas fischer (thomas{AT}thomasfischer{DOT}biz) 6th of Janurary 2009

#include "wsync.h"
#include "SHA1.h"
#include "tokenize.h"
#include <ctime>

#ifdef WIN32
#include <windows.h>
#include <conio.h> // for getch
#endif
#include "Timer.h"
#include <boost/algorithm/string.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::filesystem;
using namespace std;

WSync::WSync() : downloadSize(0)
{
	memset(statusText, 0, 1024);
	statusPercent=0;
	dlerror=0;
}

WSync::~WSync()
{
}

int WSync::downloadMod(std::string modname, std::string &modfilename, boost::filesystem::path dir, bool util)
{
	ensurePathExist(dir);

	// 1) ask repo api for file
	if(!util) sprintf(statusText, "searching mod on repository...\n");
	string filename="";
	std::vector< std::vector< std::string > > list;
	if(!downloadConfigFile(API_SERVER, API_REPOSEARCH + string("?id=") + modname, &list))
	{
		if(list.size()>0 && list[0].size() > 0)
		{
			filename = list[0][0];
			if(!util)
				sprintf(statusText, "found mod on repo: %s\n", filename.c_str());
		} else
		{	if(!util)
				sprintf(statusText, "wrong repo response2\n");
			else
				sprintf(statusText, "error\n");
			return 2;
		}
	} else
	{
		if(!util)
			sprintf(statusText, "wrong repo response\n");
		else
			sprintf(statusText, "error\n");
		return 1;
	}

	// do not download non-existing files
	if(filename == "notfound")
	{
		if(util) sprintf(statusText, "%s", filename.c_str());
		return 3;
	}

	// 2) download the file
	path filepath = dir / filename;

	if(!util) sprintf(statusText, "downloading ...");
	int res = downloadFile(filepath, REPO_SERVER, REPO_DOWNLOAD + filename, !util);
	if(!util) sprintf(statusText, "downloading done!\n");
	if(util) printf("%s", filename.c_str());
	modfilename = filename;
	return res;
}

int WSync::cleanURL(string &url)
{
	int position = url.find("//");
	while (position != string::npos)
	{
		url.replace(position, 2, "/" );
		position = url.find( "//", position + 1 );
	}
	position = url.find("//");
	while (position != string::npos)
	{
		url.replace(position, 2, "/" );
		position = url.find( "//", position + 1 );
	}
	return 0;
}

std::string WSync::findHashInHashmap(std::map<string, Hashentry> hashMap, std::string filename)
{
	std::map<string, Hashentry>::iterator it;
	for(it = hashMap.begin(); it != hashMap.end(); it++)
	{
		if(it->first == filename)
			return it->second.hash;
	}
	return "";
}

std::string WSync::findHashInHashmap(std::map < std::string, std::map < std::string, Hashentry > > hashMap, std::string filename)
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

int WSync::sync(boost::filesystem::path localDir, string server, string remoteDir, bool useMirror, bool deleteOk)
{
	// download remote currrent file first
	int res = 0;
	path remoteFileIndex;
	if(getTempFilename(remoteFileIndex))
		printf("error creating tempfile!\n");

	string url = "/" + remoteDir + "/" + INDEXFILENAME;
	if(downloadFile(remoteFileIndex.string(), server, url))
	{
		printf("error downloading file index from %s\n", url.c_str());
		return -1;
	}
	//printf("downloaded FileIndex\n");

	//printf("generating local FileIndex\n");
	path myFileIndex = localDir / INDEXFILENAME;

	std::map<string, Hashentry> hashMapLocal;
	if(buildFileIndex(myFileIndex, localDir, localDir, hashMapLocal, true, 1))
		printf("error while generating local FileIndex\n");
	//printf("#1: %s\n", myFileIndex.string().c_str());
	//printf("#2: %s\n", remoteFileIndex.string().c_str());


	string hashMyFileIndex = generateFileHash(myFileIndex);
	string hashRemoteFileIndex = generateFileHash(remoteFileIndex);
	//printf("#2: CWD:%s \n", boost::filesystem::current_path().string().c_str());
	//printf("#3: %s = %s\n", myFileIndex.string().c_str(), hashMyFileIndex.c_str());
	//printf("#4: %s = %s\n", remoteFileIndex.string().c_str(), hashRemoteFileIndex.c_str());

	// remove temp file again
	tryRemoveFile(INDEXFILENAME);
	if(hashMyFileIndex == hashRemoteFileIndex)
	{
		printf("Files are up to date, no sync needed\n");
		return 0;
	}

	// now find out what files differ
	std::map<string, Hashentry> hashMapRemote;
	int modeNumber = 0;
	if(loadHashMapFromFile(remoteFileIndex, hashMapRemote, modeNumber))
	{
		printf("error reading remote file index!\n");
		return -2;
	}

	if(hashMapRemote.size() == 0)
	{
		printf("remote file index is invalid\nConnection Problems / Server down?\n");
		return -3;
	}
	// remove that temp file as well
	tryRemoveFile(remoteFileIndex);

	std::vector<Fileentry> deletedFiles;
	std::vector<Fileentry> changedFiles;
	std::vector<Fileentry> newFiles;

	std::map<string, Hashentry>::iterator it;
	//first, detect deleted or changed files
	for(it = hashMapLocal.begin(); it != hashMapLocal.end(); it++)
	{
		if(it->first == string("/update.temp.exe"))
			continue;
		if(it->first == string("/stream.info")) continue;
		if(it->first == string("/version")) continue;
		//if(hashMapRemote[it->first] == it->second)
			//printf("same: %s %s==%s\n", it->first.c_str(), hashMapRemote[it->first].c_str(), it->second.c_str());
		if(hashMapRemote.find(it->first) == hashMapRemote.end())
			deletedFiles.push_back(Fileentry("",it->first, it->second.filesize));
		else if(hashMapRemote[it->first].hash != it->second.hash)
			changedFiles.push_back(Fileentry("",it->first, hashMapRemote[it->first].filesize));
	}
	// second, detect new files
	for(it = hashMapRemote.begin(); it != hashMapRemote.end(); it++)
	{
		if(it->first == string("/stream.info")) continue;
		if(it->first == string("/version")) continue;
		if(hashMapLocal.find(it->first) == hashMapLocal.end())
			newFiles.push_back(Fileentry("", it->first, it->second.filesize));
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
		printf("are you sure you have placed the application in the correct directory?\nIt will delete over 1000 files! Aborting ...\n");
#ifdef WIN32
		//printf("Press any key to continue...\n");
		//_getch();
		//exit(0);
#endif
	}

	if(changeMax)
	{
		string server_use = server, dir_use = remoteDir;
		bool use_mirror = false;
		if(filesToDownload)
		{
			printf("downloading %d files now (%s) ..\n", filesToDownload, formatFilesize(predDownloadSize).c_str());

			// now find a suitable mirror
			string mirror_server="", mirror_dir="", mirror_info="";
			if(useMirror)
			{
				printf("searching for suitable mirror...\n");
				std::vector< std::vector< std::string > > list;
				if(!downloadConfigFile(API_SERVER, API_MIRROR, &list))
				{
					if(list.size()>0 && list[0].size() > 2)
					{
						if(list[0][0] == "failed")
						{
							printf("failed to get mirror, using main server\n");
						} else
						{
							mirror_server = list[0][0];
							mirror_dir = list[0][1];
							mirror_info = list[0][2];
							use_mirror=true;
							printf("using mirror server: %s, %s\n", mirror_server.c_str(), mirror_info.c_str());
						}
					} else
						printf("wrong API response2\n");
				} else
					printf("wrong API response\n");
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
				progressOutputShort(float(changeCounter)/float(changeMax));
				printf(" A  %s (%s) ", itf->filename.c_str(), formatFilesize(itf->filesize).c_str());
				path localfile = localDir / itf->filename;
				string url = "/" + dir_use + "/" + itf->filename;
				int stat = downloadFile(localfile, server_use, url, true);
				if(stat == -404 && retrycount < 2)
				{
					// fallback to main server!
					printf("falling back to main server.\n");
					server_use = server;
					dir_use = remoteDir;
					retrycount++;
					goto retry;
				}
				if(stat)
				{
					printf("\nunable to create file: %s\n", itf->filename.c_str());
				} else
				{
					string checkHash = generateFileHash(localfile);
					string hash_remote = findHashInHashmap(hashMapRemote, itf->filename);
					if(hash_remote == checkHash)
					{
						printf(" OK                                             \n");
					} else
					{
						printf(" OK                                             \n");
						//printf(" hash is: '%s'\n", checkHash.c_str());
						//printf(" hash should be: '%s'\n", hash_remote.c_str());
						tryRemoveFile(localfile);
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
				progressOutputShort(float(changeCounter)/float(changeMax));
				printf(" U  %s (%s) ", itf->filename.c_str(), formatFilesize(itf->filesize).c_str());
				path localfile = localDir / itf->filename;
				string url = "/" + dir_use + "/" + itf->filename;
				int stat = downloadFile(localfile, server_use, url, true);
				if(stat == -404 && retrycount < 2)
				{
					// fallback to main server!
					printf("falling back to main server.\n");
					server_use = server;
					dir_use = remoteDir;
					retrycount++;
					goto retry2;
				}
				if(stat)
				{
					printf("\nunable to update file: %s\n", itf->filename.c_str());
				} else
				{
					string checkHash = generateFileHash(localfile);
					string hash_remote = findHashInHashmap(hashMapRemote, itf->filename);
					if(hash_remote == checkHash)
					{
						printf(" OK                                             \n");
					} else
					{
						printf(" OK                                             \n");
						//printf(" hash is: '%s'\n", checkHash.c_str());
						//printf(" hash should be: '%s'\n", hash_remote.c_str());
						tryRemoveFile(localfile);
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
				progressOutputShort(float(changeCounter)/float(changeMax));
				printf(" D  %s (%s)\n", itf->filename.c_str(), formatFilesize(itf->filesize).c_str());
				path localfile = localDir / itf->filename;
				try
				{
					tryRemoveFile(localfile);
					if(exists(localfile))
						printf("unable to delete file: %s\n", localfile.string().c_str());
				} catch(...)
				{
					printf("unable to delete file: %s\n", localfile.string().c_str());
				}
			}
		} else if (!deleteOk && deletedFiles.size())
		{
			printf("wont delete any files during this run\n");
		}

		printf("sync complete, downloaded %s\n", formatFilesize(downloadSize).c_str());
		res = 1;
	} else
		printf("sync complete (already up to date), downloaded %s\n", formatFilesize(downloadSize).c_str());

	//remove temp files again
	tryRemoveFile(remoteFileIndex);
	return res;
}

// util functions below
string WSync::generateFileHash(boost::filesystem::path file)
{
	CSHA1 sha1;
	bool res = sha1.HashFile(const_cast<char*>(file.string().c_str()));
	if(!res) return string("");
	sha1.Final();
	char resultHash[256] = "";
	sha1.ReportHash(resultHash, CSHA1::REPORT_HEX_SHORT);
	return string(resultHash);
}

int WSync::buildFileIndex(boost::filesystem::path &outfilename, boost::filesystem::path &path, boost::filesystem::path &rootpath, std::map<string, Hashentry> &hashMap, bool writeFile, int mode)
{
	vector<string> files;
	listFiles(path, files);
	printf("building local file index ...");
	int counter = 0, counterMax = files.size();
	for(vector<string>::iterator it=files.begin(); it!=files.end(); it++, counter++)
	{
		progressOutput(float(counter)/float(counterMax));
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
	printf("done.                      \n");
	if(writeFile)
		return saveHashMapToFile(outfilename, hashMap, mode);
	return 0;
}

int WSync::createIndex(boost::filesystem::path localDir, int mode)
{
	path fileIndex = localDir / INDEXFILENAME;
	std::map<string, Hashentry> hashMap;
	return buildFileIndex(fileIndex, localDir, localDir, hashMap, true, mode);
}

int WSync::saveHashMapToFile(boost::filesystem::path &filename, std::map<string, Hashentry> &hashMap, int mode)
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

double WSync::measureDownloadSpeed(std::string server, std::string url)
{
	path tempfile;
	if(getTempFilename(tempfile))
	{
		printf("error creating tempfile!\n");
		return -1;
	}

	int filesize=0;
	Timer timer = Timer();
	if(downloadFile(tempfile, server, url, false, false, &filesize))
	{
		return -1;
	}
	double tdiff = timer.elapsed();
	printf("mirror speed: %s : %dkB in %0.2f seconds = %0.2f kB/s\n", server.c_str(), (int)(filesize/1024.0f), tdiff, (filesize/1024.0f)/(float)tdiff);
	tryRemoveFile(tempfile);
	return tdiff;
}

int WSync::downloadConfigFile(std::string server, std::string url, std::vector< std::vector< std::string > > *list)
{
	if(!list) return -1;
	path tempfile;
	if(getTempFilename(tempfile))
	{
		printf("error creating tempfile!\n");
		return -1;
	}

	if(downloadFile(tempfile, server, url))
	{
		printf("error downloading file from %s, %s\n", server.c_str(), url.c_str());
		return -1;
	}
	ifstream fin(tempfile.string().c_str());
	if (fin.is_open() == false)
	{
		printf("unable to open file for reading: %s\n", tempfile.string().c_str());
		return -1;
	}
	string line = string();
	while(getline(fin, line))
	{
		std::vector<std::string> args = tokenize_str(line, " ");
		list->push_back(args);
	}
	fin.close();
	tryRemoveFile(tempfile);
	return 0;
}

int WSync::downloadAdvancedConfigFile(std::string server, std::string url, std::vector< std::map< std::string, std::string > > &list)
{
	path tempfile;
	if(getTempFilename(tempfile))
	{
		printf("error creating tempfile!\n");
		return -1;
	}

	if(downloadFile(tempfile, server, url))
	{
		printf("error downloading file from %s, %s\n", server.c_str(), url.c_str());
		return -1;
	}
	ifstream fin(tempfile.string().c_str());
	if (fin.is_open() == false)
	{
		printf("unable to open file for reading: %s\n", tempfile.string().c_str());
		return -1;
	}
	bool complete = false;
	string line = string();
	std::map < std::string, std::string > obj;
	while(getline(fin, line))
	{
		if(!line.size()) continue;
		if(line == "#end")
		{
			complete = true;
			continue;
		}
		if(line[0] == '#') continue;
		if(line[0] == '=')
		{
			// new stream
			if(obj.size() > 0)
				list.push_back(obj);
			obj.clear();
			continue;
		}
		std::vector<std::string> args = tokenize_str(line, "=");
		if(args.size() == 2)
		{
			boost::trim(args[0]);
			boost::trim(args[1]);
			obj[args[0]] = args[1];
		}
	}
	fin.close();
	tryRemoveFile(tempfile);

	if(!complete)
	{
		dlerror++;
		if(dlerror < 3)
		{
			// three retries
			return downloadAdvancedConfigFile(server, url, list);
		} else
		{
			printf("error downloading file from %s, %s\n", server.c_str(), url.c_str());
			return -1;
		}
	}
	return 0;
}

void WSync::tryRemoveFile(boost::filesystem::path filename)
{
	try
	{
		remove(filename);
	} catch(...)
	{
	}
}
int WSync::loadHashMapFromFile(boost::filesystem::path &filename, std::map<string, Hashentry> &hashMap, int &mode)
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


int WSync::responseLessRequest(std::string server, std::string uri)
{
	std::vector< std::vector< std::string > > list;
	return downloadConfigFile(server, uri, &list);
}

int WSync::downloadFile(boost::filesystem::path localFile, string server, string path, bool displayProgress, bool debug, int *fileSizeArg)
{
	// remove '//' and '///' from url
	cleanURL(path);
	double lastTime=-1;
	try
	{
		Timer time = Timer();

		std::ofstream myfile;
		ensurePathExist(localFile);

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

			return downloadFile(localFile, new_server, new_path, displayProgress);
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
		while (boost::asio::read(socket, data, boost::asio::transfer_at_least(1), error))
		{
			double tdiff = (time.elapsed() - lastTime);
			if(displayProgress && tdiff >= 0.5f)
			{
				float percent = datacounter / (float)reported_filesize;
				float dspeed = (float)(dataspeed / tdiff);
				float eta = filesize_left / dspeed;
				progressOutput(percent, dspeed, eta);
				dataspeed=0;
				lastTime = time.elapsed();
			}

			if (displayProgress)
				dataspeed += data.size();
			datacounter += data.size();
			filesize_left -= data.size();
			myfile << &data;
		}
		if (error != boost::asio::error::eof)
			throw boost::system::system_error(error);

		myfile.close();
		boost::uintmax_t fileSize = file_size(localFile);
		if(reported_filesize != 0 && fileSize != reported_filesize)
		{
			printf("\nError: file size is different: should be %d, is %d. removing file.\n", (int)reported_filesize, (int)fileSize);
			printf("download URL: http://%s%s\n", server.c_str(), path.c_str());
			tryRemoveFile(localFile);
			downloadSize += fileSize;
			socket.close();
			return 2;
		}
		socket.close();

		downloadSize += fileSize;
		if(fileSizeArg)
			*fileSizeArg = (int)fileSize;
	}
	catch (std::exception& e)
	{
		std::cout << endl << "Error: " << e.what() << "\n";
		printf("download URL: http://%s%s\n", server.c_str(), path.c_str());
		return 1;
	}
	return 0;
}

void WSync::listFiles(const path &dir_path, std::vector<std::string> &files)
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

void WSync::progressOutput(float progress, float speed, float eta)
{
	if(speed<0)
	{
		printf("(% 3.0f%%)\b\b\b\b\b\b", progress * 100);
	} else
	{
		char tmp[255]="";
		char etastr[255]="";
		if(eta > 0 && eta < 1000000)
		{
			if(eta<60)
				sprintf(etastr, ", ETA: % 4.0f seconds", eta);
			else
				sprintf(etastr, ", ETA: % 4.1f minutes", eta/60.0);
		}

		string speedstr = formatFilesize((int)speed) + "/s";
		sprintf(tmp, "(% 3.0f%%, %s%s)   ", progress * 100, speedstr.c_str(), etastr);
		int stringsize = (int)strnlen(tmp, 255);
		for(int i=0;i<stringsize; i++)
			strcat(tmp, "\b");
		cout << tmp;
	}
}

void WSync::progressOutputShort(float progress)
{
	sprintf(statusText, "% 3.0f%%|", progress * 100);
	statusPercent = (int)(progress * 100);
}

int WSync::getTempFilename(path &tempfile)
{
#ifdef _WIN32
	char tmp_path[MAX_PATH]="";
	char tmp_name[MAX_PATH]="";

	if(GetTempPath(MAX_PATH, tmp_path) == 0)
		return -2;
	//printf("GetTempPath() returned: '%s'\n", tmp_path);
	if(GetTempFileNameA(tmp_path, "WSY", 0, tmp_name) == 0)
		return -3;

	tempfile = path(tmp_name, native);
	return 0;
#else
	char tempBuffer[] = "/tmp/wsync_tmp_XXXXXX";
	int res = mkstemp(tempBuffer);
	if(res == -1)
		return -4;
	tempfile = path(tempBuffer);
	return 0;
#endif
}

int WSync::ensurePathExist(boost::filesystem::path &path)
{
	boost::filesystem::path::iterator it;
	for(it=path.begin();it!=path.end(); it++)
	{
		if(!exists(*it))
			create_directories(path.branch_path());
	}
	return 0;
}

string WSync::formatFilesize(boost::uintmax_t size)
{
	char tmp[256] = "";
	if(size < 1024)
		sprintf(tmp, "%d B", (int)(size));
	else if(size/1024 < 1024)
		sprintf(tmp, "%0.2f kB", (size/1024.0f));
	else
		sprintf(tmp, "%0.2f MB", (size/1024.0f/1024.0f));
	return string(tmp);
}

int WSync::getStatus(int &percent, std::string &message)
{
	message = std::string(statusText);
	percent = statusPercent;
	return 0;
}

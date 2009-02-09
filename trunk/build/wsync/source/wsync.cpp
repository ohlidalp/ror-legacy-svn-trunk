// thomas fischer (thomas{AT}thomasfischer{DOT}biz) 6th of Janurary 2009

#include "wsync.h"
#include "SHA1.h"
#include "tokenize.h"
#include <ctime>

#ifdef _WIN32
# include <windows.h>
#else
#endif

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::filesystem; 
using namespace std; 

WSync::WSync() : downloadSize(0)
{
}

WSync::~WSync()
{
}

int WSync::sync(boost::filesystem::path localDir, string server, string remoteDir)
{
	// download remote currrent file first
	path remoteFileIndex;
	if(getTempFilename(remoteFileIndex))
		printf("error creating tempfile!\n");

	string url = "/" + remoteDir + "/" + INDEXFILENAME;
	if(downloadFile(remoteFileIndex.string(), server, url))
	{
		printf("error downloading file index\n");
		return -1;
	}
	//printf("downloaded FileIndex\n");

	//printf("generating local FileIndex\n");
	path myFileIndex = localDir / INDEXFILENAME;

	std::map<string, Hashentry> hashMapLocal;
	if(buildFileIndex(myFileIndex, localDir, localDir, hashMapLocal, false))
		printf("error while generating local FileIndex\n");
	//printf("#1: %s\n", myFileIndex.string().c_str());
	//printf("#2: %s\n", remoteFileIndex.string().c_str());


	string hashMyFileIndex = generateFileHash(myFileIndex);
	string hashRemoteFileIndex = generateFileHash(remoteFileIndex);
	//printf("#3: %s\n", hashMyFileIndex.c_str());
	//printf("#4: %s\n", hashRemoteFileIndex.c_str());

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

	std::vector<Fileentry> deletedFiles;
	std::vector<Fileentry> changedFiles;
	std::vector<Fileentry> newFiles;

	std::map<string, Hashentry>::iterator it;
	//first, detect deleted or changed files
	for(it = hashMapLocal.begin(); it != hashMapLocal.end(); it++)
	{
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
		if(hashMapLocal.find(it->first) == hashMapLocal.end())
			newFiles.push_back(Fileentry(it->first, it->second.filesize));
	}
	// done comparing

	std::vector<Fileentry>::iterator itf;
	int filesToDownload = newFiles.size() + changedFiles.size();
	int predDownloadSize = 0;
	for(itf=newFiles.begin(); itf!=newFiles.end(); itf++)
		predDownloadSize += (int)itf->filesize;
	for(itf=changedFiles.begin(); itf!=changedFiles.end(); itf++)
		predDownloadSize += (int)itf->filesize;

	if(filesToDownload)
		printf("downloading %d files now (%s) ..\n", filesToDownload, formatFilesize(predDownloadSize).c_str());

	// do things now!
	if(changedFiles.size())
	{
		for(itf=changedFiles.begin();itf!=changedFiles.end();itf++)
		{
			printf("U    %s (%s) ", itf->filename.c_str(), formatFilesize(itf->filesize).c_str());
			path localfile = localDir / itf->filename;
			string url = "/" + remoteDir + "/" + itf->filename;
			if(downloadFile(localfile, server, url, true))
				printf("\nunable to update file: %s\n", itf->filename.c_str());
			else
				printf("                           \n");
		}
	}
	
	if(deletedFiles.size() && !(modeNumber & WMO_NODELETE))
	{
		for(itf=deletedFiles.begin();itf!=deletedFiles.end();itf++)
		{
			printf("D    %s (%s)\n", itf->filename.c_str(), formatFilesize(itf->filesize).c_str());
			path localfile = localDir / itf->filename;
			try
			{
				boost::filesystem::remove(localfile);
				if(exists(localfile))
					printf("unable to delete file: %s\n", localfile.string().c_str());
			} catch(...)
			{
				printf("unable to delete file: %s\n", localfile.string().c_str());
			}
		}
	}
	
	if(newFiles.size())
	{
		for(itf=newFiles.begin();itf!=newFiles.end();itf++)
		{
			printf("A    %s (%s) ", itf->filename.c_str(), formatFilesize(itf->filesize).c_str());
			path localfile = localDir / itf->filename;
			string url = "/" + remoteDir + "/" + itf->filename;
			if(downloadFile(localfile, server, url, true))
				printf("\nunable to create file: %s\n", itf->filename.c_str());
			else
				printf("                           \n");
		}
	}

	printf("sync complete, downloaded %s\n", formatFilesize(downloadSize).c_str());

	//remove temp files again
	remove(remoteFileIndex);
	return 0;
}

// util functions below
string WSync::generateFileHash(boost::filesystem::path file)
{
	CSHA1 sha1;
	sha1.HashFile(const_cast<char*>(file.string().c_str()));
	sha1.Final();
	char resultHash[256] = "";
	sha1.ReportHash(resultHash, CSHA1::REPORT_HEX_SHORT);
	return string(resultHash);
}

int WSync::buildFileIndex(boost::filesystem::path &outfilename, boost::filesystem::path &path, boost::filesystem::path &rootpath, std::map<string, Hashentry> &hashMap, bool writeFile, int mode)
{
	vector<string> files;
	listFiles(path, files);
	for(vector<string>::iterator it=files.begin(); it!=files.end(); it++)
	{
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


		string resultHash = generateFileHash(it->c_str());

		Hashentry entry(resultHash, file_size(*it));
		hashMap[respath] = entry;
	}
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

int WSync::downloadConfigFile(std::string server, std::string path, std::vector< std::vector< std::string > > &list)
{
	std::vector<std::string> lines;
	try
	{
		boost::asio::io_service io_service;

		// Get a list of endpoints corresponding to the server name.
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(server, "http");
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
			std::cout << endl << "Error: Invalid response\n";
			return 1;
		}
		if (status_code != 200)
		{
			std::cout << endl << "Error: Response returned with status code " << status_code << "\n";
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

		// Write whatever content we already have to output.
		if (response.size() > 0)
		{
			string line;
			while(getline(response_stream, line))
				lines.push_back(line);
			//cout << &response;
		}

		// Read until EOF, writing data to output as we go.
		boost::uintmax_t datacounter=0;
		while (boost::asio::read(socket, data, boost::asio::transfer_at_least(1), error))
		{
			string line;
			std::istream data_stream(&data);
			while(getline(data_stream, line))
				lines.push_back(line);

			datacounter += data.size();
		}
		if (error != boost::asio::error::eof)
			throw boost::system::system_error(error);

		downloadSize += reported_filesize;
	}
	catch (std::exception& e)
	{
		std::cout << endl << "Error: " << e.what() << "\n";
		return 1;
	}
	for(std::vector<std::string>::iterator it=lines.begin(); it!=lines.end(); it++)
	{
		std::vector<std::string> args = tokenize_str(*it, " ");
		list.push_back(args);
	}
		
	return 0;
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
		boost::uintmax_t filesize = 0;
		int res = fscanf(f, "%s : %d : %s\n", file, &filesize, filehash);
		if(res < 2)
			continue;
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

int WSync::downloadFile(boost::filesystem::path localFile, string server, string path, bool displayProgress)
{
	try
	{
		std::ofstream myfile;
		ensurePathExist(localFile);
		myfile.open(localFile.string().c_str(), ios::out | ios::binary); 

		boost::asio::io_service io_service;

		// Get a list of endpoints corresponding to the server name.
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(server, "http");
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
			std::cout << endl << "Error: Invalid response\n";
			return 1;
		}
		if (status_code != 200)
		{
			std::cout << endl << "Error: Response returned with status code " << status_code << "\n";
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

		// Write whatever content we already have to output.
		if (response.size() > 0)
			myfile << &response;

		// Read until EOF, writing data to output as we go.
		boost::uintmax_t datacounter=0;
		boost::uintmax_t dataspeed=0;
		time_t time = std::time(0);
		while (boost::asio::read(socket, data, boost::asio::transfer_at_least(1), error))
		{
			if(displayProgress && std::time(0) - time > 1)
			{
				float percent = datacounter / (float)reported_filesize;
				progressOutput(percent, (float)dataspeed);
				dataspeed=0;
				time = std::time(0);
			} else if (displayProgress)
			{
				dataspeed += data.size();
			}
			datacounter += data.size();
			myfile << &data;
		}
		if (error != boost::asio::error::eof)
			throw boost::system::system_error(error);

		myfile.close();
		boost::uintmax_t fileSize = file_size(localFile);
		if(reported_filesize != 0 && fileSize != reported_filesize)
			printf("\nError: file size is different: should be %d, is %d\n", reported_filesize, fileSize);
		
		downloadSize += fileSize;
	}
	catch (std::exception& e)
	{
		std::cout << endl << "Error: " << e.what() << "\n";
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

void WSync::progressOutput(float progress, float speed)
{
	if(speed<0)
	{
		printf("(%03.0f%%)\b\b\b\b\b\b", progress * 100);
	} else
	{
		char tmp[255]="";
		string speedstr = formatFilesize((int)speed) + "/s";
		sprintf(tmp, "(%03.0f%%, %s)", progress * 100, speedstr.c_str());
		int stringsize = (int)strnlen(tmp, 255);
		for(int i=0;i<stringsize; i++)
			strcat(tmp, "\b");
		cout << tmp;
	}
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
	mkstemp(tempBuffer);
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
		sprintf(tmp, "%d B", (size));
	else if(size/1024 < 1024)
		sprintf(tmp, "%0.2f kB", (size/1024.0f));
	else
		sprintf(tmp, "%0.2f MB", (size/1024.0f/1024.0f));
	return string(tmp);
}

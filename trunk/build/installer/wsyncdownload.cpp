#include "wsyncdownload.h"

#include <wx/thread.h>
#include <wx/event.h>
#include <string.h>
#include "Timer.h"
#include "installerlog.h"
#include "tokenize.h"
#include "utils.h"

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::filesystem;
using namespace std;

std::map < std::string, boost::uintmax_t > WsyncDownload::traffic_stats;

WsyncDownload::WsyncDownload(wxEvtHandler* parent) : parent(parent)
{
}

int WsyncDownload::downloadFile(boost::filesystem::path localFile, string server, string path, boost::uintmax_t predDownloadSize, boost::uintmax_t *fileSizeArg)
{
	LOG("DLFile| http://%s%s -> %s ... \n", server.c_str(), path.c_str(), localFile.string().c_str());
	boost::uintmax_t downloadDoneSize=0;

	Timer dlStartTime = Timer();

	// remove '//' and '///' from url
	cleanURL(path);
	double lastTime=-1;
	boost::uintmax_t fileSize = 0;
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
			LOG("DLFile|Error: Invalid response: %s\n", http_version.c_str());
			LOG("DLFile|download URL: http://%s%s\n", server.c_str(), path.c_str());
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
				LOG("DLFile|Error: redirection uses unkown protocol: %s\n", new_url.c_str());
				LOG("DLFile|download URL: http://%s%s\n", server.c_str(), path.c_str());
				return 1;
			}
			// trim line
			new_url = new_url.substr(0, new_url.find_first_of("\r"));

			// separate URL into server and path
			string new_server = new_url.substr(7, new_url.find_first_of("/", 7)-7);
			string new_path = new_url.substr(new_url.find_first_of("/", 7));

			LOG("DLFile| got redirected: http://%s%s -> http://%s%s\n", server.c_str(), path.c_str(), new_server.c_str(), new_path.c_str());
			return downloadFile(localFile, new_server, new_path);
		}
		if (status_code != 200)
		{
			LOG("DLFile|Error: Response returned with status code: %d\n", status_code);
			LOG("DLFile|download URL: http://%s%s\n", server.c_str(), path.c_str());
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
				{
					reported_filesize = atoi(line.substr(16).c_str());
					if(predDownloadSize <= 0)
						predDownloadSize = reported_filesize;
				}
			}
		}

		// open local file late -> prevent creating emoty files
		myfile.open(localFile.string().c_str(), ios::out | ios::binary);
		if(!myfile.is_open())
		{
			LOG("DLFile|error opening local file: %d\n", localFile.string().c_str());
			LOG("DLFile|download URL: http://%s%s\n", server.c_str(), path.c_str());
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
			if(tdiff >= 0.5f)
			{
				downloadDoneSize += dataspeed;
				reportDownloadProgress(dlStartTime, predDownloadSize, downloadDoneSize);
				dataspeed=0;
				lastTime=time.elapsed();
			}

			dataspeed += data.size();
			datacounter += data.size();
			filesize_left -= data.size();
			myfile << &data;
		}

		// dont loose some last bytes
		if(dataspeed>0)
			downloadDoneSize += dataspeed;

		if (error != boost::asio::error::eof)
			throw boost::system::system_error(error);

		myfile.close();
		fileSize = file_size(localFile);
		socket.close();
		if(reported_filesize != 0 && fileSize != reported_filesize)
		{
			LOG("DLFile|Error: file size is different: should be %d, is %d. removing file.\n", reported_filesize, (int)fileSize);
			LOG("DLFile|download URL: http://%s%s\n", server.c_str(), path.c_str());

			tryRemoveFile(localFile);
			return 1;
		}

		// traffic stats tracking
		increaseServerStats(server, fileSize);
		
		if(fileSizeArg)
			*fileSizeArg = (int)fileSize;
	}
	catch (std::exception& e)
	{
		LOG("DLFile|Exception Error: %s\n", string(e.what()).c_str());
		LOG("DLFile|download URL: http://%s%s\n", server.c_str(), path.c_str());

		return 1;
	}
	LOG("DLFile| download ok, %d bytes received\n", fileSize);

	return 0;
}


void WsyncDownload::reportDownloadProgress(Timer dlStartTime, boost::uintmax_t predDownloadSize, boost::uintmax_t size_done)
{
	// this function will format and send some info to the gui
	unsigned int size_left = predDownloadSize - size_done;
	double tdiff = dlStartTime.elapsed();
	float speed = (float)(size_done / tdiff);
	float eta = size_left / speed;
	if(predDownloadSize == 0) eta = 0;
	float progress = ((float)size_done) /((float)predDownloadSize);
	char tmp[255]="";

	// update the progress bar
	updateCallback(MSE_UPDATE_PROGRESS, "", progress);

	string sizeDone = formatFilesize(size_done);
	string sizePredicted = formatFilesize(predDownloadSize);

	string timestr = formatSeconds(tdiff);
	updateCallback(MSE_UPDATE_TIME, timestr);

	if(eta > 10)
	{
		timestr = formatSeconds(eta);
		updateCallback(MSE_UPDATE_TIME_LEFT, timestr);
	} else if (eta != 0)
	{
		timestr = std::string("less than 10 seconds");
		updateCallback(MSE_UPDATE_TIME_LEFT, timestr);
	}

	string speedstr = formatFilesize((int)speed) + "/s";
	updateCallback(MSE_UPDATE_SPEED, speedstr);

	if(progress<1.0f)
	{
		char trafstr[256] = "";
		sprintf(trafstr, "%s / %s (%0.0f%%)", sizeDone.c_str(), sizePredicted.c_str(), progress * 100);
		updateCallback(MSE_UPDATE_TRAFFIC, string(trafstr));
	} else
	{
		string sizeOverhead = formatFilesize(size_done-predDownloadSize);
		char trafstr[256] = "";
		sprintf(trafstr, "%s (%s overhead)", sizeDone.c_str(), sizeOverhead.c_str());
		updateCallback(MSE_UPDATE_TRAFFIC, string(trafstr));
	}
}

void WsyncDownload::tryRemoveFile(boost::filesystem::path filename)
{
	LOG("removing file: %s ... ", filename.string().c_str());
	try
	{
		remove(filename);
		LOG("ok\n");
	} catch(...)
	{
		LOG("ERROR\n");
	}
}


void WsyncDownload::increaseServerStats(std::string server, boost::uintmax_t bytes)
{
	if(traffic_stats.find(server) == traffic_stats.end())
		traffic_stats[server] = 0;
	
	// add filesize to traffic stats
	traffic_stats[server] += bytes;
}

void WsyncDownload::updateCallback(int type, std::string txt, float percent)
{
	if(!this->parent) return;

	// send event
	MyStatusEvent ev(MyStatusCommandEvent, type);
	ev.text = wxString(txt.c_str(), wxConvUTF8);
	ev.progress = percent;
	this->parent->AddPendingEvent(ev);
}




int WsyncDownload::downloadAdvancedConfigFile(std::string server, std::string url, std::vector< std::map< std::string, std::string > > &list)
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
		return -2;
	}
	ifstream fin(tempfile.string().c_str());
	if (fin.is_open() == false)
	{
		printf("unable to open file for reading: %s\n", tempfile.string().c_str());
		return -3;
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
		LOG("error downloading file from %s, %s\n", server.c_str(), url.c_str());
		return -1;
	}
	return 0;
}


int WsyncDownload::downloadConfigFile(std::string server, std::string url, std::vector< std::vector< std::string > > &list)
{
	list.clear();

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
		list.push_back(args);
	}
	fin.close();
	WsyncDownload::tryRemoveFile(tempfile);
	return 0;
}

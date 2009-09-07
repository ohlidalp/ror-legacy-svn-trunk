// thomas fischer (thomas{AT}thomasfischer{DOT}biz) 6th of Janurary 2009

#ifndef WSYNC_H
#define WSYNC_H

#include "boost/asio.hpp"
#include "boost/filesystem.hpp"
#include <iostream> // for std::cout
#include <vector>
#include <string>
#include <fstream>


#define INDEXFILENAME  "files.index"

// servers
#define API_SERVER     "api.rigsofrods.com"

#ifndef REPO_SERVER
# define REPO_SERVER   "repository.rigsofrods.com"
#endif //REPO_SERVER


// functions
#define API_MIRROR     "/getwsyncmirror/"
#define API_REPOSEARCH "/reposearch/"
#define REPO_DOWNLOAD  "/files/mirror/geoselect/"

#define WMO_FULL     0x00000001
#define WMO_NODELETE 0x00000010

class Hashentry
{
public:
	std::string hash;
	boost::uintmax_t filesize;
	Hashentry()
	{
	}
	Hashentry(std::string hash, boost::uintmax_t filesize) : hash(hash), filesize(filesize)
	{
	}
};

class Fileentry
{
public:
	std::string filename;
	boost::uintmax_t filesize;
	Fileentry()
	{
	}
	Fileentry(std::string filename, boost::uintmax_t filesize) : filename(filename), filesize(filesize)
	{
	}
};

class WSync
{
public:
	WSync();
	~WSync();
	// main functions
	int sync(boost::filesystem::path localDir, std::string server, std::string remoteDir, bool useMirror=true, bool deleteOk=false);
	int createIndex(boost::filesystem::path localDir, int mode);

	// useful util functions
	int downloadFile(boost::filesystem::path localFile, std::string server, std::string remoteDir, bool displayProgress=false, bool debug=false);
	std::string generateFileHash(boost::filesystem::path file);
	static int getTempFilename(boost::filesystem::path &tempfile);

	int downloadMod(std::string modname, std::string &modfilename, boost::filesystem::path path, bool util=false);
	
	int downloadConfigFile(std::string server, std::string remoteDir, std::vector< std::vector< std::string > > &list);
	int downloadAdvancedConfigFile(std::string server, std::string path, std::vector< std::map< std::string, std::string > > &list);
protected:
	// members
	boost::uintmax_t downloadSize;

	// functions
	void listFiles(const boost::filesystem::path &dir_path, std::vector<std::string> &files);
	int buildFileIndex(boost::filesystem::path &outfilename, boost::filesystem::path &path, boost::filesystem::path &rootpath, std::map<std::string, Hashentry> &hashMap, bool writeFile=true, int mode=0);

	// hashmap utils
	int saveHashMapToFile(boost::filesystem::path &filename, std::map<std::string, Hashentry> &hashMap, int mode);
	int loadHashMapFromFile(boost::filesystem::path &filename, std::map<std::string, Hashentry> &hashMap, int &mode);

	// tools
	int ensurePathExist(boost::filesystem::path &path);
	std::string formatFilesize(boost::uintmax_t size);
	void progressOutput(float progress, float speed=-1, float eta=-1);
	void progressOutputShort(float progress);
	int cleanURL(std::string &url);
	std::string findHashInHashmap(std::map<std::string, Hashentry> hashMap, std::string filename);
};
#endif

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Request.hpp"
#include "Error.hpp"
#include "configParser.hpp"

#define SEND_CHUNK_SIZE 9000

#define NO_DATA_TO_UPLOAD (convert.find("POST") == 0 && convert.find(startBoundary) == std::string::npos)
#define IS_FOLDER_OR_FILE (stat((_info._configInfo._rootFolder +"/"+ _info._url).c_str(), &s) == 0)
#define IS_FOLDER (s.st_mode & S_IFDIR)
#define IS_FILE (s.st_mode & S_IFREG)
#define UPLOAD_FOLDER "root/upload/"


// TODO implement in config:
//#define REQUEST_TOO_BIG 413
//#define PATH_REQUEST_TOO_BIG "error/400.html"

class Request;

struct configInfo
{
    std::string _indexFile;
    std::string _rootFolder;
    bool _autoIndex;
    bool _postAllowed;
    bool _getAllowed;
    bool _deleteAllowed;
};

struct postInfo
{
    std::vector<uint8_t> _input;
    std::string     _filename;
    std::string     _boundary;
    std::ofstream*  _outfile;
    int _contentLen;
};

struct cgiInfo
{
	std::string _cgiPath;
	std::string _query;
	std::string _fileExtension;
	std::string _body;
};

struct clientInfo
{
    int         _clientSocket;
    httpMethod  _myHTTPMethod;
    std::string _url;
    std::string _fileContentType;
    std::string _contentType;
    bool        _isMultiPart;
    postInfo    _postInfo;
    configInfo  _configInfo;
	cgiInfo _cgiInfo;
    std::streampos _filePos;
    std::map<int,std::string> _errorMap;
	std::vector<std::string> _cgiFileExtension;
    int _globalStatusCode;
    bool _isChunkedFile;
};


class Response
{
    private:
        int _localStatusCode;
        clientInfo  _info;
        std::string _header;
        std::vector<uint8_t> _file;
        std::map<int, std::ofstream> _fileStreams;


    public:
        Response(int, const clientInfo&);
        ~Response();

        std::string getContentType();
        void        initHeader();
        int         initFile(int);
    std::streampos        mySend(int);
        int         getDirectoryIndexPage(const std::string&);
        void        sendIndexPage();
    std::streampos        sendRequestedFile();
        bool        uploadFile(const std::string&, const std::string&, std::ofstream*);
        bool        saveRequestToFile(std::ofstream&, const std::string&);
        void        deleteFile();

		bool	checkLanguage();
		int	validCGIextension();
		int		callCGI();
		bool	CGIoutput();
		bool isCgi();
		int inputCheck();

        std::vector<uint8_t> readFile(const std::string &fileName);
        int getRightResponse() const;
    void sendShittyChunk(const std::string&);
};

#endif

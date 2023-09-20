#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Request.hpp"
#include "Error.hpp"
#include "configParser.hpp"



#define NO_DATA_TO_UPLOAD (convert.find("POST") == 0 && convert.find(startBoundary) == std::string::npos)
#define IS_FOLDER_OR_FILE (stat((_info._configInfo._rootFolder +"/"+ _info._url).c_str(), &s) == 0)
#define IS_FOLDER (s.st_mode & S_IFDIR)
#define IS_FILE (s.st_mode & S_IFREG)
#define UPLOAD_FOLDER "root/upload/"

class Request;

struct configInfo
{
    std::string _indexFile; // set to index.html/php... or FAILURE
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
    std::map<int,std::string> _errorMap;
};

class Response
{
    private:
        int _statusCode;
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
        void        mySend(int);
        int         getDirectoryIndexPage(const std::string&);
        void        sendIndexPage();
        void        sendRequestedFile();
        bool        uploadFile(const std::string&, const std::string&, std::ofstream*);
        bool        saveRequestToFile(std::ofstream&, const std::string&);
        void        deleteFile();
};

#endif

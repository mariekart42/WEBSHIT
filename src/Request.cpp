
#include "../header/Request.hpp"

Request::Request(const std::vector<uint8_t>& clientData):
    _tmp(std::string(clientData.begin(), clientData.end())), _statusCode()
{}

Request::~Request() {}


/* extracts last file of URL and identifies what content it contains
 *  eg. found  ".pdf"  ==>  returns  "application/pdf"          */
std::string Request::getFileContentType(const std::string& url)
{
    if (url.find('.') != std::string::npos)
    {
        size_t startPos = url.find_last_of('.');
        size_t endPos = url.size();

        // from found till end next space:
        std::string fileExtension;

        if (endPos != std::string::npos)
            fileExtension = (url.substr(startPos + 1, endPos - (startPos)));
        else
            fileExtension = (url.substr(startPos));
        std::string contentType = comparerContentType(fileExtension);
        if (contentType == "FAILURE")
            return FAILURE;
        return (contentType);
    }
    return FAILURE;
}


bool Request::pathExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

bool Request::checkPathInFolder(const std::string& filePath, const std::string& rootFolder)
{
    //THIS WORKED BEFORE:
//    std::string fullPath = rootFolder + "/" + filePath;
//    return pathExists(fullPath);

    if (filePath.find('/') != std::string::npos) {
        std::string fullPath = rootFolder + "/" + filePath;
        return pathExists(fullPath);
    } else {
        return pathExists(rootFolder + "/" + filePath);
    }
}


bool Request::fileExists(const std::string& checkFilename, const std::string& uploadFolder)
{
    const std::string& filename = checkFilename;

    DIR* dir = opendir(uploadFolder.c_str());
    if (dir == nullptr)
    {
        Logging::log("Failed to open directory", 500);
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (strcmp(entry->d_name, filename.c_str()) == 0)
        {
            closedir(dir);
            return true;
        }
    }

    closedir(dir);
    return false;
}


std::string Request::getNewFilename(const std::string& checkFilename, const std::string& uploadFolder)
{
    size_t lastDotPos = checkFilename.rfind('.'); // Find the last dot position

    if (lastDotPos == std::string::npos) // If dot is found
        exitWithError("unexpected Error: could not getNewFilename [EXIT]");

    std::string filename = checkFilename.substr(0, lastDotPos); // Take substring up to the last dot
    std::string fileExtension = checkFilename.substr(lastDotPos, checkFilename.size());

    int fileCount = 1;
    while (fileExists(filename + "(" + std::to_string(fileCount) + ")" + fileExtension, uploadFolder))
        fileCount++;
    return (filename + "(" + std::to_string(fileCount) + ")"+ fileExtension);
}


/* only for POST && multipart
 * extracts the filename defined in the header of the body      */
std::string Request::getFileName(const std::string& contentType, const std::string& prevFileName, const std::string& uploadFolder)
{
    if (!prevFileName.empty() && prevFileName.compare(0, 13, "not_found_yet") != 0)
        return prevFileName;    // correct filename was already found

    if (contentType.compare(0, 34, "multipart/form-data") == 0)
    {
        size_t foundPos = _tmp.find("filename=\"");

        if (foundPos != std::string::npos)
        {
            size_t endPos = _tmp.find("\"\r\n", foundPos);
            if (endPos != std::string::npos)
            {
                std::string fileName = _tmp.substr(foundPos + 10, (endPos) - (foundPos + 10));
                if (fileName.rfind('.') == std::string::npos) // no file with .
                    return (FAILURE);
                if (fileExists(fileName, uploadFolder))
                {
                    fileName = getNewFilename(fileName, uploadFolder);

                }
                #ifdef DEBUG
                    std::cout << GRN"DEBUG: filename: " << fileName << ""RESET<< std::endl;
                #endif
                return fileName;
            }
            #ifdef DEBUG
                std::cout << "DEBUG: no filename found in POST request" << std::endl;
            #endif
            return (&"tmpFileForSocket_" [ random()]);
        }
        #ifdef DEBUG
            std::cout << "prolly first chunk of multipart, wait for filename"<<std::endl;
        #endif
        return "not_found_yet";  // error
    }

    return FAILURE; // not failure but we don't consider filename if not POST
}




/* only for POST 
 * returns the value as string after Content-Type:      */
std::string Request::getContentType()
{
    size_t foundPos = _tmp.find("Content-Type: ");

    if (foundPos != std::string::npos)
    {
        size_t endPos = _tmp.find(';', foundPos);// CHanged from ; to \r
        if (endPos != std::string::npos)
        {
            std::string contentType = _tmp.substr(foundPos + 14, endPos - (foundPos + 14));
            #ifdef DEBUG
                std::cout << GRN"DEBUG: Content-Type: " << contentType << ""RESET<< std::endl;
            #endif
            return contentType;
        }
    }
    return FAILURE;
}


/* only for POST && multipart
 * extracts boundary for multipart      */
std::string Request::getBoundary()
{
    size_t foundPos = _tmp.find("multipart/form-data; boundary=");

    if (foundPos != std::string::npos)
    {
        size_t endPos = _tmp.find('\r', foundPos);
        if (endPos != std::string::npos)
        {
            std::string contentType = _tmp.substr(foundPos + 30, endPos - (foundPos + 30));
            return contentType;
        }
    }
    return FAILURE;
}


httpMethod Request::getHTTPMethod()
{
    if (_tmp.compare(0, 3, "GET") == 0)
        return M_GET;
    else if (_tmp.compare(0, 4, "POST") == 0)
        return M_POST;
    else if (_tmp.compare(0, 6, "DELETE") == 0)
        return M_DELETE;
    else
        return M_error;
}


std::string Request::getUrlString()
{
    size_t startPos = _tmp.find('/', 0) + 1;
    size_t endPos = _tmp.find(' ', startPos);

    // MAKE URL LOWERCASE HERE
    if (endPos != std::string::npos){
        return (_tmp.substr(startPos, endPos - (startPos)));
    }
    else
        return (_tmp.substr(startPos));// dis was before
}

int Request::getPort()
{
    int lineStart = _tmp.find("Host: ") + 6; // This is the starting position
    std::string::size_type lineEnd = _tmp.find('\r', lineStart); // finds last char after startPos
    std::string hostLine = _tmp.substr(lineStart, lineEnd - lineStart);

    std::string::size_type lastCharPos = hostLine.rfind(':') + 1;

    // If the last character is found, create a substring between startPos and lastCharPos
    if (lastCharPos != std::string::npos)
    {
        std::string resultStr = hostLine.substr(lastCharPos, lineEnd - lastCharPos);
        return atoi(resultStr.c_str());
    }
    else
        exitWithError("unable to extract Port from request [EXIT]");
    return -1;
}


int Request::getStatusCode() const
{
    return _statusCode;
}


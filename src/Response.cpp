#include "../header/Response.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

Response::Response(const std::vector<uint8_t>& input, int clientSocket, const std::string& url)
{
    std::cout << "BEFORE doing anything" << std::endl;
    _info._input = input;
    _info._clientSocket = clientSocket;
    _info._statusCode = 200;
    _info._url = url;

}

Response::~Response()
{
    // for (std::map<int, std::ofstream>::iterator it = _fileStreams.begin(); it != _fileStreams.end(); ++it)
    // {
    //     it->second.close();
    // }
}
//Response::Response() {}


// FOR GET:
//  - url
//  - contentType
void Response::getResponse()
{
    sendRequestedFile();
}




bool Response::postResponse(std::string filename, int bytesLeft, std::string contentType, std::string boundary, int bytesRead, std::ofstream *outfile)
{
    _info._postInfo._filename = filename;
    _info._postInfo._bytesLeft = bytesLeft;
     _info._postInfo._boundary = boundary; // NEED LATER
    std::cout<< GRN"DEBUG: postResponse: content-Type: " << contentType<<""RESET<<std::endl;
    if (contentType == "multipart/form-data")
        return saveRequestToFile(bytesRead, *outfile);
    else if (contentType == "application/x-www-form-urlencoded")
    {
        std::cout << GRN"DEBUG: it's application/x-www-form-urlencoded"RESET<<std::endl;
        std::cout << GRN"DEBUG: filename: " << filename<<""RESET<<std::endl;
        urlDecodedInput();

    }
    return false;
}





std::string Response::getContentType()
{
   if (_info._url.find('.') != std::string::npos)
   {
       size_t startPos = _info._url.find_last_of('.');
       size_t endPos = _info._url.size();

       // from found till end next space:
       std::string fileExtension;

       if (endPos != std::string::npos)
           fileExtension = (_info._url.substr(startPos + 1, endPos - (startPos)));
       else
           fileExtension = (_info._url.substr(startPos));
       std::string contentType = comparerContentType(fileExtension);
       if (contentType == "FAILURE")
           mySend(404);
       return (contentType);
   }


   std::cout << RED"ERROR: is File but can't detect file extension"RESET<<std::endl;
   return FAILURE;
}


// if statusCode 200, _file NEEDS so be initialized!!
void Response::mySend(int statusCode)
{
    if (statusCode != 200)     // WRITE FUNCTION THAT RETURNS FILE SPECIFIED ON STATUS CODE
    {
        _info._fileContentType = "text/html";

        if (statusCode == FILE_SAVED)
        {
            statusCode = 200;
            _file = readFile(PATH_FILE_SAVED);
        }
        else if (statusCode == FILE_NOT_SAVED)
        {
            // statusCode = 500;
            statusCode = 200;
            _file = readFile(PATH_FILE_NOT_SAVED);
        }
        else if (statusCode == FILE_ALREADY_EXISTS)
        {
            // statusCode = 409;
            statusCode = 200;
            _file = readFile(PATH_FILE_ALREADY_EXISTS);
        }
        else if (statusCode == FILE_SAVED_AND_OVERWRITTEN)
        {
            statusCode = 200;
            _file = readFile(PATH_FILE_SAVED_AND_OVERWRITTEN);

        }
        else if (statusCode == DEFAULTWEBPAGE)
        {
            statusCode = 200;
            _file = readFile(PATH_DEFAULTWEBSITE);
        }
        else if (statusCode == 500)
            _file = readFile(PATH_500_ERRORWEBSITE);
        else if (statusCode == 404)
        {
            _file = readFile(PATH_404_ERRORWEBSITE);
            std::cout << RED"ERROR: 404 File not found"RESET << std::endl;   // LATER WRITE IN ERROR FILE
        }
        else if (statusCode == 6969)
            _file = readFile(PATH_HANDLEFOLDERSLATER);
        else
            exitWithError("status code not defined in mySend() [EXIT]");
    }
    else
    {
//        // can't find file extension
       _info._fileContentType = getContentType();
        if (_info._fileContentType == FAILURE)
            mySend(404);
    }
    std::string header = getHeader(statusCode);

    std::cout << "Response Hedaer:\n"GRN<<header<<""RESET<<std::endl;
    // std::cout << "Response Body:\n"GRN<<(_file.data())<<""RESET<<std::endl;
    std::cout << "Response Body:\n"GRN<<std::string(_file.begin(), _file.end())<<" ["<< _file.size() << "]"RESET<<std::endl;

    send(_info._clientSocket, header.c_str(), header.size(), 0);
//    send(_clientSocket, static_cast<const void*>(_file.data()), _file.size(), 0);
    send(_info._clientSocket, (std::string(_file.begin(), _file.end())).c_str(), _file.size(), 0);
}


std::string Response::getHeader(int statusCode)
{
    std::string header;

    header = "HTTP/1.1 " + std::to_string(statusCode) + " " +
            ErrorResponse::getErrorMessage(statusCode) + "\r\nConnection: close\r\n"
                                                         "Content-Type: "+_info._fileContentType+"\r\n"
                                                                                           "Content-Length: " + std::to_string(_file.size()) + "\r\n\r\n";
    return header;
}




void Response::sendDefaultWebpage()
{
    // _file = readFile(PATH_DEFAULTWEBSITE);
    // if (_file.empty())
    // {
    //     mySend(500);
    //     std::cout << RED"ERROR: unexpected Error, path to defaultWebsite wrong or no defaultWebsite provided"RESET << std::endl;   // LATER WRITE IN ERROR FILE
    //     return;
    // }
    mySend(DEFAULTWEBPAGE);
}


void Response::sendRequestedFile()
{
    if (_info._url == INDEX_PAGE)
        return (sendDefaultWebpage());

    struct stat s = {};
    int statusCode = OK;

    if (stat((SITE_FOLDER + _info._url).c_str(), &s) == 0)
    {
        if (IS_FOLDER)  //-> LATER if config is parsed
        {
            statusCode=6969;
            exitWithError("Cant handle Folders jet, do if config parser is done");
        }
        else if (IS_FILE)
        {
            _file = readFile(SITE_FOLDER + _info._url);
            if (_file.empty())   // if file doesn't exist
                statusCode = 404;
        }
        else
        {
            statusCode = 500;
            std::cout << RED"ERROR: unexpected Error in sendRequestedFile()"RESET << std::endl;   // LATER WRITE IN ERROR FILE
        }
    }
    else
        statusCode = 404;


    mySend(statusCode);
}

std::vector<uint8_t> Response::readFile(const std::string &fileName)
{
    std::ifstream file(fileName, std::ios::binary);

    if (!file)
    {
        std::cerr << "Failed to open file: " << fileName << std::endl;
        return static_cast<std::vector<uint8_t> >(0);
    }

    // Read the file content into a vector
    std::vector<uint8_t> content(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
    );

    return content;
}




// void Response::DELETEResponse() {std::cout << RED "DELETEResponse not working now!"RESET<<std::endl;}



// ^ ^ ^  GET   ^ ^ ^

// --- --- --- --- ---

// v v v  POST  v v v



// void Response::sendResponse()
// {
//     switch (_info._myHTTPMethod)
//     {
//         case M_GET:
//             sendRequestedFile();
//             break;
//         case M_POST:
//             POSTResponse();
//             break;
//         case M_DELETE:
//             DELETEResponse();
//             break;
//         default:
//             mySend(500);
//             exitWithError("unexpected Error: sendResponse can't identifies HTTPMethod [EXIT]");
//     }
// }


// // TODO: INIT LATER IF GET IS DONE
// void Response::POSTResponse()
// {
//     std::cout << RED "POST Response!"RESET<<std::endl;

// //    saveFile();
// //_file = readFile(PATH_500_ERRORWEBSITE);
//     saveRequestToFile();
// //    mySend(500);
// }


//std::string Response::getFileName(const Response::postInfo& info)
//{
//    std::string debug(_file.begin(), _file.end());
//    std::cout << "POST request:\n"RED <<  debug << RESET""<<std::endl;
//    if (info._filename.empty() || info._filename.compare(0, 16, ("tmpFileForSocket_")) == 0)
//    {
//        std::string tmp(_file.begin(), _file.end());
//        size_t foundPos = tmp.find("filename=");
//
//        if (foundPos != std::string::npos)
//        {
//            size_t endPos = tmp.find("\"", foundPos);
//            if (endPos != std::string::npos)
//            {
//                std::string requestedName = tmp.substr(foundPos + 10, endPos - foundPos + 10);  // NOT SURE ABOUT NUMBER
//
//                std::cout << GRN"DEBUG: filename: " << requestedName << ""RESET<< std::endl;
//                return requestedName;
//            }
//        }
//        std::cout << "DEBUG: no filename found in POST request" << std::endl;
//        return ("tmpFileForSocket_" + std::to_string(_clientSocket));
//    }
//    else
//        return info._filename;
//}



//size_t Response::getContentLen()
//{
//    std::string tmp(_file.begin(), _file.end());
//
//    size_t foundPos = tmp.find("Content-Length: ");
//
//    if (foundPos != std::string::npos)
//    {
//        size_t endPos = tmp.find("\n", foundPos);
//        if (endPos != std::string::npos)
//        {
//            std::string requestLenStr = tmp.substr(foundPos + 15, endPos - foundPos - 15);
//            size_t requestLen = static_cast<size_t>(std::strtol(requestLenStr.c_str(), nullptr, 10));
//
//            std::cout << GRN"DEBUG: Content-Length: " << requestLen << ""RESET<< std::endl;
//            return requestLen;
//        }
//    }
//
//    exitWithError("unexpected error: unable do get Content-Lenght from post request");
//    return -1;  // error
//}




//void Response::emptyClientPipe()
//{
////     //TODO: MAKE LOOP STOP LOL
////     ssize_t bytesRead = 1;
//// std::cout << GRN"in empty pipe"RESET<<std::endl;
////     while (bytesRead > 0)
////     {
////         char clientData[MAX_REQUESTSIZE];
////         memset(clientData, 0, MAX_REQUESTSIZE);
////         bytesRead = recv(_info._clientSocket, clientData, sizeof(clientData), O_NONBLOCK);
//// std::cout << GRN"lol"RESET<<std::endl;
////     }
////     _info._postInfo._bytesLeft = 0;
//
//ssize_t bytesRead = 0;
//    while (true)
//    {
//        uint8_t tempBuffer[MAX_REQUESTSIZE];
//        // memset(tempBuffer, 0, MAX_REQUESTSIZE);
//        bytesRead = read(_info._clientSocket, tempBuffer, sizeof(tempBuffer));
//std::cout << GRN"lol"RESET<<std::endl;
//
//        if (bytesRead == 0)
//        {
//            // Pipe is empty
//            break;
//        } else
//        {
//            // Error reading from the pipe
//            break;
//        }
//    }
//    std::cout <<GRN"DEBUG: end of empty client pipe with status["<<bytesRead<<"]"RESET<<std::endl;
//}

//void Response::initNewFileName()
//{
//    while (fileExistsInDirectory())
//        _info._postInfo._filename += "+";
//}
//


//
//bool Response::saveRequestToFile(int bytesRead)
//{
//
//}



bool Response::saveRequestToFile(int bytesRead, std::ofstream &outfile)
{

    std::string convert(_info._input.begin(), _info._input.end());
    std::string startBoundary = "--"+_info._postInfo._boundary+"\r\n";
    std::string endBoundary = "\r\n--"+_info._postInfo._boundary+"--";
    size_t boundaryPos;

    if (convert.find("POST") == 0 && convert.find(startBoundary) == std::string::npos) // only header, no body -> not writing to outfile!
        return true;
    else if ((boundaryPos = convert.find(startBoundary) != std::string::npos))  // cut header and put stuff afterward to outfile
    {
        size_t bodyHeaderPos = boundaryPos + startBoundary.size();

        size_t bodyPos = convert.find("\r\n\r\n", bodyHeaderPos+2) + 4; // maybe not right Pos


        for (std::vector<uint8_t>::iterator it = _info._input.begin() + bodyPos; it != _info._input.end(); it++)
            outfile << *it;

    }
    else if ((boundaryPos = convert.find(endBoundary) != std::string::npos))    // found last boundary
    {

        for (std::vector<uint8_t>::iterator it = _info._input.begin(); it != _info._input.begin() + boundaryPos; it++)
        {
            outfile << *it;
        }
        mySend(FILE_SAVED);
        return false;
    }
    else    // in the middle of body
    {
        for (std::vector<uint8_t>::iterator it = _info._input.begin(); it != _info._input.end(); it++)
            outfile << *it;
    }
    return true;

//
//
//    // print file content:
//    int fd = open((UPLOAD_FOLDER + _info._postInfo._filename).c_str(), O_RDONLY);
//
//    if (fd == -1) {
//        perror("Error opening file");
//        return 1;
//    }
//
//    char buffer[MAX_REQUESTSIZE];
//    ssize_t blob;
//
//    while ((blob = read(fd, buffer, sizeof(buffer))) > 0)
//    {
//        // Printing binary data as hexadecimal values
//        for (ssize_t i = 0; i < blob; ++i) {
//            std::cout << std::hex << static_cast<unsigned int>(buffer[i]) << " ";
//        }
//    }
//
//    close(fd);


}


bool Response::fileExistsInDirectory() const
{
    std::string filename = _info._postInfo._filename;

   DIR* dir = opendir(UPLOAD_FOLDER);
   if (dir == nullptr) {
       std::cerr << "Error opening directory: " << strerror(errno) << std::endl;
       return false;
   }

   struct dirent* entry;
   while ((entry = readdir(dir)) != nullptr) {
       if (strcmp(entry->d_name, filename.c_str()) == 0) {
           closedir(dir);
           return true;
       }
   }

   closedir(dir);
   return false;
}







// -------------------- SOME MARIE BULLSHIT ----------------------------------------

#ifdef ILLEGAL
void sendEMail(std::string emailAddress)
{
    const char* sender = "schwokelbaer@gmail.com";
    // const char* recipient = "recipient@example.com";

    // Compose the command
    std::string command = "sendmail ";
    command += emailAddress;
    command += " < /dev/null";

    // Execute the sendmail command
    int result = std::system(command.c_str());

    if (result == 0) {
        std::cout << "Email sent successfully." << std::endl;
    } else {
        std::cout << "Failed to send email." << std::endl;
    }

}
#endif
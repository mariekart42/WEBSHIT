#include "../header/connectClients.hpp"


ConnectClients::ConnectClients():
    _clientSocket(), _clientAddress(),
    _clientAddressLen(sizeof(_clientAddress)), _fdList()
{}


ConnectClients::~ConnectClients()
{
    printf("Closing client connection...\n");
}


void ConnectClients::initFdList(int serverSocket)
{
    int i = 0;

    for (; i < MAX_USERS; i++) {
        _fdList[i].fd = -1;         // File descriptor
        _fdList[i].events = 0;      // Set of events to monitor
        _fdList[i].revents = 0;     // Ready Event Set of Concerned Descriptors
    }
    i = 0;
    for (; i < MAX_USERS; i++) {
        if (_fdList[i].fd == -1)
        {
            _fdList[i].fd = serverSocket;
            _fdList[i].events = POLLIN;     // Concern about Read-Only Events
            break;
        }
    }
}



void ConnectClients::clientConnected(int serverSocket)
{
    for (size_t i = 0; i < MAX_USERS; ++i)
    {
        if (_fdList[i].revents & POLLIN)
        {
            std::cout << YEL " . . . Accepting Connection from Client" RESET << std::endl;
            _clientSocket = accept(serverSocket, (struct sockaddr *) &_clientAddress, &_clientAddressLen);
            if (_clientSocket < 0)
                exitWithError("Failed to init client Socket [EXIT]");

            char clientData[BUFFER_SIZE];
            ssize_t bytesRead = read(_clientSocket, clientData, sizeof(clientData));
            if (bytesRead > 0)
            {
                clientData[bytesRead] = '\0';
                std::cout << "\n=== " << bytesRead << " bytes REQUEST: \n" GRN << clientData << RESET << std::endl;


                Request request(clientData);
                Response response(request, _clientSocket);
                response.sendResponse();


                close(_clientSocket);
            }
            else if (bytesRead == 0)
            {
                // DO I HAVE TO RESPONSE ANYTHING?
                std::cout << "Connection closed by client" << std::endl;
                close(_fdList[i].fd);
                _fdList[i].fd = -1;
            }
            else
                exitWithError("unexpected error while reading data from client with read()");
        }
    }
}


void ConnectClients::connectClients(int serverSocket)
{
    initFdList(serverSocket);

    while (69)
    {
        switch (poll(_fdList, MAX_USERS, -1))
        {
            case -1:
                exitWithError("Failed to poll [EXIT]");
                break;
            case 0:
                exitWithError("poll returned 0, how to handle??");
//                std::cout << "poll returned 0, how to handle??" << std::endl;
                break;
            default:
                clientConnected(serverSocket);
                break;
        }
    }
}
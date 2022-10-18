#include "server.h"
#include "common.h"

namespace pipetrick
{

Server::Server(size_t maxClients) :
        maxNumberClients_(maxClients), currentNumberClients_(0)
{}

void Server::runClient(int *socketDescriptor)
{
    int socketClient = *socketDescriptor;
    char clientBuffer[BUFFER_SIZE];
    memset(clientBuffer, 0, sizeof(clientBuffer));

    if (!pipetrick::Common::readMessage(socketClient, clientBuffer))
    {
        return;
    }

    int sleepingTime = atoi(clientBuffer);

    std::this_thread::sleep_for(std::chrono::milliseconds(sleepingTime));
    strcpy(clientBuffer, std::to_string(++sleepingTime).c_str());

    if (!pipetrick::Common::writeMessage(socketClient, clientBuffer))
    {
        return;
    }

    close(socketClient);
}

bool Server::bind(int port)
{
    struct sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(port);
    if (::bind(serverSocketDescriptor_, (struct sockaddr*) &socketAddress, sizeof(socketAddress)) == -1)
    {
        int errorNumber = errno;
        std::cerr << "Could not bind to socket address. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return false;
    }

    return true;
}

bool Server::start(int port)
{
    const int LISTEN_BACKLOG = 1;

    if (!Common::createSocket(serverSocketDescriptor_))
    {
        return false;
    }

    int socketReuseOption = 1;
    if (setsockopt(serverSocketDescriptor_, SOL_SOCKET, SO_REUSEADDR, &socketReuseOption, sizeof(int)) == -1)
    {
        int errorNumber = errno;
        std::cerr << "Could not reuse the socket descriptor. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return false;
    }

    if (!bind(port))
    {
        return false;
    }

    if (listen(serverSocketDescriptor_, LISTEN_BACKLOG) == -1)
    {
        int errorNumber = errno;
        std::cerr << "Could not listen to socket. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return false;
    }

    serverThread_ = std::thread(&Server::run, this);
    return true;
}

void Server::run()
{
    while (1)
    {
        struct sockaddr_in clientAddress;
        int sizeofSockAddr = sizeof(struct sockaddr_in);

        int socketClientDescriptor = accept(serverSocketDescriptor_, (struct sockaddr*) &clientAddress, (socklen_t*) &sizeofSockAddr);
        if (socketClientDescriptor == -1)
        {
            int errorNumber = errno;
            std::cerr << "Could not accept on the socket descriptor. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            return;
        }

        std::thread(&Server::runClient, this, &socketClientDescriptor).detach();
    }
}

}

int main()
{
    pipetrick::Server server(90);
    server.start(8080);
    std::this_thread::sleep_for(std::chrono::seconds(90000));
    return 0;
}

#include "server.h"
#include "log.h"

namespace pipetrick
{

const std::chrono::milliseconds Server::MAX_TIME_TO_WAIT_FOR_CLIENTS_TO_FINISH = std::chrono::milliseconds(2000);

Server::Server(size_t maxClients) :
        maxNumberClients_(maxClients), currentNumberClients_(0), isRunning_(false), quitSignal_(true)
{
}

void Server::closeClientAndNotify(int socketClientDescriptor)
{
    std::unique_lock <std::mutex> lock(mutex_);
    close(socketClientDescriptor);
    currentNumberClients_--;
    clientsCV_.notify_all();
}

bool Server::sleep(char clientBuffer[BUFFER_SIZE])
{
    int sleepingTime = atoi(clientBuffer);
    std::unique_lock < std::mutex > lock(mutex_);
    if (quitSignal_)
    {
        Log::logVerbose("Server::sleep - Quit signal already raised, not sleeping.");
        return true;
    }

    clientsCV_.wait_for(lock, std::chrono::milliseconds(sleepingTime), [this]()
    {
        return quitSignal_;
    });

    strcpy(clientBuffer, std::to_string(++sleepingTime).c_str());
    return quitSignal_;
}

void Server::runClient(int socketClientDescriptor)
{
    fd_set writeFds;
    fd_set readFds;
    char clientBuffer[BUFFER_SIZE];
    memset(clientBuffer, 0, sizeof(clientBuffer));

    FD_ZERO(&readFds);
    FD_SET(socketClientDescriptor, &readFds);
    FD_SET(pipeDescriptors_[0], &readFds);

    if (Common::doSelect((pipeDescriptors_[0] > socketClientDescriptor ? pipeDescriptors_[0] : socketClientDescriptor) + 1, &readFds, nullptr, nullptr, "Server:") != SelectResult::OK)
    {
        Log::logError("Server::runClient - Error in the select operation when waiting for the client message with the sleeping time.");
        closeClientAndNotify(socketClientDescriptor);
        return;
    }

    if (FD_ISSET(pipeDescriptors_[0], &readFds))
    {
        Log::logVerbose("Server::runClient - Socket client closed by self pipe.");
        closeClientAndNotify(socketClientDescriptor);
        return;
    }

    if (!FD_ISSET(socketClientDescriptor, &readFds))
    {
        Log::logError("Server::runClient - Expected a client file descriptor ready to read operations.");
        closeClientAndNotify(socketClientDescriptor);
        return;
    }

    if (!Common::readMessage(socketClientDescriptor, clientBuffer, "Server:"))
    {
        Log::logError("Server::runClient - Error reading the client message with the sleeping time.");
        closeClientAndNotify(socketClientDescriptor);
        return;
    }

    if (sleep(clientBuffer))
    {
        Log::logVerbose("Server::runClient - Client will be closed after the sleeping time. No writing back to them.");
        closeClientAndNotify(socketClientDescriptor);
        return;
    }

    FD_ZERO(&writeFds);
    FD_SET(socketClientDescriptor, &writeFds);

    if (Common::doSelect(socketClientDescriptor + 1, nullptr, &writeFds, nullptr, "Server:") != SelectResult::OK)
    {
        Log::logError("Server::runClient - Error in the select operation when writing the increased sleeping time to the client.");
        closeClientAndNotify(socketClientDescriptor);
        return;
    }

    if (!FD_ISSET(socketClientDescriptor, &writeFds))
    {
        Log::logError("Server::runClient - Expected a client file descriptor ready to write operations.");
        closeClientAndNotify(socketClientDescriptor);
        return;
    }

    if (!Common::writeMessage(socketClientDescriptor, clientBuffer, "Server:"))
    {
        Log::logError("Server::runClient - Error writing to the client message the increased sleeping time.");
    }
    closeClientAndNotify(socketClientDescriptor);
}

bool Server::bindAndListen(int port)
{
    struct sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(port);

    int socketReuseOption = 1;
    if (setsockopt(serverSocketDescriptor_, SOL_SOCKET, SO_REUSEADDR, &socketReuseOption, sizeof(int)) == -1)
    {
        int errorNumber = errno;
        Log::logError("Server::start - Could not reuse the socket descriptor", errorNumber);
        return false;
    }

    if (::bind(serverSocketDescriptor_, (struct sockaddr*) &socketAddress, sizeof(socketAddress)) == -1)
    {
        int errorNumber = errno;
        Log::logError("Server::bind - Could not bind to socket address", errorNumber);
        return false;
    }

    const int LISTEN_BACKLOG = 550;
    if (listen(serverSocketDescriptor_, LISTEN_BACKLOG) == -1)
    {
        int errorNumber = errno;
        Log::logError("Server::start - Could not listen to socket", errorNumber);
        return false;
    }

    return true;
}

bool Server::start(int port)
{
    if (!Common::createSocket(serverSocketDescriptor_, SOCK_NONBLOCK, "Server:"))
    {
        return false;
    }

    if (pipe2(pipeDescriptors_, O_NONBLOCK) == -1)
    {
        int errorNumber = errno;
        Log::logError("Server::Server - Could not create the pipe file descriptors", errorNumber);
        return false;
    }

    if (!bindAndListen(port))
    {
        return false;
    }

    isRunning_ = true;
    quitSignal_ = false;
    serverThread_ = std::thread(&Server::run, this);
    return true;
}

void Server::stop()
{
    quitRunningThread();
    waitForRunningThread();
    serverThread_.join();
    close(serverSocketDescriptor_);
    close(pipeDescriptors_[0]);
    close(pipeDescriptors_[1]);
}

void Server::waitForRunningThread()
{
    std::unique_lock < std::mutex > lock(mutex_);
    auto quitPredicate = [this]()
    {
        return !isRunning_;
    };

    if (!clientsCV_.wait_for(lock, MAX_TIME_TO_WAIT_FOR_CLIENTS_TO_FINISH, quitPredicate))
    {
        Log::logError("Server::stop - Time out expired when waiting for the running thread to finish!!!");
    }
    else
    {
        Common::consumePipe(pipeDescriptors_[0], "Server:");
    }
}

void Server::quitRunningThread()
{
    std::unique_lock <std::mutex> lock(mutex_);
    write(pipeDescriptors_[1], "0", 1);
    quitSignal_ = true;
    clientsCV_.notify_all();
}

bool Server::doAccept()
{
    struct sockaddr_in clientAddress;
    int sizeofSockAddr = sizeof(struct sockaddr_in);

    int socketClientDescriptor = accept4(serverSocketDescriptor_, (struct sockaddr*) &clientAddress, (socklen_t*) &sizeofSockAddr, SOCK_NONBLOCK);
    if (socketClientDescriptor == -1)
    {
        int errorNumber = errno;
        if (errorNumber == EMFILE)
        {
            Log::logError("Server::doAccept - The system reached the maximum number of open files."); 
            return true; //This client is not attended, but the server is kept alive
        }
        Log::logError("Server::doAccept - Could not accept on the socket descriptor", errorNumber);
        return false;
    }

    if (checkForMaximumNumberClients())
    {
        Log::logVerbose("Server::doAccept - Quit signal was raised while waiting for the current number of clients to decrease.");
        return false;
    }

    std::unique_lock < std::mutex > lock(mutex_);
    currentNumberClients_++;
    std::thread(&Server::runClient, this, socketClientDescriptor).detach();
    return true;
}

bool Server::checkForMaximumNumberClients()
{
    std::unique_lock < std::mutex > lock(mutex_);
    if (currentNumberClients_ >= maxNumberClients_)
    {
        Log::logVerbose("Server::checkForMaximumNumberClients - The maximum number of clients has been reached. Waiting until one client finishes.");
        if (currentNumberClients_ > maxNumberClients_)
        {
            Log::logError("Server::checkForMaximumNumberClients - The current number of clients is way beyond the maximum number allowed. This should never happen!!!");
        }
    
        clientsCV_.wait(lock, [this]()
        {
            return (currentNumberClients_ < maxNumberClients_) || quitSignal_;
        });
    }

    return quitSignal_;
}

void Server::waitForClientsToFinish()
{
    std::unique_lock <std::mutex> lock(mutex_);
    auto clientsToFinishPredicate = [this]()
    {
        return currentNumberClients_ == 0;
    };

    if(currentNumberClients_ == 0)
    {
        Log::logVerbose("Server::waitForClientsToFinish - No clients connected.");
    }
    else if(!clientsCV_.wait_for(lock, MAX_TIME_TO_WAIT_FOR_CLIENTS_TO_FINISH, clientsToFinishPredicate))
    {
        Log::logError("Server::waitForClientsToFinish  - Time out expired when waiting for all the clients to finish!!!");
    }

    isRunning_ = false;
    clientsCV_.notify_all();
}

void Server::run()
{
    bool quit = false;
    while (!quit)
    {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(serverSocketDescriptor_, &readFds);
        FD_SET(pipeDescriptors_[0], &readFds);

        if (Common::doSelect((pipeDescriptors_[0] > serverSocketDescriptor_ ? pipeDescriptors_[0] : serverSocketDescriptor_) + 1, &readFds, nullptr, nullptr, "Server:") != SelectResult::OK)
        {
            quit = true;
        }
        else if (FD_ISSET(pipeDescriptors_[0], &readFds))
        {
            Log::logVerbose("Server::run - Quitting server main loop by the self pipe trick.");
            quit = true;
        }
        else if (FD_ISSET(serverSocketDescriptor_, &readFds))
        {
            if (!doAccept())
            {
                quit = true;
            }
        }
    }

    quitRunningThread();
    waitForClientsToFinish();
}

size_t Server::getNumberOfClients() const
{
    std::unique_lock < std::mutex > lock(mutex_);
    return currentNumberClients_;
}

}

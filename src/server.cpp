#include <sys/ioctl.h>
#include "server.h"
#include "log.h"

namespace pipetrick
{

const std::chrono::milliseconds Server::MAX_TIME_TO_WAIT_FOR_CLIENTS_TO_FINISH = std::chrono::milliseconds(2000);

Server::Server(size_t maxClients) 
: maxNumberClients_(maxClients)
, currentNumberClients_(0)
, serverSocketDescriptor_(-1)
, isRunning_(false)
, quitSignal_(true)
{
}

void Server::closeClientAndNotify(int socketClientDescriptor)
{
#ifdef WITH_PTHREADS
    pthread_mutex_lock(&mutex_);
    close(socketClientDescriptor);
    currentNumberClients_--;
    pthread_cond_broadcast(&clientsCV_);
    pthread_mutex_unlock(&mutex_);
#else
    std::scoped_lock lock(mutex_);
    close(socketClientDescriptor);
    currentNumberClients_--;
    clientsCV_.notify_all();
#endif
}

bool Server::sleep(int socketClientDescriptor, char clientBuffer[BUFFER_SIZE])
{
    int sleepingTime = atoi(clientBuffer);
    std::chrono::microseconds selectTimeOut(sleepingTime * 1000);
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(socketClientDescriptor, &readFds);
    FD_SET(pipeDescriptors_[0], &readFds);

    SelectResult result = Common::doSelect((pipeDescriptors_[0] > socketClientDescriptor ? pipeDescriptors_[0] : socketClientDescriptor) + 1, &readFds, nullptr, &selectTimeOut, "Server:");
    if (result == SelectResult::TIMEOUT)
    {
        strcpy(clientBuffer, std::to_string(++sleepingTime).c_str());
        return false;
    }

    if (result == SelectResult::ERROR)
    {
        Log::logError("Server::sleep - Select call failed.");
        return true;
    }

    if (FD_ISSET(pipeDescriptors_[0], &readFds))
    {
        Log::logVerbose("Server::sleep - stop was called while sleeping.");
        return true;
    }

    if (!FD_ISSET(socketClientDescriptor, &readFds))
    {
        Log::logError("Server::sleep - select returned with no error, no files ready to be read and before the sleeping time!!!");
        return true;
    }

    //Let's check if the remote peer closed the connection
    int bytesAvailable;
    if (ioctl(socketClientDescriptor, FIONREAD, &bytesAvailable) == -1)
    {
        int errorNumber = errno;
        Log::logError("Server::sleep - ioctel failed when checking if the remote peer closed the connection", errorNumber);
        return true;
    }

    if (bytesAvailable == 0) //The remote peer closed the connection.
    {
        Log::logVerbose("Server::sleep - the remote peer closed the connection.");
    }

    return true;
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

    if (sleep(socketClientDescriptor, clientBuffer))
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

#ifdef WITH_PTHREADS
void* Server::runHelper(void *context)
{
    Server* self = static_cast<Server*>(context);
    self->run();
    pthread_exit(NULL);
    return NULL;
}

void* Server::runClientHelper(void* context)
{
    clientArgs* args = static_cast<clientArgs*>(context);
    args->self->runClient(args->socketClientDescriptor);
    delete args;
    pthread_exit(NULL);
    return NULL;
}
#endif

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
#ifdef WITH_PTHREADS
    if (pthread_mutex_init(&mutex_, NULL) != 0)
    {
        int errorNum = errno;
        Log::logError("Server::Server - Error initializing mutex", errorNum);
        return false;
    }

    if (pthread_cond_init(&clientsCV_, NULL) != 0)
    {
        int errorNum = errno;
        Log::logError("Server::Server - Error initializing condition variable", errorNum);
        return false;
    }

    if (pthread_create(&serverThread_, NULL, &Server::runHelper, this) != 0)
    {
        int errorNum = errno;
        Log::logError("Server::start - Could not create the server thread.", errorNum);
        return false;
    }
#else
    serverThread_ = std::thread(&Server::run, this);
#endif
    return true;
}

void Server::stop()
{
    quitRunningThread();
    waitForRunningThread();
#ifdef WITH_PTHREADS
    if (pthread_join(serverThread_, NULL) != 0)
    {
        int errorNum = errno;
        Log::logError("Server::stop - Could not join the server thread.", errorNum);
    }

    if (pthread_cond_destroy(&clientsCV_) != 0)
    {
        int errorNum = errno;
        Log::logError("Server::stop - Error destroying condition variable", errorNum);
    }

    if (pthread_mutex_destroy(&mutex_) != 0)
    {
        int errorNum = errno;
        Log::logError("Server::stop - Error destroying the mutex", errorNum);
    }

#else
    serverThread_.join();
#endif
    close(serverSocketDescriptor_);
    close(pipeDescriptors_[0]);
    close(pipeDescriptors_[1]);
}

void Server::waitForRunningThread()
{
#ifdef WITH_PTHREADS
    pthread_mutex_lock (&mutex_);
    int error = 0;
    while(isRunning_ == true && error == 0)
    {
        error = pthread_cond_wait (&clientsCV_, &mutex_); //TODO check time out
    }

    if (error != 0)
    {
        int errorNum = error;
        Log::logError("Server::waitForRunningThread - Error waiting for the running thread to finish. ", errorNum);
    }

    Common::consumePipe(pipeDescriptors_[0], "Server:");
    pthread_mutex_unlock (&mutex_);
#else
    std::unique_lock < std::mutex > lock(mutex_);
    auto quitPredicate = [this]()
    {
        return !isRunning_;
    };

    if (!clientsCV_.wait_for(lock, MAX_TIME_TO_WAIT_FOR_CLIENTS_TO_FINISH, quitPredicate))
    {
        Log::logError("Server::waitForRunningThread - Time out expired when waiting for the running thread to finish!!!");
    }
    else
    {
        Common::consumePipe(pipeDescriptors_[0], "Server:");
    }
#endif
}

void Server::quitRunningThread()
{
#ifdef WITH_PTHREADS
    pthread_mutex_lock(&mutex_);
    write(pipeDescriptors_[1], "0", 1);
    quitSignal_ = true;
    pthread_cond_broadcast(&clientsCV_);
    pthread_mutex_unlock(&mutex_);
#else
    std::scoped_lock lock(mutex_);
    write(pipeDescriptors_[1], "0", 1);
    quitSignal_ = true;
    clientsCV_.notify_all();
#endif
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
#ifdef WITH_PTHREADS
    pthread_mutex_lock(&mutex_);
    currentNumberClients_++;
    pthread_mutex_unlock(&mutex_);
    pthread_t clientThread;
    clientArgs* args = new clientArgs;
    args->socketClientDescriptor = socketClientDescriptor;
    args->self = this;
    pthread_create(&clientThread, NULL, &Server::runClientHelper, args);
    pthread_detach(clientThread);
#else
    std::scoped_lock lock(mutex_);
    currentNumberClients_++;
    std::thread(&Server::runClient, this, socketClientDescriptor).detach();
#endif
    return true;
}

bool Server::checkForMaximumNumberClients()
{
#ifdef WITH_PTHREADS
    pthread_mutex_lock(&mutex_);
#else
    std::unique_lock < std::mutex > lock(mutex_);
#endif
    if (currentNumberClients_ >= maxNumberClients_)
    {
        Log::logVerbose("Server::checkForMaximumNumberClients - The maximum number of clients has been reached. Waiting until one client finishes.");
        if (currentNumberClients_ > maxNumberClients_)
        {
            Log::logError("Server::checkForMaximumNumberClients - The current number of clients is way beyond the maximum number allowed. This should never happen!!!");
        }
#ifdef WITH_PTHREADS
        int error = 0;
        while(quitSignal_ == false && currentNumberClients_ >= maxNumberClients_ && error == 0)
        {
            error = pthread_cond_wait (&clientsCV_, &mutex_); //TODO check time out
        }

        if (error != 0)
        {
            Log::logError("Server::checkForMaximumNumberClients - Error when waiting for quitSignal to be raised");
        }
#else
        clientsCV_.wait(lock, [this]()
        {
            return (currentNumberClients_ < maxNumberClients_) || quitSignal_;
        });
#endif
    }
#ifdef WITH_PTHREADS
    bool quitSignalAux = quitSignal_;
    pthread_mutex_unlock(&mutex_);
    return quitSignalAux;
#else
    return quitSignal_;
#endif
}

void Server::waitForClientsToFinish()
{
#ifdef WITH_PTHREADS
    pthread_mutex_lock(&mutex_);
    if(currentNumberClients_ == 0)
    {
        Log::logVerbose("Server::waitForClientsToFinish - No clients connected.");
    }
    else
    {
        int error = 0;
        while(currentNumberClients_ > 0 && error == 0)
        {
            error = pthread_cond_wait (&clientsCV_, &mutex_);
        }

        if (error != 0)
        {
            Log::logError("Server::waitForClientsToFinish  - Error when waiting for the number of clients to become 0. Number of clients = " + std::to_string(currentNumberClients_));
        }
    }
    isRunning_ = false;
    pthread_cond_broadcast(&clientsCV_);
    pthread_mutex_unlock(&mutex_);
#else
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
        Log::logError("Server::waitForClientsToFinish  - Time out expired when waiting for all the clients to finish. There are still some clients connected!!!");
    }

    isRunning_ = false;
    clientsCV_.notify_all();
#endif
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
#ifdef WITH_PTHREADS
    pthread_mutex_lock(&mutex_);
    size_t value = currentNumberClients_;
    pthread_mutex_unlock(&mutex_);
    return value;
#else
    std::scoped_lock lock(mutex_);
    return currentNumberClients_;
#endif
}

}

#include "log.h"
#include "client.h"

namespace pipetrick
{

const char *Client::DEFAULT_IP = "127.0.0.1";
const std::chrono::milliseconds Client::MAXIMUM_WAITING_TIME_FOR_FLAG = std::chrono::milliseconds(2000);
const std::chrono::microseconds Client::DEFAULT_TIMEOUT = std::chrono::microseconds(5 * 1000 * 1000);

Client::Client(const std::chrono::microseconds& timeOut, const char* serverIP, int port) :
         timeOut_(timeOut), serverIP_(serverIP), serverPort_(port), isRunning_(false)
{
    pipeDescriptors_[0] = -1;
    pipeDescriptors_[1] = -1;
    if (pipe2(pipeDescriptors_, O_NONBLOCK) == -1)
    {
        int errorNumber = errno;
        Log::logError("Client::Client - Could not create the pipe file descriptors", errorNumber);
    }
}

Client::~Client()
{
    close(pipeDescriptors_[0]);
    close(pipeDescriptors_[1]);
}

void Client::stop()
{
    writeToPipeAndWait();
    Common::consumePipe(pipeDescriptors_[0], "Client:");
}

void Client::writeToPipeAndWait()
{
    std::unique_lock < std::mutex > lock(mutex_);
    if (!isRunning_)
    {
        Log::logVerbose("Client::writeToPipeAndWait - Client is not running.");
        return;
    }

    if (write(pipeDescriptors_[1], "0", 1) == -1)
    {
        int errorNumber = errno;
        Log::logError("Client::writeToPipeAndWait - Error writing to the pipe.", errorNumber);
    }
    auto quitPredicate = [this]()
    {
        return !isRunning_;
    };

    if (!quitCV_.wait_for(lock, MAXIMUM_WAITING_TIME_FOR_FLAG, quitPredicate))
    {
        Log::logError("Client::stop - Time out expired when waiting for pending connections to finish!!!");
        return;
    }
}

void Client::closeAndNotify(int socketDescriptor)
{
    std::unique_lock < std::mutex > lock(mutex_);
    if (!isRunning_)
    {
        Log::logVerbose("Client::closeAndNotify - Client is not running.");
    }
    close(socketDescriptor);
    isRunning_ = false;
    quitCV_.notify_all();
}

bool Client::connectToServer(int socketDescriptor)
{
    struct sockaddr_in serverAddress;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP_.c_str());
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort_);

    if (connect(socketDescriptor, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1)
    {
        int errorNumber = errno;
        if (errorNumber != EINPROGRESS)
        {
            Log::logError("Client::connectToServer - Could not connect to the server", errorNumber);
            return false;
        }
    }

    return true;
}

bool Client::checkPipeDescriptorsAndRun()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (pipeDescriptors_[0] == -1 || pipeDescriptors_[1] == -1)
    {
        int errorNumber = errno;
        Log::logError("Client::Client - Could not create the pipe file descriptors", errorNumber);
        return false;
    }

    isRunning_ = true;
    return true;
}

bool Client::sendDelayToServer(std::chrono::milliseconds& serverDelay)
{
    int socketDescriptor;
    if (!Common::createSocket(socketDescriptor, SOCK_NONBLOCK, "Client:") || !checkPipeDescriptorsAndRun())
    {
        return false;
    }
    
    if (!connectToServer(socketDescriptor))
    {
        closeAndNotify(socketDescriptor);
        return false;
    }

    fd_set writeFds;
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(pipeDescriptors_[0], &readFds);
    FD_ZERO(&writeFds);
    FD_SET(socketDescriptor, &writeFds);

    if (Common::doSelect((pipeDescriptors_[0] > socketDescriptor ? pipeDescriptors_[0] : socketDescriptor) + 1, &readFds, &writeFds, &timeOut_, "Client:") != SelectResult::OK)
    {
        closeAndNotify(socketDescriptor);
        return false;
    }

    if (FD_ISSET(pipeDescriptors_[0], &readFds)) //Another thread wrote to the 'write' end of the pipe.
    {
        Log::logVerbose("Client::sendDelayToServer - Quit client in the connect operation by the self pipe trick");
        closeAndNotify(socketDescriptor);
        return false;
    }

    if (!FD_ISSET(socketDescriptor, &writeFds))
    {
        Log::logError("Client::sendDelayToServer - Expected a file descriptor ready to write operations.");
        closeAndNotify(socketDescriptor);
        return false;
    }

    char message[BUFFER_SIZE];
    memset(message, 0, sizeof(message));
    strcpy(message, std::to_string(serverDelay.count()).c_str());
    if (!Common::writeMessage(socketDescriptor, message, "Client:"))
    {
        Log::logError("Client::sendDelayToServer - Could not send the delay to the server.");
        closeAndNotify(socketDescriptor);
        return false;
    }

    FD_ZERO(&readFds);
    FD_SET(socketDescriptor, &readFds);
    FD_SET(pipeDescriptors_[0], &readFds);

    if (Common::doSelect((pipeDescriptors_[0] > socketDescriptor ? pipeDescriptors_[0] : socketDescriptor) + 1, &readFds, nullptr, &timeOut_, "Client:") != SelectResult::OK)
    {
        closeAndNotify(socketDescriptor);
        return false;
    }

    if (FD_ISSET(pipeDescriptors_[0], &readFds)) //Another thread wrote to the 'write' end of the pipe.
    {
        Log::logVerbose("Client::sendDelayToServer - Quit client in the read operation by the self pipe trick");
        closeAndNotify(socketDescriptor);
        return false;
    }

    if (!FD_ISSET(socketDescriptor, &readFds))
    {
        Log::logError("Client::sendDelayToServer - Expected a file descriptor ready to read operations.");
        closeAndNotify(socketDescriptor);
        return false;
    }

    memset(message, 0, sizeof(message));
    if (!Common::readMessage(socketDescriptor, message, "Client:"))
    {
        Log::logError("Client::sendDelayToServer - Could not get the increased delay from the server.");
        closeAndNotify(socketDescriptor);
        return false;
    }

    serverDelay = std::chrono::milliseconds(atoi(message));

    closeAndNotify(socketDescriptor);
    return true;
}

}
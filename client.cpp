#include <arpa/inet.h>
#include <fcntl.h>
#include "client.h"
#include "common.h"
#define DEFAULT_TIME_OUT_MSECONDS 4000000

namespace pipetrick
{

const char *Client::DEFAULT_IP = "127.0.0.1";
const int Client::DEFAULT_PORT = 8080;
const std::chrono::milliseconds Client::DEFAULT_DELAY = std::chrono::milliseconds(1000);
const std::chrono::milliseconds Client::MAXIMUM_WAITING_TIME_FOR_FLAG = std::chrono::milliseconds(2000);

Client::Client(const char *serverIP, int port) :
        serverIP_(serverIP), serverPort_(port), isRunning_(false)
{
    int errorNumber;
    if (pipe2(pipeDescriptors_, O_NONBLOCK) == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not create the pipe file descriptors. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
    }
}

bool Client::createSocket()
{
    socketDescriptor_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socketDescriptor_ == -1)
    {
        int errorNumber = errno;
        std::cerr << "Could not create the socket server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return false;
    }

    return true;
}

void Client::stop()
{
    std::unique_lock < std::mutex > lock(mutex_);
    if (!isRunning_.load())
    {
        return;
    }

    write(pipeDescriptors_[1], "0", 1);
    auto quitPredicate = [this]()
    {
        return !isRunning_.load();
    };

    if (!quitCV_.wait_for(lock, MAXIMUM_WAITING_TIME_FOR_FLAG, quitPredicate))
    {
        std::cerr << "Time out expired when waiting for pending connections to finish!!!" << std::endl;
    }
}

void Client::closeSocketAndNotify()
{
    std::unique_lock < std::mutex > lock(mutex_);
    close(socketDescriptor_);
    isRunning_.exchange(false);
    quitCV_.notify_one();
}

bool Client::connectToServer()
{
    struct sockaddr_in serverAddress;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP_.c_str());
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort_);
    int errorNumber;

    if (connect(socketDescriptor_, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1)
    {
        errorNumber = errno;
        if (errorNumber != EINPROGRESS)
        {
            std::cerr << "Could not connect to the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            return false;
        }
    }

    return true;
}

void Client::consumePipe() const
{
    bool done = false;
    while (!done)
    {
        char ch;
        if (read(pipeDescriptors_[0], &ch, 1) == -1)
        {
            int errorNumber = errno;
            if (errorNumber != EAGAIN)
            {
                std::cerr << "Error reading all bytes from the pipe. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
                return;
            }
            done = true;
        }
    }
}

bool Client::sendDelayToServer(const std::chrono::milliseconds &serverDelay)
{
    char message[BUFFER_SIZE];
    fd_set writeFds;
    fd_set readFds;

    std::chrono::microseconds timeOut(DEFAULT_TIME_OUT_MSECONDS);
    isRunning_.exchange(true);

    if (!createSocket() || !connectToServer())
    {
        closeSocketAndNotify(); //TODO check if this is a problem.
        return false;
    }

    consumePipe(); //To consume data from the read side of the pipe produced on previous calls to this method.

    FD_ZERO(&readFds);
    FD_SET(socketDescriptor_, &readFds);
    FD_SET(pipeDescriptors_[0], &readFds);
    FD_ZERO(&writeFds);
    FD_SET(socketDescriptor_, &writeFds);

    if (!Common::doSelect((pipeDescriptors_[0] > socketDescriptor_ ? pipeDescriptors_[0] : socketDescriptor_) + 1, &readFds, &writeFds, &timeOut))
    {
        closeSocketAndNotify();
        return false;
    }

    if (FD_ISSET(pipeDescriptors_[0], &readFds)) //Another thread wrote to the 'write' end of the pipe.
    {
        consumePipe();
        closeSocketAndNotify();
        return false;
    }
    else if (FD_ISSET(socketDescriptor_, &writeFds))
    {
        memset(message, 0, sizeof(message));
        strcpy(message, std::to_string(serverDelay.count()).c_str());
        if (!Common::writeMessage(socketDescriptor_, message))
        {
            closeSocketAndNotify();
            return false;
        }
    }

    FD_ZERO(&readFds);
    FD_SET(socketDescriptor_, &readFds);
    FD_SET(pipeDescriptors_[0], &readFds);

    if (!Common::doSelect((pipeDescriptors_[0] > socketDescriptor_ ? pipeDescriptors_[0] : socketDescriptor_) + 1, &readFds, nullptr, &timeOut))
    {
        closeSocketAndNotify();
        return false;
    }

    if (FD_ISSET(pipeDescriptors_[0], &readFds)) //Another thread wrote to the 'write' end of the pipe.
    {
        consumePipe();
        closeSocketAndNotify();
        return false;
    }
    else if (FD_ISSET(socketDescriptor_, &readFds))
    {
        memset(message, 0, sizeof(message));
        if (!Common::readMessage(socketDescriptor_, message))
        {
            closeSocketAndNotify();
            return false;
        }
    }

    closeSocketAndNotify();
    return true;
}

Client::~Client()
{
    close(pipeDescriptors_[0]);
    close(pipeDescriptors_[1]);
}

}

int main(int argc, char *argv[])
{
    pipetrick::Client client;

    std::thread threadClient([&client]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2200));
        client.stop();
    });

    client.sendDelayToServer(std::chrono::milliseconds(atoi(argv[1])));

    threadClient.join();

    return 0;
}

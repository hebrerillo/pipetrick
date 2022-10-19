#include "common.h"

namespace pipetrick
{

bool Common::createSocket(int &socketDescriptor, int flags)
{
    socketDescriptor = socket(AF_INET, SOCK_STREAM | flags, 0);
    if (socketDescriptor == -1)
    {
        int errorNumber = errno;
        std::cerr << "Could not create the socket. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return false;
    }

    return true;
}

Common::SelectResult Common::doSelect(int maxFileDescriptor, fd_set *readFds, fd_set *writeFds, const std::chrono::microseconds *timeOut)
{

    if (!readFds && !writeFds)
    {
        std::cout << "No write or read file descriptors were provided." << std::endl;
        return SelectResult::ERROR;
    }

    int retValue;
    if (timeOut)
    {
        struct timeval timeOutSelect;
        timeOutSelect.tv_sec = 0;
        timeOutSelect.tv_usec = timeOut->count();
        retValue = select(maxFileDescriptor, readFds, writeFds, NULL, &timeOutSelect);
    }
    else
    {
        retValue = select(maxFileDescriptor, readFds, writeFds, NULL, NULL);
    }

    if (retValue == -1)
    {
        int errorNumber = errno;
        std::cerr << "select failed. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return SelectResult::ERROR;
    }

    if (retValue == 0)
    {
        std::cerr << "Time out expired" << std::endl;
        return SelectResult::TIMEOUT;
    }

    return SelectResult::OK;
}

bool Common::readMessage(int socketDescriptor, char buffer[BUFFER_SIZE])
{
    bool keepReading = true;
    int errorNumber;
    size_t bufferPosition = 0;
    memset(buffer, 0, BUFFER_SIZE);

    while (keepReading)
    {
        std::cout << "Before reading " << std::endl;
        ssize_t bytes = read(socketDescriptor, buffer + bufferPosition, BUFFER_SIZE);
        std::cout << "after reading " << std::endl;
        if (bytes == 0)
        {
            keepReading = 0;
        }
        else if (bytes == -1)
        {
            errorNumber = errno;
            if (errorNumber == EAGAIN || errorNumber == EWOULDBLOCK)
            {
                std::cerr << "Reached time out when reading from the end point. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            }
            else
            {
                std::cerr << "Could not read data from the end point. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            }
            keepReading = 0;
            return false;
        }
        else if (bytes < BUFFER_SIZE)
        {
            bufferPosition += bytes;
            keepReading = 1;
            printf("Fragmentation. Bytes leídos = %ld, bufferPosition = %ld \n", bytes, bufferPosition);
        }
        else
        {
            keepReading = 0;
            printf("All read at once\n %s\n", buffer);
        }
    }

    return true;
}

bool Common::writeMessage(int socketDescriptor, char message[BUFFER_SIZE])
{
    char *buffer = message;
    int errorNumber;
    ssize_t remainingBytesToSend = BUFFER_SIZE;

    while (remainingBytesToSend)
    {
        ssize_t bytesSent = write(socketDescriptor, buffer, remainingBytesToSend);
        if (bytesSent <= 0)
        {
            errorNumber = errno;
            std::cerr << "Could not send data to the end point. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            return false;
        }
        remainingBytesToSend -= bytesSent;
        buffer += bytesSent;
    }

    return true;
}

}

#include "common.h"
#include "log.h"

namespace pipetrick
{

bool Common::createSocket(int &socketDescriptor, int flags)
{
    socketDescriptor = socket(AF_INET, SOCK_STREAM | flags, 0);
    if (socketDescriptor == -1)
    {
        int errorNumber = errno;
        Log::logError("Could not create the socket", errorNumber);
        return false;
    }

    return true;
}

Common::SelectResult Common::doSelect(int maxFileDescriptor, fd_set *readFds, fd_set *writeFds, const std::chrono::microseconds *timeOut)
{

    if (!readFds && !writeFds)
    {
        Log::logError("No write or read file descriptors were provided.");
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
        Log::logError("Select failed", errorNumber);
        return SelectResult::ERROR;
    }

    if (retValue == 0)
    {
        Log::logError("Time out expired");
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
        ssize_t bytes = read(socketDescriptor, buffer + bufferPosition, BUFFER_SIZE);
        if (bytes == 0)
        {
            keepReading = 0;
        }
        else if (bytes == -1)
        {
            errorNumber = errno;
            if (errorNumber == EAGAIN || errorNumber == EWOULDBLOCK)
            {
                Log::logError("Reached time out when reading from the end point", errorNumber);
            }
            else
            {
                Log::logError("Could not read data from the end point", errorNumber);
            }
            keepReading = 0;
            return false;
        }
        else if (bytes < BUFFER_SIZE)
        {
            bufferPosition += bytes;
            keepReading = 1;
        }
        else
        {
            keepReading = 0;
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
            Log::logError("Could not send data to the end point", errorNumber);
            return false;
        }
        remainingBytesToSend -= bytesSent;
        buffer += bytesSent;
    }

    return true;
}

void Common::consumePipe(int pipeReadEnd)
{
    bool done = false;
    while (!done)
    {
        char ch;
        if (read(pipeReadEnd, &ch, 1) == -1)
        {
            int errorNumber = errno;
            if (errorNumber != EAGAIN)
            {
                Log::logError("Error reading all bytes from the pipe", errorNumber);
                return;
            }
            done = true;
        }
    }
}

}

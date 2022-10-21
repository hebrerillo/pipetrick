#include "common.h"
#include "log.h"

namespace pipetrick
{

bool Common::createSocket(int &socketDescriptor, int flags, const std::string& prefix)
{
    socketDescriptor = socket(AF_INET, SOCK_STREAM | flags, 0);
    if (socketDescriptor == -1)
    {
        int errorNumber = errno;
        Log::logError(prefix + "Common::createSocket - Could not create the socket", errorNumber);
        return false;
    }

    return true;
}

Common::SelectResult Common::doSelect(int maxFileDescriptor, fd_set* readFds, fd_set* writeFds, const std::chrono::microseconds* timeOut, const std::string& prefix)
{
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
        Log::logError(prefix + "Common::doSelect - Select failed", errorNumber);
        return SelectResult::ERROR;
    }

    if (retValue == 0)
    {
        Log::logError(prefix + "Common::doSelect - Time out expired");
        return SelectResult::TIMEOUT;
    }

    return SelectResult::OK;
}

bool Common::readMessage(int socketDescriptor, char buffer[BUFFER_SIZE], const std::string& prefix)
{
    int errorNumber;
    size_t bufferPosition = 0;
    memset(buffer, 0, BUFFER_SIZE);

    bool keepReading = true;
    while (keepReading)
    {
        ssize_t bytes = read(socketDescriptor, buffer + bufferPosition, BUFFER_SIZE);
        if (bytes == 0)
        {
            Log::logError(prefix + "Common::readMessage - The remote peer closed the connection.");
            return false;
        }
        else if (bytes == -1)
        {
            errorNumber = errno;
            if (errorNumber == EAGAIN || errorNumber == EWOULDBLOCK)
            {
                Log::logError(prefix + "Common::readMessage - Reached time out when reading from the end point");
            }
            else
            {
                Log::logError(prefix + "Common::readMessage - Could not read data from the end point", errorNumber);
            }
            return false;
        }
        else if (bytes < BUFFER_SIZE)
        {
            bufferPosition += bytes;
        }
        else
        {
            return true;
        }
    }

    return true;
}

bool Common::writeMessage(int socketDescriptor, const char message[BUFFER_SIZE], const std::string& prefix)
{
    const char *buffer = message;
    int errorNumber;
    ssize_t remainingBytesToSend = BUFFER_SIZE;

    while (remainingBytesToSend)
    {
        ssize_t bytesSent = write(socketDescriptor, buffer, remainingBytesToSend);
        if (bytesSent == 0)
        {
            Log::logError(prefix + "Common::writeMessage - The remote peer closed the connection.");
            return false;
        }

        if (bytesSent == -1)
        {
            errorNumber = errno;
            Log::logError(prefix + "Common::writeMessage - Could not send data to the end point", errorNumber);
            return false;
        }
        remainingBytesToSend -= bytesSent;
        buffer += bytesSent;
    }

    return true;
}

void Common::consumePipe(int pipeReadEnd, const std::string& prefix)
{
    bool done = false;
    while (!done)
    {
        char ch;
        int readResult = read(pipeReadEnd, &ch, 1);

        if (readResult == 0)
        {
            Log::logError(prefix + "Common::consumePipe - The pipe has been closed.");
            return;
        }

        if (readResult == -1)
        {
            int errorNumber = errno;
            if (errorNumber != EAGAIN)
            {
                Log::logError(prefix + "Common::consumePipe - Error reading all bytes from the pipe", errorNumber);
                return;
            }
            done = true;
        }
    }
}

}

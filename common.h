#ifndef PS_COMMON_H
#define PS_COMMON_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <chrono>

#define BUFFER_SIZE 1024
#define PORT 8080
#define DEFAULT_SERVER_ADDRESS "127.0.0.1"

void readMessage(int socketDescriptor, char buffer[BUFFER_SIZE])
{
    bool keepReading = true;
    int errorNumber;
    size_t bufferPosition = 0;
    memset(buffer, 0, BUFFER_SIZE);

    while(keepReading)
    {
        ssize_t bytes = read(socketDescriptor, buffer + bufferPosition, BUFFER_SIZE);
        if (bytes == 0)
        {
            keepReading = 0;
        }
        else if (bytes == -1)
        {
            errorNumber = errno;
            std::cerr << "Could not data from the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            keepReading = 0;
            return;
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
}

void writeMessage(int socketDescriptor, char message[BUFFER_SIZE])
{
    char* buffer = message;
    int errorNumber;
    ssize_t remainingBytesToSend = BUFFER_SIZE;

    while (remainingBytesToSend)
    {
        ssize_t bytesSent = write(socketDescriptor, buffer, remainingBytesToSend);
        if (bytesSent <= 0)
        {
            errorNumber = errno;
            std::cerr << "Could not send data to the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            return;
        }
        remainingBytesToSend -= bytesSent;
        buffer += bytesSent;
    }
}

#endif
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include "common.h"

static void runAcceptedClient(int* paramSocket)
{
    int socketClient = *paramSocket;
    char clientBuffer[BUFFER_SIZE];
    bool keepReading = true;
    size_t bufferPosition = 0;
    memset(clientBuffer, 0, sizeof (clientBuffer));

    while(keepReading)
    {
        ssize_t bytes = read(socketClient, clientBuffer + bufferPosition, BUFFER_SIZE);
        if (bytes == 0)
        {
            printf("End reading from client\n");
            keepReading = 0;
        }
        else if (bytes == -1)
        {
            printf("read failed. %d,%s\n", errno, strerror(errno));
           keepReading = 0;
        }
        else if (bytes < BUFFER_SIZE)
        {
            bufferPosition += bytes;
            keepReading = 1;
            printf("Fragmentation. Bytes leídos = %ld, bufferPosition = %ld \n", bytes, bufferPosition);
        }
        else
        {
            printf("All read at once\n %s\n", clientBuffer);
        }
    }

    printf("Cadena total = %s\n", clientBuffer);
}

static void socketServer()
{
    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    int errorNumber = 0;
    int socketReuseOption = 1;
    struct sockaddr_in socketAddress;
    const int LISTEN_BACKLOG = 1;

    if (socketDescriptor == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not create the socket server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    if (setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &socketReuseOption, sizeof (int)) == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not reuse the socket descriptor. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(PORT);

    if (bind(socketDescriptor, (struct sockaddr *) &socketAddress, sizeof (socketAddress)) == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not bind to socket address. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    if (listen(socketDescriptor, LISTEN_BACKLOG) == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not listen to socket. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    while(1)
    {
        struct sockaddr_in clientAddress;
        int sizeofSockAddr = sizeof (struct sockaddr_in);

        int socketClientDescriptor = accept(socketDescriptor, (struct sockaddr *) &clientAddress, (socklen_t*) & sizeofSockAddr);
        if (socketClientDescriptor == -1)
        {
            errorNumber = errno;
            std::cerr << "Could not accept on the socket descriptor. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            return;
        }
        
        std::thread threadClient(runAcceptedClient, &socketClientDescriptor);
        threadClient.join();
        close(socketClientDescriptor);
    }
}


int main()
{
    
    socketServer();
    return 0;
}
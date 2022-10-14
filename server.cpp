#include "common.h"

static void runAcceptedClient(int* paramSocket)
{
    int socketClient = *paramSocket;
    char clientBuffer[BUFFER_SIZE];
    memset(clientBuffer, 0, sizeof (clientBuffer));

    readMessage(socketClient, clientBuffer);
    printf("Cadena total recibida = %s\n", clientBuffer);
    writeMessage(socketClient, clientBuffer);
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
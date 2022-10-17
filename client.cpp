#include <arpa/inet.h>
#include <fcntl.h>
#include "common.h"
#define DEFAULT_TIME_OUT_SECONDS 3

static int socketDescriptor;

bool makeSocketNonBlocking(int socketDescriptor)
{
    int errorNumber;
    int flags = fcntl(socketDescriptor, F_GETFL, 0);
    if (flags == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not get the flags of the socket descriptor. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return false;
    }

    flags = (flags | O_NONBLOCK);

    if (fcntl(socketDescriptor, F_SETFL, flags) == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not set socket to non-blocking. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return false;
    }

    return true;
}

void socketClient(const char *serverIP = DEFAULT_SERVER_ADDRESS)
{
    int errorNumber;
    char message[BUFFER_SIZE];
    struct sockaddr_in serverAddress;
    memset(message, 0, sizeof(message));
    strcpy(message, "hola server guapo");

    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not create the socket server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    if (!makeSocketNonBlocking(socketDescriptor))
    {
        return;
    }

    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    if (connect(socketDescriptor, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1)
    {
        errorNumber = errno;
        if (errorNumber != EINPROGRESS)
        {
            std::cerr << "Could not connect to the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            return;
        }
    }

    struct timeval timeOut;
    timeOut.tv_sec = DEFAULT_TIME_OUT_SECONDS;
    timeOut.tv_usec = 0;

    fd_set writeFds;
    FD_ZERO(&writeFds);
    FD_SET(socketDescriptor, &writeFds);

    int retValue = select(socketDescriptor + 1, NULL, &writeFds, NULL, &timeOut);

    if (retValue == 0)
    {
        std::cerr << "Time out expired when waiting to connect to the server." << std::endl;
        close(socketDescriptor);
        return;
    }

    if (retValue == -1)
    {
        errorNumber = errno;
        std::cerr << "select failed when trying to connect to the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    if (FD_ISSET(socketDescriptor, &writeFds))
    {
        if (!writeMessage(socketDescriptor, message))
        {
            return;
        }
    }

    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(socketDescriptor, &readFds);

    retValue = select(socketDescriptor + 1, &readFds, NULL, NULL, &timeOut);
    if (retValue == -1)
    {
        errorNumber = errno;
        std::cerr << "select failed when trying to read from the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    if (retValue == 0)
    {
        std::cerr << "Time out expired when waiting for the server to write." << std::endl;
        close(socketDescriptor);
        return;
    }

    if (FD_ISSET(socketDescriptor, &readFds))
    {
        memset(message, 0, sizeof(message));
        if (!readMessage(socketDescriptor, message))
        {
            return;
        }
    }

    printf("Recibo del servidor = %s\n", message);
    close(socketDescriptor);
}

int main()
{
    std::thread threadClient(socketClient, "127.0.0.1");
    //std::this_thread::sleep_for(std::chrono::milliseconds(400));
    //close(socketDescriptor);
    threadClient.join();
    return 0;
}

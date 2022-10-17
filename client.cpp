#include <arpa/inet.h>
#include <fcntl.h>
#include "common.h"
#define DEFAULT_TIME_OUT_SECONDS 4

static int socketDescriptor;
static int pfd[2];

void socketClient(const char *serverIP = DEFAULT_SERVER_ADDRESS)
{
    int errorNumber;
    char message[BUFFER_SIZE];
    struct sockaddr_in serverAddress;
    memset(message, 0, sizeof(message));
    strcpy(message, "hola server guapo");

    socketDescriptor = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socketDescriptor == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not create the socket server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
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
    fd_set readFds;
    FD_ZERO(&writeFds);
    FD_ZERO(&readFds);
    FD_SET(socketDescriptor, &writeFds);
    FD_SET(socketDescriptor, &readFds);
    FD_SET(pfd[0], &readFds);

    int retValue = select((pfd[0] > socketDescriptor ? pfd[0] : socketDescriptor) + 1, &readFds, &writeFds, NULL, &timeOut);

    if (retValue == -1)
    {
        errorNumber = errno;
        std::cerr << "select failed when trying to connect to the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    if (retValue == 0)
    {
        std::cerr << "Time out expired when waiting to connect to the server." << std::endl;
        close(socketDescriptor);
        return;
    }

    if (FD_ISSET(socketDescriptor, &writeFds))
    {
        if (!writeMessage(socketDescriptor, message))
        {
            return;
        }
    }

    FD_ZERO(&readFds);
    FD_SET(socketDescriptor, &readFds);
    FD_SET(pfd[0], &readFds);

    retValue = select((pfd[0] > socketDescriptor ? pfd[0] : socketDescriptor) + 1, &readFds, NULL, NULL, &timeOut);
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

    if (FD_ISSET(pfd[0], &readFds)) //Another thread wrote to the 'write' end of the pipe.
    {
        for (;;)
        {
            char ch;
            if (read(pfd[0], &ch, 1) == -1)
            {
                errorNumber = errno;
                if (errorNumber == EAGAIN)
                {
                    break; /* No more bytes */
                }
                else
                {
                    std::cerr << "Error reading all bytes from the pipe. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
                    return;
                }
            }
        }
    }
    else if (FD_ISSET(socketDescriptor, &readFds))
    {
        memset(message, 0, sizeof(message));
        if (!readMessage(socketDescriptor, message))
        {
            return;
        }
        printf("Recibo del servidor = %s\n", message);
    }

    close(socketDescriptor);
}

int main()
{

    if (pipe2(pfd, O_NONBLOCK) == -1)
    {
        std::cerr << "select failed when trying to read from the server. Error code " << errno << ": " << strerror(errno) << std::endl;
        return 0;
    }

    std::thread threadClient(socketClient, "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    write(pfd[1], "0", 1);
    threadClient.join();

    close(pfd[0]);
    close(pfd[1]);
    
    return 0;
}

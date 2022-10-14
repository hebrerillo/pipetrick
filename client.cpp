#include <arpa/inet.h>
#include "common.h"

static int socketDescriptor;

void socketClient(const char* serverIP = DEFAULT_SERVER_ADDRESS)
{
    int errorNumber;
    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;

    char message[BUFFER_SIZE];
    memset(message, 0, sizeof (message));
    strcpy(message, "hola server guapo");

    if (socketDescriptor == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not create the socket server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    if (connect(socketDescriptor, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not connect to the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    char* bufferPosition = message;
    ssize_t remainingBytesToSend = strlen(message);
    

    while (remainingBytesToSend)
    {
        ssize_t bytesSent = write(socketDescriptor, bufferPosition, remainingBytesToSend);
        if (bytesSent <= 0)
        {
            errorNumber = errno;
            std::cerr << "Could not send data to the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
            return;
        }
        else
        {
            std::cout << "Bytes sent = " << bytesSent << std::endl;
        }
        remainingBytesToSend -= bytesSent;
        bufferPosition += bytesSent;
    }

    close(socketDescriptor);
}


int main()
{
    //std::thread threadClient(socketClient);
    socketClient();
    return 0;
}
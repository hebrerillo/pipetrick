#include <arpa/inet.h>
#include "common.h"
#define DEFAULT_TIME_OUT_SECONDS 3

static int socketDescriptor;

void socketClient(const char* serverIP = DEFAULT_SERVER_ADDRESS)
{
    int errorNumber;
    char message[BUFFER_SIZE];
    struct sockaddr_in serverAddress;
    memset(message, 0, sizeof (message));
    strcpy(message, "hola server guapo");

    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not create the socket server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    struct timeval timeOut;
    timeOut.tv_sec = DEFAULT_TIME_OUT_SECONDS;
    timeOut.tv_usec = 0;

    setsockopt(socketDescriptor, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&timeOut, sizeof(struct timeval));

    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    if (connect(socketDescriptor, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) == -1)
    {
        errorNumber = errno;
        std::cerr << "Could not connect to the server. Error code " << errorNumber << ": " << strerror(errorNumber) << std::endl;
        return;
    }

    if (!writeMessage(socketDescriptor, message))
    {
        return;
    }
    memset(message, 0, sizeof (message));

    if (!readMessage(socketDescriptor, message))
    {
        return;
    }

    printf("Recibo del servidor = %s\n", message);
    close(socketDescriptor);
}


int main()
{
    std::thread threadClient(socketClient, "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    close(socketDescriptor);
    threadClient.join();
    return 0;
}
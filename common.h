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

namespace pipetrick
{

/**
 * Class with some properties and methods shared between server and client.
 */
class Common
{
public:

    /**
     * Reads a message of size BUFFER_SIZE on the file descriptor 'socketDescriptor'.
     *
     * @param[in] socketDescriptor
     * @param[out] buffer
     * @return true if the read operation was successful, false otherwise
     */
    static bool readMessage(int socketDescriptor, char buffer[BUFFER_SIZE]);

    /**
     * Writes a message of size BUFFER_SIZE on the file descriptor 'socketDescriptor'.
     *
     * @param[in] socketDescriptor
     * @param[out] buffer
     * @return true if the write operation was successful, false otherwise
     */
    static bool writeMessage(int socketDescriptor, char message[BUFFER_SIZE]);
};

}

#endif

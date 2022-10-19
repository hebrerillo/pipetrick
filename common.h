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
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

namespace pipetrick
{

/**
 * Class with helper methods shared between server and client.
 */
class Common
{
public:

    /**
     * Possible values of the select call.
     */
    enum class SelectResult
    {
        OK, //There is a file descriptor ready to be read and/or write
        TIMEOUT, //The time out expired
        ERROR //The select call failed
    };

    /**
     * Creates a socket on 'socketDescriptor' with additional flags
     *
     * @param[out] socketDescriptor The new socket descriptor.
     * @param[in] Additional flags to the 'socket' call.
     * @return true if the socket was created successfully, false if the 'socket' call failed.
     */
    static bool createSocket(int& socketDescriptor, int flags = 0);

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

    /**
     * Performs a select operation on the file descriptors set in 'readFds' and 'writeFds', with a time out.
     *
     * @param[in] maxFileDescriptor The highest-numbered file descriptor in 'readFds' and 'writeFds'.
     * @param[in/out] readFds The file descriptors that will be watched for read operations.
     * @param[in/out] writeFds The file descriptors that will be watched for write operations.
     * @param[in] timeOut The time out for the select operation.
     * @return true if there is a file descriptor that is ready to be read and/or write, false if the time out expired or if the select call failed.
     */
    static SelectResult doSelect(int maxFileDescriptor, fd_set* readFds, fd_set* writeFds, const std::chrono::microseconds* timeOut = nullptr);

    /**
     * Consumes all the pending data in the read end pipe 'pipeReadEnd'.
     */
    static void consumePipe(int pipeReadEnd);
};

}

#endif

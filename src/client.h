#ifndef PT_CLIENT_H
#define PT_CLIENT_H

#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "common.h"

namespace pipetrick
{
class Client
{
public:
    using SelectResult = Common::SelectResult;

    static const char* DEFAULT_IP;
    static const std::chrono::milliseconds MAXIMUM_WAITING_TIME_FOR_FLAG; //The maximum waiting time for the flag 'isRunning_' to be cleared.
    static const std::chrono::microseconds DEFAULT_TIMEOUT;

    /**
     * Constructor.
     * Creates the pipe file descriptors for 'pipeDescriptors_'.
     *
     * @param[in] timeOut The time out to wait for socket operations.
     */
    Client(const std::chrono::microseconds& timeOut = DEFAULT_TIMEOUT);

    ~Client();

    /**
     * Sends a delay 'serverDelay' to the server, so the server will sleep 'serverDelay' milliseconds before answering back.
     * This call blocks until :
     * - The server answers back.
     * - The time out 'timeOut_' expires.
     * - A call to 'stop' is performed.
     *
     * @param[in/out] serverDelay The amount of time that the server will sleep before answering back to this client. If the call is successful, this method
     *                            will modify this parameter by increasing its value by one.
     * @param[in] serverIP The IP address of the remote server.
     * @param[in] serverPort The port where the remote server is listening to connections.
     * @return true if this client had a response from the server, false if the time out expired, a call to 'stop' was performed while waiting or an error occurred.
     */
    bool sendDelayToServer(std::chrono::milliseconds& serverDelay, const std::string& serverIP = DEFAULT_IP, int serverPort = DEFAULT_PORT);

    /**
     * Quits any pending connection by a previous call to 'sendDelayToServer' by using the self pipe trick.
     * This call blocks waiting until a maximum time of MAXIMUM_WAITING_TIME_FOR_FLAG for the flag 'isRunning_' to be cleared.
     */
    void stop();

private:

    /**
     * Performs a connection operation to 'serverIP_' on port 'serverPort_'.
     *
     * @param[in] socketDescriptor The socket descriptor of this client.
     * @param[in] serverIP The IP address of the remote server.
     * @param[in] serverPort The port where the remote server is listening to connections.
     * @return true if the connection operation was succesfull, false otherwise.
     */
    bool connectToServer(int socketDescriptor, const std::string& serverIP, int serverPort);

    /**
     * Closes the socket descriptor 'socketDescriptor' and clears the flag 'isRunning_' to notify all threads.
     *
     * @param socketDescriptor The socket descriptor of this client.
     */
    void closeAndNotify(int socketDescriptor);

    /**
     * Writes to the end 'write' of the pipe and waits until 'isRunning_' is cleared.
     */
    void writeToPipeAndWait();

    /**
     * Check for valid pipe descriptors 'pipeDescriptors_' and, if the descriptors are valid, raises the flag 'isRunning_'.
     *
     * @return true if the pipe descriptors are initialised successfully and the 'isRunning_' flag is raised, false otherwise.
     */
    bool checkPipeDescriptorsAndRun();

    std::chrono::microseconds timeOut_; //The maximum time to wait for socket operations to complete.
    int pipeDescriptors_[2]; //The file descriptors involved in the 'Self pipe trick'
    std::mutex mutex_;
    size_t numConnections_; //The number of current connections of this client.
    std::condition_variable quitCV_; //To notify to the main that there are no pending connections.
};
}

#endif

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
    static const int DEFAULT_PORT;
    static const std::chrono::milliseconds MAXIMUM_WAITING_TIME_FOR_FLAG; //The maximum waiting time for the flag 'isRunning_' to be cleared.
    static const std::chrono::microseconds DEFAULT_TIMEOUT;

    /**
     * Constructor.
     * Creates the pipe file descriptors for 'pipeDescriptors_'.
     *
     * @param[in] serverIP The IP address of the remote server.
     * @param[in] port The port where the remote server is listening to connections.
     * @param[in] timeOut The time out to wait for socket operations.
     */
    Client(const char* serverIP = DEFAULT_IP, int port = DEFAULT_PORT, const std::chrono::microseconds& timeOut = DEFAULT_TIMEOUT);

    /**
     * Sends a delay 'serverDelay' to the server, so the server will sleep 'serverDelay' milliseconds before answering back.
     * This call blocks until :
     * - The server answers back.
     * - The time out 'timeOut_' expires.
     * - A call to 'stop' is performed.
     *
     * @param[in/out] serverDelay The amount of time that the server will sleep before answering back to this client. If the call is successful, this method
     *                            will modify this parameter by increasing its value by one.
     * @return true if this client had a response from the server, false if the time out expired, a call to 'stop' was performed while waiting or an error occurred.
     */
    bool sendDelayToServerAndWait(std::chrono::milliseconds& serverDelay);

    /**
     * Quits any pending connection by a previous call to 'sendDelayToServer' by using the self pipe trick.
     * This call blocks waiting until a maximum time of MAXIMUM_WAITING_TIME_FOR_FLAG for the flag 'isRunning_' to be cleared.
     */
    void stop();

    /**
     * Destructor.
     * Closes the pipe file descriptors in 'pipeDescriptors_'.
     */
    ~Client();

private:

    /**
     * Performs a connection operation to 'serverIP_' on port 'serverPort_'.
     *
     * @return true if the connection operation was succesfull, false otherwise.
     */
    bool connectToServer();
    
    /**
     * Close the socket descriptor 'socketDescriptor_' and clears the flag 'isRunning_' to notify on threads waiting on 'quitCV_'.
     */
    void closeSocketAndNotify();

    std::string serverIP_;
    int serverPort_;
    int socketDescriptor_;
    int pipeDescriptors_[2]; //The file descriptors involved in the 'Self pipe trick'
    std::mutex mutex_;
    std::atomic<bool> isRunning_; //True if there is a pending connection, false otherwise.
    std::condition_variable quitCV_; //To notify to the main that there are no pending connections.
    std::chrono::microseconds timeOut_; //The maximum time to wait for socket operations to complete.
};
}

#endif

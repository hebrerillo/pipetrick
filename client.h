#ifndef PT_CLIENT_H
#define PT_CLIENT_H

#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace pipetrick
{
class Client
{
public:
    static const char* DEFAULT_IP;
    static const int DEFAULT_PORT;
    static const std::chrono::milliseconds DEFAULT_DELAY;
    static const std::chrono::milliseconds MAXIMUM_WAITING_TIME_FOR_FLAG; //The maximum waiting time for the flag 'isRunning_' to be cleared.
    static const std::chrono::microseconds DEFAULT_TIMEOUT;

    /**
     * Constructor.
     * Creates the pipe file descriptors for 'pipeDescriptors_'.
     */
    Client(const char* serverIP = DEFAULT_IP, int port = DEFAULT_PORT, const std::chrono::microseconds& timeOut = DEFAULT_TIMEOUT);

    //TODO dock
    bool sendDelayToServer(const std::chrono::milliseconds& serverDelay = DEFAULT_DELAY);

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
     * Creates a non-blocking socket on the file descriptor 'socketDescriptor_'.
     *
     * @return true if the socket was created successfully, false otherwise.
     */
    bool createSocket();

    /**
     * Performs a connection operation to 'serverIP_' on port 'serverPort_'.
     *
     * @return true if the connection operation was succesfull, false otherwise.
     */
    bool connectToServer();

    /**
     * Consumes all the pending data in the read end pipe 'pipeDescriptors_[0]'.
     */
    void consumePipe() const;
    
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

#ifndef PT_SERVER_H
#define PT_SERVER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#ifdef WITH_PTHREADS //TODO review the rest of the includes when using pthreads
#include <pthread.h>
#endif
#include "common.h"

namespace pipetrick
{

class Server
{
public:

    static const std::chrono::milliseconds MAX_TIME_TO_WAIT_FOR_CLIENTS_TO_FINISH;

    using SelectResult = Common::SelectResult;

    /**
     * Constructor
     *
     * @param[in] maxClients The maximum number of parallel clients that this server can attend at the same time.
     */
    explicit Server(size_t maxClients);

    /**
     * Starts the server to listen to connections on port 'port' in a new thread that will execute the method 'run'.
     *
     * @param[in] port The port where the server will listen to incoming connections.
     * @return true if the server is started successfully, false otherwise.
     */
    bool start(int port = DEFAULT_PORT);

    /**
     * Writes to the 'write' end of the pipe descriptor (pipeDescriptors_[1]) and waits for all clients to finish.
     */
    void stop();

    /**
     * @return the current number of connected clients to this server.
     */
    size_t getNumberOfClients() const;

private:

    /**
     * Performs a bind and listen operations on the socket 'serverSocketDescriptor_' on port 'port'.
     *
     * @param[in] The port to bind.
     * @return true if the bind and listen operations were successful, false otherwise.
     */
    bool bindAndListen(int port);

    /**
     * Method to serve a client with a socket descriptor 'socketDecriptor'.
     * This method blocks for a specific amount of time that is sent by the client.
     * However, if 'stop' is called in the middle of the sleeping time, this call returns immediately.
     *
     * @param[in] socketClientDescriptor The socket descriptor of the client.
     */
    void runClient(int socketClientDescriptor);

    /**
     * Performs an accept call. For each new connection, it creates a new thread to serve it and increases 'currentNumberClients_'.
     *
     * @return true if the accept operation was successful, false otherwise.
     */
    bool doAccept();

    /**
     * Checks whether the maximum number of clients has been reached. In that case, the call blocks until one or several clients finish or
     * until the 'quitSignal_' flag is raised.
     *
     * @return true if the 'quitSignal_' flag is raised, false the current number of clients(currentNumberClients_) is less than the maximum number of clients allowed (maxNumberClients_).
     */
    bool checkForMaximumNumberClients();

    /**
     * Waits for all the current clients to finish and clears the flag 'isRunning_' to notify on 'clientsCV_'.
     */
    void waitForClientsToFinish();

    /**
     * Closes the socket client descriptor and decreases 'currentNumberClients_' to notify on 'clientsCV_'.
     *
     * @param[in] socketClientDescriptor The socket file descriptor to be closed.
     */
    void closeClientAndNotify(int socketClientDescriptor);

    /**
     * Place the current thread to sleep for the number of milliseconds specified in 'buffer'. However, the call will return immediately if 'stop' is called from
     * another thread or if the remote client closes the connection. It also modifies 'buffer' by increasing the initial number of milliseconds by one.
     *
     * @param[in] The socket descriptor of the remote client.
     * @param[in/out] buffer The string containing the number of milliseconds to sleep. If the thread slept the specified time, this string will be modified to contain the initial number increased by one.
     * @return true if 'stop' call was performed while sleeping or the peer closed the connection, false if the current thread was capable of sleeping for the time specified in 'buffer', in which
     *         case 'buffer' will be modified to contain the initial number of milliseconds increased by one.
     */
    bool sleep(int socketClientDescriptor, char buffer[BUFFER_SIZE]);

    /**
     * Raises the flag 'quitSignal_' and writes to the 'write' end of the pipe (pipeDescriptor_[1]).
     */
    void quitRunningThread();

    /**
     * Blocks the current thread until the flag 'isRunning_' is cleared.
     */
    void waitForRunningThread();

    /**
     * The method executed by the server to attend connections. It will be executed until a call to 'stop' is performed.
     */
    void run();
#ifdef WITH_PTHREADS
    /**
     * Helper function to perform a call to 'run' when using Posix threads.
     */
    static void* runHelper(void *context);
#endif

    size_t maxNumberClients_; //The maximum number of parallel clients allowed.
    size_t currentNumberClients_; //The current number of parallel connected clients.
    int serverSocketDescriptor_; //The socket descriptor for this server.
    bool isRunning_; //Whether the server thread is running.
    bool quitSignal_; //Will be raised when 'stop' is called.
#ifdef WITH_PTHREADS
    pthread_t serverThread_;
#else
    std::thread serverThread_; //The running thread
#endif
    mutable std::mutex mutex_; //To notify on 'clientsCV_'
    std::condition_variable clientsCV_; //Will block when 'currentNumberClients_ >= maxNumberClients_'
    int pipeDescriptors_[2]; //The file descriptors involved in the 'Self pipe trick'
};

}

#endif

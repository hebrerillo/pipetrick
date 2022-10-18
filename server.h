#ifndef PT_SERVER_H
#define PT_SERVER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace pipetrick
{

class Server
{
public:

    /**
     * Constructor
     *
     * @param[in] maxClients The maximum number of parallel clients that this server can attend at the same time.
     */
    Server(size_t maxClients);

    /**
     * Starts the server to listen to connections on port 'port' in a new thread that will execute the method 'run'.
     *
     * @param[in] port The port where the server will listen to incoming connections.
     * @return true if the server is started successfully, false otherwise.
     */
    bool start(int port);

    /**
     * TODOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
     */
    void stop();
private:

    /**
     * Performs a bind operation on the socket 'serverSocketDescriptor_' on port 'port'.
     *
     * @param[in] The port to bind.
     * @return true if the bind operation was successful, false otherwise.
     */
    bool bind(int port);

    /**
     * Method to serve a client with a socket descriptor 'socketDecriptor'.
     * This method blocks for a specific amount of time that is sent by the client.
     *
     * @param[in] socketDescriptor The socket descriptor of the client.
     */
    void runClient(int *socketDescriptor);

    void run();
    size_t maxNumberClients_;
    size_t currentNumberClients_;
    int serverSocketDescriptor_;
    std::thread serverThread_;
    std::mutex mutex_;
    std::condition_variable clientsCV_; //Will block when 'currentNumberClients_ >= maxNumberClients_'
};

}

#endif

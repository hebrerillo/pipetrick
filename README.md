# An example of the self pipe trick.

The 'self pipe trick' is a technique used to close a pending connection from a different thread.

This example uses this technique both in server and client sockets.

The server (server.cpp) is a multi-thread server that supports several parallel clients connected at the same time.

The client (client.cpp) implements the method 'sendDelayToServer(std::chrono::milliseconds& serverDelay)', which tells the remote server thread that is created for such a client to sleep for the specified time. Once the sleeping time finishes, the server writes back to the client the sleeping time increased by one.

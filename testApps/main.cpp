#include <iostream>
#include <thread>
#include "server.h"
#include "client.h"

using namespace pipetrick;

int main()
{
    const uint64_t SMALL_DELAY = 50;
    size_t const MAX_NUMBER_CLIENTS = 1;

    Server server(MAX_NUMBER_CLIENTS);
    server.start();

    Client client;

    std::thread threadFirstConnection([&client, SMALL_DELAY]()
    {
        std::chrono::milliseconds serverDelay(SMALL_DELAY);
        client.sendDelayToServer(serverDelay);
//        serverDelay.count() == (SMALL_DELAY + 1));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    client.stop();
    threadFirstConnection.join();
    server.stop();
    return 1;
}

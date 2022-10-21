#include <chrono>
#include <thread>
#include <pthread.h>
#include <valgrind/memcheck.h>
#include "test.h"

void PipeTrickTest::SetUp()
{

}

void PipeTrickTest::TearDown()
{

}

TEST_F(PipeTrickTest, WhenConnectingALotOfClientsWithAHighTimeOutToOneServerAndStoppingAllOfThem_ThenTheQuitProcessIsFast)
{
    const size_t MAX_NUMBER_CLIENTS = 200;
    const uint64_t SERVER_DELAY = 900 * 1000;
    const std::chrono::microseconds TIMEOUT = std::chrono::microseconds(900 * 1000 * 1000);
    const size_t MAX_NUMBER_OF_TRIES_TO_WAIT_FOR_ALL_CLIENTS_TO_CONNECT = 300;

    Server server(MAX_NUMBER_CLIENTS);
    server.start();
    std::vector<ClientInfo* > clients;

    //Create clients
    for(size_t i = 0; i < MAX_NUMBER_CLIENTS + 2; i++)
    {
        Client* client = new Client(TIMEOUT);
        std::thread* clientThread = new std::thread([client, SERVER_DELAY]()
        {
            std::chrono::milliseconds serverDelay(SERVER_DELAY);
            EXPECT_FALSE(client->sendDelayToServerAndWait(serverDelay));
        });

        ClientInfo* clientInfo = new ClientInfo(clientThread, client);
        clients.push_back(clientInfo);
    }

    //Wait for all the clients to connect
    for(size_t i = 0; i < MAX_NUMBER_OF_TRIES_TO_WAIT_FOR_ALL_CLIENTS_TO_CONNECT && server.getNumberOfClients() < MAX_NUMBER_CLIENTS; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    EXPECT_EQ(server.getNumberOfClients(), MAX_NUMBER_CLIENTS);

    uint64_t MAX_ELAPSED_TIME = 60; //The maximum elapsed time before and after stopping all clients and server, in milliseconds.
    if (RUNNING_ON_VALGRIND)
    {
        MAX_ELAPSED_TIME = 4000;
    }

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    //Stop clients and server
    for(size_t i = 0; i < clients.size(); i++)
    {
        clients[i]->client->stop();
        delete clients[i]->client;
    }
    server.stop();

    for(size_t i = 0; i < clients.size(); i++)
    {
        clients[i]->clientThread->join();
        delete clients[i]->clientThread;
        delete clients[i];
    }

    std::chrono::milliseconds elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - begin);
    EXPECT_LT(elapsedTime.count(), MAX_ELAPSED_TIME);
}


TEST_F(PipeTrickTest, WhenConnectingALotOfClientsWithAHighTimeOutToOneServerAndStoppingOnlyTheServer_ThenTheQuitProcessIsFast)
{
    const size_t MAX_NUMBER_CLIENTS = 200;
    const uint64_t SERVER_DELAY = 900 * 1000;
    const std::chrono::microseconds TIMEOUT = std::chrono::microseconds(900 * 1000 * 1000);
    const size_t MAX_NUMBER_OF_TRIES_TO_WAIT_FOR_ALL_CLIENTS_TO_CONNECT = 300;

    Server server(MAX_NUMBER_CLIENTS);
    server.start();
    std::vector<ClientInfo* > clients;

    //Create clients
    for(size_t i = 0; i < MAX_NUMBER_CLIENTS; i++)
    {
        Client* client = new Client(TIMEOUT);
        std::thread* clientThread = new std::thread([client, SERVER_DELAY]()
        {
            std::chrono::milliseconds serverDelay(SERVER_DELAY);
            EXPECT_FALSE(client->sendDelayToServerAndWait(serverDelay));
        });

        ClientInfo* clientInfo = new ClientInfo(clientThread, client);
        clients.push_back(clientInfo);
    }

    //Wait for all the clients to connect
    for(size_t i = 0; i < MAX_NUMBER_OF_TRIES_TO_WAIT_FOR_ALL_CLIENTS_TO_CONNECT && server.getNumberOfClients() < MAX_NUMBER_CLIENTS; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    EXPECT_EQ(server.getNumberOfClients(), MAX_NUMBER_CLIENTS);

    uint64_t MAX_ELAPSED_TIME = 60; //The maximum elapsed time before and after stopping all clients and server, in milliseconds.
    if (RUNNING_ON_VALGRIND)
    {
        MAX_ELAPSED_TIME = 4000;
    }

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    server.stop();

    for(size_t i = 0; i < clients.size(); i++)
    {
        clients[i]->clientThread->join();
        delete clients[i]->clientThread;
        delete clients[i]->client;
        delete clients[i];
    }

    std::chrono::milliseconds elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - begin);
    EXPECT_LT(elapsedTime.count(), MAX_ELAPSED_TIME);
}

TEST_F(PipeTrickTest, WhenAddingSomeClientsWithDifferentSleepingTimes_ThenTheServerReturnsTheCorrectIncreasedSleepingTimeForEachClient)
{
    size_t const MAX_NUMBER_CLIENTS = 30;
    size_t START_DELAY_MS = 200;

    Server server(MAX_NUMBER_CLIENTS);
    server.start();

    std::vector<ClientInfo* > clients;

    //Create clients
    for(size_t i = 0; i < MAX_NUMBER_CLIENTS; i++)
    {
        Client* client = new Client();
        std::thread* clientThread = new std::thread([client, START_DELAY_MS, i]()
        {
            std::chrono::milliseconds serverDelay(START_DELAY_MS + i);
            EXPECT_TRUE(client->sendDelayToServerAndWait(serverDelay));
            EXPECT_EQ(serverDelay.count(), (START_DELAY_MS + i + 1));
        });

        ClientInfo* clientInfo = new ClientInfo(clientThread, client);
        clients.push_back(clientInfo);
    }

    for(size_t i = 0; i < clients.size(); i++)
    {
        clients[i]->clientThread->join();
        delete clients[i]->clientThread;
        delete clients[i]->client;
        delete clients[i];
    }
    server.stop();
}

TEST_F(PipeTrickTest, WhenAClientTellsTheServerToSleepForAVeryLongTimeAndTheClientIsStopped_ThenTheServerStopsTheSleep)
{
    size_t const MAX_NUMBER_CLIENTS = 1;
    Server server(MAX_NUMBER_CLIENTS);
    server.start();

    Client c1;
    std::thread threadFirstClient = std::thread([&c1]()
    {
        std::chrono::milliseconds serverDelay(90 * 1000);
        EXPECT_FALSE(c1.sendDelayToServerAndWait(serverDelay));
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(90));
    EXPECT_EQ(server.getNumberOfClients(), MAX_NUMBER_CLIENTS);
    c1.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(90));
    EXPECT_EQ(server.getNumberOfClients(), 0);
    threadFirstClient.join();
    server.stop();
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

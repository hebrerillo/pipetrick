#include <chrono>
#include <thread>
#include <pthread.h>
#include <valgrind/memcheck.h>
#include "test.h"

void PipeTrickTest::SetUp()
{
    valgrindCheck_.leakCheckInit();
}

void PipeTrickTest::TearDown()
{
    valgrindCheck_.leakCheckEnd();
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
            EXPECT_FALSE(client->sendDelayToServer(serverDelay));
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
        MAX_ELAPSED_TIME = 9000;
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
            EXPECT_FALSE(client->sendDelayToServer(serverDelay));
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
        MAX_ELAPSED_TIME = 10000;
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

TEST_F(PipeTrickTest, WhenAddingAClientWithSmallTimeOutToAFullyServer_ThenTheClientReturnsWithTimeOutError)
{
    const size_t MAX_NUMBER_CLIENTS = 1;
    const uint64_t SERVER_DELAY = 90 * 1000;
    const std::chrono::microseconds SMALL_TIME_OUT(200000); //In microseconds

    Server server(MAX_NUMBER_CLIENTS);
    server.start();

    Client client1;
    std::thread client1Thread = std::thread([&client1, SERVER_DELAY]()
    {
        std::chrono::milliseconds serverDelay(SERVER_DELAY);
        EXPECT_FALSE(client1.sendDelayToServer(serverDelay));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    Client client2(SMALL_TIME_OUT);
    std::chrono::milliseconds serverDelay(SERVER_DELAY);
    EXPECT_FALSE(client2.sendDelayToServer(serverDelay)); //This will fail because the server is full of clients and this client has a very small time out to wait for the server.
    client1.stop();
    client1Thread.join();
    server.stop();
}

TEST_F(PipeTrickTest, WhenAddingSomeClientsWithDifferentSleepingTimes_ThenTheServerReturnsTheCorrectIncreasedSleepingTimeForEachClient)
{
    //This test shows that the server can accept several parallel clients at the same time.
    const size_t MAX_NUMBER_CLIENTS = 30;
    const size_t START_DELAY_MS = 200;

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
            EXPECT_TRUE(client->sendDelayToServer(serverDelay));
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

TEST_F(PipeTrickTest, WhenAClientConnectsToTwoDifferentServers_ThenBothServersReturnTheExpectedValue)
{
    const uint64_t SMALL_DELAY = 50;
    const int SECOND_SERVER_PORT = 8081;
    size_t const MAX_NUMBER_CLIENTS = 1;
    
    Server server(MAX_NUMBER_CLIENTS);
    Server server2(MAX_NUMBER_CLIENTS);
    server.start();
    server2.start(SECOND_SERVER_PORT);

    Client client;

    std::thread threadFirstConnection([&client, SMALL_DELAY](){
        std::chrono::milliseconds serverDelay(SMALL_DELAY);
        EXPECT_TRUE(client.sendDelayToServer(serverDelay));
        EXPECT_TRUE(serverDelay.count() == (SMALL_DELAY + 1));
    });

    std::thread threadSecondConnection([&client, SECOND_SERVER_PORT, SMALL_DELAY](){
        std::chrono::milliseconds serverDelay(SMALL_DELAY + 1);
        EXPECT_TRUE(client.sendDelayToServer(serverDelay, "127.0.0.1", SECOND_SERVER_PORT));
        EXPECT_TRUE(serverDelay.count() == (SMALL_DELAY + 2));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    client.stop();
    threadSecondConnection.join();
    threadFirstConnection.join();
    server.stop();
    server2.stop();
}

TEST_F(PipeTrickTest, WhenAClientConnectsToTwoDifferentServersAndClientIsStopped_ThenTheQuitProcessIsFast)
{
    const uint64_t LONG_DELAY = 90000;
    const int SECOND_SERVER_PORT = 8081;
    size_t const MAX_NUMBER_CLIENTS = 1;
    uint64_t MAX_ELAPSED_TIME = 60; //The maximum elapsed time before and after stopping client and server, in milliseconds.

    if (RUNNING_ON_VALGRIND)
    {
        MAX_ELAPSED_TIME = 9000;
    }
    
    Server server(MAX_NUMBER_CLIENTS);
    Server server2(MAX_NUMBER_CLIENTS);
    server.start();
    server2.start(SECOND_SERVER_PORT);

    Client client;

    std::thread threadFirstConnection([&client, LONG_DELAY](){
        std::chrono::milliseconds serverDelay(LONG_DELAY);
        EXPECT_FALSE(client.sendDelayToServer(serverDelay));
    });

    std::thread threadSecondConnection([&client, SECOND_SERVER_PORT, LONG_DELAY](){
        std::chrono::milliseconds serverDelay(LONG_DELAY + 1);
        EXPECT_FALSE(client.sendDelayToServer(serverDelay, "127.0.0.1", SECOND_SERVER_PORT));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    client.stop();
    threadSecondConnection.join();
    threadFirstConnection.join();
    server.stop();
    server2.stop();
    std::chrono::milliseconds elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - begin);
    EXPECT_LT(elapsedTime.count(), MAX_ELAPSED_TIME);
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
        EXPECT_FALSE(c1.sendDelayToServer(serverDelay));
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

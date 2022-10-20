#include <chrono>
#include <thread>
#include <pthread.h>
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
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

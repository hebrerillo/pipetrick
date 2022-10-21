#ifndef PT_TEST_H
#define PT_TEST_H

#include <gtest/gtest.h>
#include "valgrind_check.h"
#include "server.h"
#include "client.h"

using namespace pipetrick;

class PipeTrickTest: public ::testing::Test {
public: 

    struct ClientInfo
    {
        std::thread* clientThread;
        Client* client;
        ClientInfo(std::thread* _clientThread, Client* _client)
        : clientThread(_clientThread)
        , client(_client)
        {

        }
    };

    void SetUp() override;

    void TearDown() override;

protected:
    ValgrindCheck valgrindCheck_;
};
#endif

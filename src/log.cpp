#include <iostream>
#include <string.h>
#include "log.h"

namespace pipetrick
{

std::mutex Log::mutex_;

void Log::logError(const std::string& errorMsg)
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::cout << errorMsg << std::endl;
}

void Log::logError(const std::string &errorMsg, int errnoNumber)
{
    logError(errorMsg + ". Error code " + std::to_string(errnoNumber) + ": " + strerror(errnoNumber));
}

void Log::logVerbose(const std::string& message)
{
#ifdef VERBOSE_LOGIN
    std::unique_lock<std::mutex> lock(mutex_);
    std::cout << message << std::endl;
#else
    (void)message;
#endif
}

}

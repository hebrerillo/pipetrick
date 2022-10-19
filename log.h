#ifndef PT_LOG_H
#define PT_LOG_H
#include <mutex>
#include <string>

namespace pipetrick
{

class Log
{
public:
    /**
     * Logs a message to the standard error output.
     *
     * @param[in] errorMsg The error message to log.
     */
    static void logError(const std::string &errorMsg);

    /**
     * Logs a message to the standard error output.
     *
     * @param[in] errorMsg The error message to log.
     * @param[in] errnoNumber The code number of the error.
     */
    static void logError(const std::string &errorMsg, int errnoNumber);

    /**
     * Logs a message to the standard output.
     *
     * @param[in] The message to log.
     */
    static void logVerbose(const std::string &message);

private:
    static std::mutex mutex_;
};

}

#endif

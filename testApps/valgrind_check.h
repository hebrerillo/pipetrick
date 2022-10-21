#ifndef PT_VALGRIND_CHECK_H
#define PT_VALGRIND_CHECK_H

/**
 * This class uses the valgrind client requests to check for memory leaks. 
 * It will maily be used for testing, to check the memory leaks before and after each test to make sure the test does not create memory leaks.
 */
class ValgrindCheck
{

public:
    ValgrindCheck();

    /**
     * Performs an initial memory check and fills the values leaked_, dubious_, reachable_ and suppressed_.
     */
    void leakCheckInit();

    /**
     * Performs a memory check and fills the values finalLeaked_, finalDubious_, finalReachable_ and finalSuppressed_.
     * Then, the value 'finalLeaked_' will be compared to 'leaked_' in a test expectation to make sure there were no memory leaks.
     */
    void leakCheckEnd();

private:
    unsigned long leaked_, dubious_, reachable_, suppressed_;
    unsigned long finalLeaked_, finalDubious_, finalReachable_, finalSuppressed_;
};


#endif
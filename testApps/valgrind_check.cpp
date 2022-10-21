#include <gtest/gtest.h>
#include "valgrind_check.h"
#include "valgrind/memcheck.h"


ValgrindCheck::ValgrindCheck()
: leaked_(0)
, dubious_(0)
, reachable_(0)
, suppressed_(0)
, finalLeaked_(0)
, finalDubious_(0)
, finalReachable_(0)
, finalSuppressed_(0)
{}

void ValgrindCheck::leakCheckInit()
{
#ifdef LINUX
    VALGRIND_DO_LEAK_CHECK;
    VALGRIND_COUNT_LEAKS(leaked_, dubious_, reachable_, suppressed_);
#endif
}

void ValgrindCheck::leakCheckEnd()
{
#ifdef LINUX
    VALGRIND_DO_LEAK_CHECK;
    VALGRIND_COUNT_LEAKS(finalLeaked_, finalDubious_, finalReachable_, finalSuppressed_);
    EXPECT_LE(finalLeaked_, leaked_);
#endif
}

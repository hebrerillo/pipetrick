#! /bin/bash

#Execute the tests using the Valgrind tool memcheck with a suppression file to get rid of errors produced by the Gtest suite.
valgrind --leak-check=full --suppressions=leak.supp  ./pttest

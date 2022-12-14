# An example of the self pipe trick in the Linux platform.

## Table of contents

- [Project](#project)
- [Build](#build)
- [Executables](#executables)
  - [Bin folder](#bin-folder)
  - [Executable test target](#executable-test-target)
    - [Valgrind](#valgrind)
    

## Project

The 'self pipe trick' is a technique used to close a pending connection from a different thread.

This example uses such a technique both in server and client sockets.

The server (server.cpp) is a multi-thread server that supports several parallel clients connected at the same time.

The client (client.cpp) implements the method 'sendDelayToServer(std::chrono::milliseconds& serverDelay)', which tells the remote server thread that is created for such a client to sleep for the specified time. Once the sleeping time finishes, the server writes back to the client the sleeping time increased by one.

## Build

To build the project and the executable shells, go to folder 'build' and type:

- cmake ..
- make

## Executables

### Bin folder

If the build is successful, all the executable targets will be generated in the 'build/bin' subdirectory.

### Executable test target

This is an executable that implements some integration tests using the C++ Google Test Framework Gtest.
The code of the main test application is located in 'testApps/test.cpp'.

#### Valgrind

The implemented tests also use Valgrind client requests to check for memory leaks in each test case. 
To do so, the number of memory leaks is checked before and after each test case. If the number of memory leaks has increased after the test case, it means the code of the test case added memory leaks. 

The source files 'testApps/valgrind_check.h' and 'testApps/valgrind_check.cpp' shows how the Valgrind client requests are used.
To execute the test target under Valgrind, go to the bin folder and execute the script 'leakTests.sh'. This script makes use of a Valgrind suppression file to get rid of errors generated by the Gtest suite.

cmake_minimum_required(VERSION 3.13.4)
project(pipetrick)

add_compile_options(-g -std=c++17 -Wall -Wextra -pedantic -Werror -DLINUX -DVERBOSE_LOGIN)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${pipetrick_SOURCE_DIR}/build/bin)

add_subdirectory(src)
add_subdirectory(testApps)


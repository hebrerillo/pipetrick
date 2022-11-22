include_directories(${pipetrick_SOURCE_DIR}/testApps)
include_directories(${pipetrick_SOURCE_DIR}/testApps/valgrind)
include_directories(${pipetrick_SOURCE_DIR}/src)

link_directories(${pipetrick_SOURCE_DIR}/src)

add_executable(ptshell main.cpp)
target_link_libraries(ptshell pipetrick pthread)

add_executable(pttest test.cpp valgrind_check.cpp)
target_link_libraries(pttest pipetrick pthread ${pipetrick_SOURCE_DIR}/lib/libgtest.a)
include_directories(${pipetrick_SOURCE_DIR}/testApps)
include_directories(${pipetrick_SOURCE_DIR}/src)

link_directories(${pipetrick_SOURCE_DIR}/src)

#add_executable(pcshell main.cpp bufferItem.cpp)
#target_link_libraries(pcshell ProducerConsumer pthread)

add_executable(pttest test.cpp)
target_link_libraries(pttest pipetrick pthread ${pipetrick_SOURCE_DIR}/lib/libgtest.a)
include_directories(${pipetrick_SOURCE_DIR}/src)

FILE(GLOB CPPSources *.cpp)
add_library(pipetrick ${CPPSources})
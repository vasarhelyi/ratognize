cmake_minimum_required(VERSION 3.5)

project( ratognize C CXX )

# Set the path where we look for additional CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/etc/cmake)

## Version information
include(GetGitRevisionDescription)
git_describe(RATOGNIZE_DETAILED_VERSION_STRING)
get_git_head_revision(RATOGNIZE_GIT_REFSPEC RATOGNIZE_GIT_REVISION)
string(REGEX MATCH "^[^-]+" RATOGNIZE_VERSION_STRING "${RATOGNIZE_DETAILED_VERSION_STRING}")
string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9+])" "\\1;\\2;\\3"
       RATOGNIZE_VERSION_PARTS ${RATOGNIZE_VERSION_STRING})
list(GET RATOGNIZE_VERSION_PARTS 0 RATOGNIZE_VERSION_MAJOR)
list(GET RATOGNIZE_VERSION_PARTS 1 RATOGNIZE_VERSION_MINOR)
list(GET RATOGNIZE_VERSION_PARTS 2 RATOGNIZE_VERSION_PATCH)
message(STATUS "Found ratognize git version: ${RATOGNIZE_DETAILED_VERSION_STRING}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/src/version.h)

set(CUDA_USE_STATIC_CUDA_RUNTIME OFF)
set(CMAKE_C_FLAGS "-Wall -std=gnu99 -fno-omit-frame-pointer -funwind-tables") 
set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -fno-omit-frame-pointer -funwind-tables") 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DON_LINUX -D__STDC_CONSTANT_MACROS)

# find OpenCV and CUDA and ffmpeg (avcodec) 
find_package( OpenCV REQUIRED )
find_package(CUDA)
find_library(AVCODEC_LIBRARY avcodec)

include_directories(src)

file(GLOB RATOGNIZE_SOURCES "src/*.cpp")

add_executable(ratognize ${RATOGNIZE_SOURCES})
target_link_libraries(ratognize m ${OpenCV_LIBS} ${AVCODEC_LIBRARY})

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
message(STATUS "Found ratognize: version ${RATOGNIZE_DETAILED_VERSION_STRING}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/src/version.h)

set(CMAKE_C_FLAGS "-std=gnu99") 
set(CMAKE_CXX_FLAGS "-std=c++11") 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DON_LINUX -D__STDC_CONSTANT_MACROS)

# find OpenCV  
find_package(OpenCV COMPONENTS core imgproc highgui REQUIRED)
if(CMAKE_VERSION VERSION_LESS "2.8.11")
    # Add OpenCV headers location to your include paths
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# find CUDA
find_package(CUDA)
set(CUDA_USE_STATIC_CUDA_RUNTIME OFF)

# find ffmpeg (avcodec)
find_library(AVCODEC_LIBRARY avcodec)

# add ratognize source
include_directories(src)
file(GLOB RATOGNIZE_SOURCES "src/*.cpp")

# create ratognize executable
add_executable(ratognize ${RATOGNIZE_SOURCES})
target_link_libraries(ratognize m ${OpenCV_LIBS} ${AVCODEC_LIBRARY})

# Set up include directories
target_include_directories(ratognize
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src
)


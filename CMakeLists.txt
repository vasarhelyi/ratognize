cmake_minimum_required(VERSION 2.8) 
project( ratognize C CXX )

set(CUDA_USE_STATIC_CUDA_RUNTIME OFF)
set(CMAKE_C_FLAGS "-Wall -std=gnu99 -fno-omit-frame-pointer -funwind-tables") 
set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -fno-omit-frame-pointer -funwind-tables") 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DON_LINUX -D__STDC_CONSTANT_MACROS)

# find OpenCV and CUDA 
find_package( OpenCV REQUIRED )
find_package(CUDA)
find_library(AVCODEC_LIBRARY avcodec)

include_directories(src)

file(GLOB RATOGNIZE_SOURCES "src/*.cpp")

add_executable(ratognize ${RATOGNIZE_SOURCES})
target_link_libraries(ratognize m ${OpenCV_LIBS} ${AVCODEC_LIBRARY})

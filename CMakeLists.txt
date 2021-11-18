cmake_minimum_required(VERSION 3.1)

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
set(CMAKE_CXX_FLAGS "-std=c++14")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D__STDC_CONSTANT_MACROS)
if (UNIX)
    add_definitions(-DON_LINUX)
endif()

# find OpenCV
if (WIN32)
    if (DEFINED ENV{OPENCV_DIR})
        SET(OpenCV_DIR $ENV{OPENCV_DIR})
    else()
        message(FATAL_ERROR "Please set the OPENCV_DIR system environment variable to point to OpenCV build dir!")
    endif()
endif()
find_package(OpenCV COMPONENTS core imgproc highgui REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# find CUDA
find_package(CUDA)
set(CUDA_USE_STATIC_CUDA_RUNTIME OFF)

if (UNIX)
    # find ffmpeg (avcodec)
    find_library(AVCODEC_LIBRARY avcodec)
endif()

# add ratognize source
include_directories(src)
file(GLOB RATOGNIZE_SOURCES "src/*.cpp")

# create ratognize executable
add_executable(ratognize ${RATOGNIZE_SOURCES})
target_link_libraries(ratognize ${OpenCV_LIBS})
if (UNIX)
    target_link_libraries(ratognize m ${AVCODEC_LIBRARY})
endif()

# Set up include directories
target_include_directories(ratognize
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src
)

# copy OpenCV .dll files next to the executable as a post build event
# TODO: copy only those relevant for the actual $<CONFIG>
if (WIN32)
    file(GLOB dll_file_names "${OpenCV_DIR}/../bin/*.dll")
    set(dest_dir "${CMAKE_BINARY_DIR}/$<CONFIG>")
    foreach(file_name ${dll_file_names})
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${file_name}" "${dest_dir}"
            COMMENT "Copying ${file_name} to build directory"
        )
    endforeach()
endif()

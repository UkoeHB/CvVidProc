######################################
#cmake setup

# obligatory CMake minimum version
cmake_minimum_required(VERSION 3.2)


######################################
#project setup

# Set project name
project(vid_bg
        DESCRIPTION "get background of a video"
        LANGUAGES CXX)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++14")


######################################
#config for project

# config setting: directory containing videos
set(VIDEO_DIR ${CMAKE_SOURCE_DIR}/Videos/)

# config setup: for project directory (auto generated header)
set(GENERATED_FILES ${CMAKE_BINARY_DIR}/generated)
configure_file(Config/project_dir_config.h.in ${GENERATED_FILES}/project_dir_config.h)
include_directories(${GENERATED_FILES})


######################################
#get libraries

# opencv
set(OPENCV OpenCV)
find_package(${OPENCV} REQUIRED)
set(LIB_INCLUDE_DIRS "${${OPENCV}_INCLUDE_DIRS}")
set(MAIN_LIBS "${${OPENCV}_LIBS}")


######################################
#executable target

add_executable(${CMAKE_PROJECT_NAME} Sources/main.cpp Sources/cv_util.cpp Sources/triframe_median_algo.cpp)

# connect headers to executable target
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC AsyncTokens/)

# connect libraries to executable target
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${LIB_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${MAIN_LIBS})


















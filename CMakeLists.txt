######################################
#cmake setup

# obligatory CMake minimum version
cmake_minimum_required(VERSION 3.16...3.19)
message("CMAKE version used: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

# module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/Modules")



######################################
#inputs

# location of OpenCV library
set(CV_DIR "" CACHE STRING "Path to OpenCV library, e.g. '~/MyLibs/' (intended for manual installs of OpenCV)")

# location of OpenCV installation
set(CV_INSTALL_DIR "" CACHE STRING "Path to OpenCV installation, e.g. '~/MyLibs/OpenCV/opencv-1.2.3/release/' (intended for manual installs of OpenCV)")



######################################
#project setup

# Set project name
project(cvvidproc
    DESCRIPTION "get background of a video"
    LANGUAGES CXX)

# C++14 required
set(CMAKE_CXX_STANDARD 14)



######################################
#get libraries

# opencv
set(OPENCV OpenCV)
if(CV_DIR)
    # get location of manually installed opencv
    set(${OPENCV}_DIR ${CV_INSTALL_DIR})
endif()
# note: lower versions of opencv seem prone to segfaulting in image processing (calls to cv::threshold)
#  however, I don't know the version that fixed it, so go with the one that probably works
find_package(${OPENCV} 4.2.0 REQUIRED PATHS ${CV_DIR})
set(LIB_INCLUDE_DIRS "${${OPENCV}_INCLUDE_DIRS}")
set(MAIN_LIBS "${${OPENCV}_LIBS}")


# python and numpy
# for scikit-build must do this
if (${CMAKE_BUILD_TYPE} STREQUAL Release AND SKBUILD)
    # thanks to: https://github.com/pybind/scikit_build_example
    # Currently, Scikit-build does not support FindPython, so we convert the
    # provided hints ourselves.
    set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
    set(Python3_INCLUDE_DIR "${PYTHON_INCLUDE_DIR}")
    set(Python3_LIBRARY "${PYTHON_LIBRARY}")
    set(DUMMY "${PYTHON_VERSION_STRING}")  # Not needed, silences a warning

    set(Python3_FIND_IMPLEMENTATIONS CPython PyPy)  # PyPy requires 3.18
endif ()

# note: NumPy is added for converting NumPy arrays <-> cv::Mat
find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
set(MAIN_LIBS ${MAIN_LIBS} Python3::NumPy)


# pybind11
# Scikit-Build does not add your site-packages to the search path automatically,
# so we need to add it _or_ the pybind11 specific directory here.
execute_process(
  COMMAND
    "${Python3_EXECUTABLE}" -c
    "import pybind11; print(pybind11.get_cmake_dir())"
  OUTPUT_VARIABLE _tmp_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ECHO STDOUT)
list(APPEND CMAKE_PREFIX_PATH "${_tmp_dir}")

# Now we can find pybind11
find_package(pybind11 CONFIG REQUIRED)
set(MAIN_LIBS ${MAIN_LIBS} pybind11::embed)


# threading support
# (thanks to https://stackoverflow.com/a/29871891/14080627)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(MAIN_LIBS ${MAIN_LIBS} Threads::Threads)


# atomic operations
# test if atomic lib must be linked
# (thanks to https://phabricator.kde.org/D16184)
include(CheckAtomic)
if (NEED_ATOMICS OR NEED_ATOMICS_FOR_64 OR NEED_ATOMICS_FOR_128)
    if (NEED_ATOMICS AND NOT HAVE_CXX_ATOMICS_WITH_LIB)
        message(FATAL_ERROR "Need libatomic but support was not found!")
    elseif (NEED_ATOMICS_FOR_64 AND NOT HAVE_CXX_ATOMICS64_WITH_LIB)
        message(FATAL_ERROR "Need libatomic for 64-bit atomics but support was not found!")
    elseif (NEED_ATOMICS_FOR_128 AND NOT HAVE_CXX_ATOMICS128_WITH_LIB)
        message(FATAL_ERROR "Need libatomic for 128-bit atomics but support was not found!")
    endif ()

    set(MAIN_LIBS ${MAIN_LIBS} atomic)
    message("Adding libatomic to MAIN_LIBS")
endif ()


# third party bubble tracking library (python scripts needed at run-time)
# currently only used by the C++ executable
#TODO: this pollutes the build directory a bit, could be simpler
if (NOT SKBUILD)
    include (FetchContent)
    FetchContent_Declare(
        bubbletracking
        GIT_REPOSITORY https://github.com/andylitalo/bubbletracking_koe
        GIT_TAG 5452d5282e342ab512ad275a271073a7d5fb93b1
    )
    FetchContent_Populate(bubbletracking)
endif ()



######################################
#config for project

# config setting: directory containing videos
set(VIDEO_DIR ${CMAKE_SOURCE_DIR}/Videos/)

# config setting: directory containing local python libs
set(PYLIBS_DIR ${CMAKE_SOURCE_DIR}/PySources/PyLibs/)

# config setting: directory containing third party bubble tracking lib
set(BUBBLETRACKING_DIR ${bubbletracking_SOURCE_DIR})

# config setup: for project directory (auto generated header)
set(GENERATED_FILES ${CMAKE_BINARY_DIR}/generated)
configure_file(Config/project_dir_config.h.in ${GENERATED_FILES}/project_dir_config.h)
include_directories(${GENERATED_FILES})



######################################
#set sources/headers

# common sources
set(COMMON_SRC Sources/main.cpp
        Sources/cv_vid_bg_helpers.cpp
        Sources/cv_vid_bubbletrack_helpers.cpp
        Sources/ProcessorAlgos/highlight_bubbles_algo.cpp
        Sources/Utility/cv_util.cpp
        Sources/Utility/ndarray_converter.cpp)

# common headers
set(COMMON_INC Sources/AsyncTokens
        Sources/ProcessorAlgos
        Sources/ProcessorTokenHandlers
        Sources/Utility)



######################################
#python bindings module target

# only create python bindings in Release mode
if (${CMAKE_BUILD_TYPE} STREQUAL Release AND SKBUILD)
    pybind11_add_module(_core MODULE ${COMMON_SRC} Sources/py_bindings.cpp)

    # connect headers to module target
    target_include_directories(_core PUBLIC ${COMMON_INC})

    # connect libraries to module target
    target_include_directories(_core PUBLIC ${LIB_INCLUDE_DIRS})
    target_link_libraries(_core PUBLIC ${MAIN_LIBS})

    # install the module
    install(TARGETS _core DESTINATION .)
elseif (SKBUILD)
    message(WARNING ":: unexpectedly tried to build with scikit-build but not in Release mode! Python module not built.")
endif ()



######################################
#executable target

# do not build main executable when python module is being installed
if (NOT SKBUILD)
    add_executable(${CMAKE_PROJECT_NAME} ${COMMON_SRC} Sources/rand_tests.cpp)

    # connect headers to executable target
    target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${COMMON_INC})

    # connect libraries to executable target
    target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${LIB_INCLUDE_DIRS})
    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${MAIN_LIBS})
endif ()



######################################
#custom commands

# copy python scripts
# currently these are only used by the C++ executable, so don't copy them for python module
#TODO: is it really necessary to copy these to build dir? for now, just use source dir (BUBBLETRACKING_DIR)
#if (NOT SKBUILD)
#    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory
#        ${CMAKE_SOURCE_DIR}/PySources/PyLibs $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/PyLibs
#    )
#endif ()












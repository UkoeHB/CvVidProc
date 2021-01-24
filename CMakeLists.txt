######################################
#cmake setup

# obligatory CMake minimum version
cmake_minimum_required(VERSION 3.15...3.18)


######################################
#project setup

# Set project name
project(vid_bg
        DESCRIPTION "get background of a video"
        LANGUAGES CXX)

# C++14 required
set(CMAKE_CXX_STANDARD 14)


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

# only create python bindings in Release mode
if (${CMAKE_BUILD_TYPE} STREQUAL Release AND SKBUILD)
    # thanks to: https://github.com/pybind/scikit_build_example
    # Currently, Scikit-build does not support FindPython, so we convert the
    # provided hints ourselves.
    set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
    set(Python_INCLUDE_DIR "${PYTHON_INCLUDE_DIR}")
    set(Python_LIBRARY "${PYTHON_LIBRARY}")
    set(DUMMY "${PYTHON_VERSION_STRING}")  # Not needed, silences a warning

    # note: NumPy is added for converting NumPy arrays <-> cv::Mat
    set(Python_FIND_IMPLEMENTATIONS CPython PyPy)  # PyPy requires 3.18
    find_package(Python REQUIRED COMPONENTS Interpreter Development NumPy)

    # Scikit-Build does not add your site-packages to the search path automatically,
    # so we need to add it _or_ the pybind11 specific directory here.
    execute_process(
      COMMAND
        "${Python_EXECUTABLE}" -c
        "import pybind11; print(pybind11.get_cmake_dir())"
      OUTPUT_VARIABLE _tmp_dir
      OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ECHO STDOUT)
    list(APPEND CMAKE_PREFIX_PATH "${_tmp_dir}")

    # Now we can find pybind11
    find_package(pybind11 CONFIG REQUIRED)
elseif (SKBUILD)
    message(WARNING ":: unexpectedly tried to build with scikit-build but not in Release mode! Python module not built.")
endif ()

# threading support (thanks to https://stackoverflow.com/a/29871891/14080627)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(MAIN_LIBS "${MAIN_LIBS}" Threads::Threads)


######################################
#set sources/headers

# common sources
set(COMMON_SRC Sources/main.cpp Sources/cv_vid_bg_helpers.cpp Sources/cv_util.cpp Sources/triframe_median_algo.cpp)

# common headers
set(COMMON_INC Sources/AsyncTokens)


######################################
#python bindings module target

# only create python bindings in Release mode
if (${CMAKE_BUILD_TYPE} STREQUAL Release AND SKBUILD)
    pybind11_add_module(_core MODULE ${COMMON_SRC} Sources/cv_vid_py_bindings.cpp Sources/ndarray_converter.cpp)

    # connect headers to module target
    target_include_directories(_core PUBLIC ${COMMON_INC})

    # connect libraries to module target
    target_include_directories(_core PUBLIC ${LIB_INCLUDE_DIRS})
    target_link_libraries(_core PUBLIC ${MAIN_LIBS} Python::NumPy)

    # install the module
    install(TARGETS _core DESTINATION .)
endif ()


######################################
#executable target

# do not build main executable when python module is being installed
if (NOT SKBUILD)
    add_executable(${CMAKE_PROJECT_NAME} ${COMMON_SRC})

    # connect headers to executable target
    target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${COMMON_INC})

    # connect libraries to executable target
    target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${LIB_INCLUDE_DIRS})
    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${MAIN_LIBS})
endif ()






















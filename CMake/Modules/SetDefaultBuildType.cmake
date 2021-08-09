# set build type to 'Release' by default
set(default_build_type "Release")

# note: don't change the build type for multiconfig generators (XCode and MSVC)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")

    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
        STRING "Choose the build type." FORCE)

    # Set the allowed build type values for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
elseif (CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Configuration types detected: '${CMAKE_CONFIGURATION_TYPES}'")
else ()
    message(STATUS "Build type detected: '${CMAKE_BUILD_TYPE}'")
endif ()

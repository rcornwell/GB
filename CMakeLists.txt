#
# Author:      Richard Cornwell (rich@sky-visions.com)
#
# Copyright 2023, Richard Cornwell
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.19)

## Set our project name
set( PROJECT_NAME "gb" )
project(${PROJECT_NAME} VERSION 0.0.0 DESCRIPTION "Game Boy Simulator")

## Set version number 
set (${PROJECT_NAME}_VERSION_MAJOR 0)
set (${PROJECT_NAME}_VERSION_MINOR 0)

# Generate Doxygen documentation files
option(BUILD_DOC "Build documenation" ON)
option(RUN_TESTS "Run tests" OFF)

# Add a custom CMake Modules directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

## Check if Doxygen is available.
if (BUILD_DOC)
    find_package(Doxygen REQUIRED)
    if (NOT DOXYGEN_FOUND)
	message(WARNING "Doxygen not found, unable to generate documatation")
    else()
        set(DOXY_IN ${CMAKE_HOME_DIRECTORY}/doc/doxyfile.in)                      ## used in the doxyfile.in
        set(DOXY_OUT ${CMAKE_HOME_DIRECTORY}/doc)    ## used in the doxyfile.in
	configure_file(${DOXY_IN} ${DOXY_OUT} @ONLY)
	add_custom_target(doxyDoc 
		COMMAND ${DOXYGEN_EXECUTABLE} ${DOXY_IN}
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
                COMMENT "Building user's documentation into doxyDoc build dir..."
        )
    endif()
endif()

## If running tests need CppUTest installed.
if (RUN_TESTS) 
# Find CppUTest.
if(CPPUTEST_HOME)
    set(CPPUTEST_INCLUDE_DIRS ${CPPUTEST_HOME}/include)
    set(CPPUTEST_LIBRARIES ${CPPUTEST_HOME}/lib)
    set(CPPUTEST_LDFLAGS CppUTest CppUTestExt)
elseif(DEFINED ENV{CPPUTEST_HOME})
    message(STATUS "Using CppUTest home: $ENV{CPPUTEST_HOME}")
    set(CPPUTEST_INCLUDE_DIRS $ENV{CPPUTEST_HOME}/include)
    set(CPPUTEST_LIBRARIES $ENV{CPPUTEST_HOME}/lib)
    set(CPPUTEST_LDFLAGS CppUTest CppUTestExt)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(CPPUTEST REQUIRED cpputest>=3.8)
    message(STATUS "Found CppUTest version ${CPPUTEST_VERSION}")
endif()
ENABLE_TESTING()
endif()


set(CMAKE_CXX_STANDARD 17)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /std:c++17")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT /O2")
else()
#    add_compile_options( -Wall -Wunused-result $<$<CONFIG:DEBUG>:-g> -fsanitize=address -fsanitize-recover=address)
    add_compile_options( -Wall -Wunused-result $<$<CONFIG:DEBUG>:-g>)
    add_compile_definitions($<$<CONFIG:DEBUG>:DEBUG>)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

find_package(CppCheck)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
if (CMAKE_CXX_CPPCHECK)
    list(
         APPEND CMAKE_CXX_CPPCHECK
             "--enable=warning"
             "--enable=style"
             "--enable=performance"
             "--enable=portability"
             "--enable=missingInclude"
             "--suppress=missingIncludeSystem"
             "--enable=information"
             "--inline-suppr"
             "--language=c++")
endif()

find_package (SDL2 REQUIRED)
find_package (SDL2_mixer REQUIRED)

# Get required files and packages
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Add in binary output so we can find configuration file.
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

## Define output executable.
if (WIN32)
    add_executable(${PROJECT_NAME} WIN32)
else()
    set (SDL2_BUILDING_LIBRARY 1)
    add_executable(${PROJECT_NAME})
endif()

# Include SDL libraries.
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_MIXER_LIBRARIES})

# Add in the program source
add_subdirectory(src)

## Clean up project if run on Linux
if(UNIX)
    add_custom_target (distclean @echo cleaning for source distribution)

    add_custom_command(
        COMMENT "distribution clean"
        COMMAND make
        ARGS -C ${CMAKE_CURRENT_BINARY_DIR} clean
        COMMAND find
        ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "CMakeCache.txt" | xargs rm -rf
        COMMAND find
        ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "CMakeFiles" | xargs rm -rf
        COMMAND find
        ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "Makefile" | xargs rm -rf
        COMMAND find
        ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "*.cmake" | xargs rm -rf
        COMMAND find
        ARGS -rf ${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt
        TARGET  distclean
    )
endif(UNIX)


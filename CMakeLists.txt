#  Copyright (c) 2019, Gregory Rogers and Philipp Claßen
#  All rights reserved.
#
#  Distributed under the MIT Software License. (See accompanying
#  file LICENSE or copy at https://opensource.org/licenses/MIT)

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(CppQuickCheck)

# General configure options 
option(CPPQC_USE_BOOST "Use Boost, allowing to build some of the examples." OFF)
option(CPPQC_EXAMPLES "Enable building of example programs." OFF)
option(CPPQC_TESTING "Enable building of test programs." ON)
option(CPPQC_COVERAGE "Enable coverage reporting." OFF)
option(CPPQC_CUSTOSM_TARGETS "Enable building custom target" OFF)
option(CPPQC_INSTALL "Enable install rules." OFF)

if(MSVC)
    set(CMAKE_CXX_FLAGS "/W4 /permissive- /EHsc")
else()
    set(CMAKE_CXX_FLAGS "-O3 -g -Wall -std=c++11")
endif()
if(CPPQC_COVERAGE AND NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif()

add_library(cppqc INTERFACE)
target_include_directories(cppqc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

enable_testing()

if(CPPQC_USE_BOOST)
    find_package(Boost REQUIRED)
endif()

# "catch" based unit tests
if(CPPQC_TESTING)
  add_subdirectory(test)
endif()

if(CPPQC_EXAMPLES)
    add_subdirectory(examples)
endif ()

if(CPPQC_CUSTOSM_TARGETS)
    # additional target to perform clang-format run, requires clang-format
    file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h)
    add_custom_target(
            clangformat
            COMMAND /usr/bin/clang-format
            -style=file
            -i
            ${ALL_SOURCE_FILES}
    )
endif()

if(CPPQC_INSTALL)
    install(DIRECTORY "include/" DESTINATION "include"
        PATTERN ".*" EXCLUDE)
endif()
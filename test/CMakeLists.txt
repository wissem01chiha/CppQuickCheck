#  Copyright (c) 2019, Gregory Rogers and Philipp Claßen
#  All rights reserved.
#
#  Distributed under the MIT Software License. (See accompanying
#  file LICENSE or copy at https://opensource.org/licenses/MIT)

add_executable(
    all-catch-tests
    catch-main.cpp
    arbitrary-tests.cpp
    shrink-explosion-protection.cpp
    compact-check-tests.cpp
    functional-tests.cpp
    )
target_include_directories(all-catch-tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(all-catch-tests all-catch-tests)

# workaround to force CMake to build test executable before running the test
# (source: http://stackoverflow.com/a/736838/783510)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS all-catch-tests)
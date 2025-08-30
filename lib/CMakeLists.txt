cmake_minimum_required(VERSION 3.5)
project(maxlang_lib)

file(GLOB_RECURSE SRCS src/*.cpp)
add_library(maxlang_lib STATIC ${SRCS}
        src/maxlang/array.h
        src/maxlang/array.cpp
)
target_include_directories(maxlang_lib PUBLIC src)
target_link_libraries(maxlang_lib PUBLIC fmt::fmt)

file(GLOB_RECURSE TEST_SRCS test/*.cpp)

add_executable(maxlang_tests ${TEST_SRCS})
target_link_libraries(maxlang_tests gtest gtest_main maxlang_lib)

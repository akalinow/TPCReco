cmake_minimum_required(VERSION 3.14)
set(This DataFormatsTests)
project(${This} VERSION 1.0.0)

set(Sources
    GeometryTPCTest.cpp
    )
add_executable(${This} ${Sources})
target_link_libraries(${This} PUBLIC
    gtest_main
    DataFormats
    )

add_test(
    NAME ${This}
    COMMAND ${This}
    )


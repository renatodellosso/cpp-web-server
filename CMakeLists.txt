cmake_minimum_required(VERSION 4.1)

project(WebServer)

add_executable(WebServer src/main.cpp)

set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost REQUIRED)
target_include_directories(WebServer PRIVATE ${Boost_INCLUDE_DIRS})

file(GLOB SOURCE_FILES src/*.cpp src/*.hpp)
target_sources(WebServer PRIVATE ${SOURCE_FILES})
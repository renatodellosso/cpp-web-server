cmake_minimum_required(VERSION 4.1)

project(Client)

add_executable(Client src/client.cpp)

find_package(Boost REQUIRED)
target_include_directories(Client PRIVATE ${Boost_INCLUDE_DIRS})
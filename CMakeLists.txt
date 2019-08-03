cmake_minimum_required(VERSION 3.15)
project(cell)

set(CMAKE_CXX_STANDARD 11)

find_package(Boost 1.70.0 COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(cell main.cpp)

target_link_libraries(cell ${Boost_LIBRARIES})
target_link_libraries(cell pthread)
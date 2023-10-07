cmake_minimum_required(VERSION 3.26)
project(FlowDB)

set(CMAKE_CXX_STANDARD 23)

find_package(Boost REQUIRED COMPONENTS system)

add_executable(FlowDB src/main.cpp
        src/connection_pool.h
        src/connection_pool.cpp)

target_include_directories(FlowDB PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(FlowDB PRIVATE ${Boost_LIBRARIES})

add_executable(remote_endpoint main.cpp)
target_include_directories(remote_endpoint PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(remote_endpoint PRIVATE ${Boost_LIBRARIES})

add_executable(flow_main src/flow_main.cpp src/actor.h src/runtime.h
        src/future.h
)
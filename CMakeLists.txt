cmake_minimum_required(VERSION 3.26)
project(FlowDB)

set(CMAKE_CXX_STANDARD 17)

include_directories(/opt/homebrew/Cellar/libuv/1.46.0/include/ /opt/homebrew/Cellar/libuv/1.46.0/src/)

# Add the uv library target
add_library(uv STATIC IMPORTED)
set_target_properties(uv PROPERTIES IMPORTED_LOCATION /opt/homebrew/Cellar/libuv/1.46.0/lib/libuv.a)

add_executable(FlowDB main.cpp)

target_link_libraries(FlowDB uv)
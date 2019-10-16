cmake_minimum_required(VERSION 3.11)


add_executable(nimptool main.cpp)
set_target_properties(nimptool PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
)
target_link_libraries(nimptool PUBLIC stdc++fs)
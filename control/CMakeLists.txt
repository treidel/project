cmake_minimum_required(VERSION 2.6)

# include the parent directory as an include directory
include_directories(../)

# define the control library
add_library(control STATIC control.cpp)

# include our dependency libraries
target_link_libraries(control proto)


cmake_minimum_required(VERSION 2.6)

# find the CMake modules we need
find_package(LibEV REQUIRED)

# include the definitions from our CMake modules
include(${LIBEV_INCLUDE_DIRS})

# include the parent directory as an include directory
include_directories(../)

# define the bluetooth library
add_library(app STATIC app_mgr.cpp app_sppconnector.cpp)

# include our dependency libraries
target_link_libraries(app control audio bluetooth ${LIBEV_LIBRARIES})


cmake_minimum_required(VERSION 2.6)

# find the CMake modules we need
find_package(PkgConfig REQUIRED)
find_package(Bluez REQUIRED)
find_package(LibEV REQUIRED)

# include the definitions from our CMake modules
include(${BLUEZ_INCLUDE_DIRS} ${LIBEV_INCLUDE_DIRS})

# include the parent directory as an include directory
include_directories(../)

# define the bluetooth library
add_library(bluetooth STATIC spp_server.cpp spp_connection.cpp)

# include our dependency libraries
target_link_libraries(bluetooth app ${BLUEZ_LIBRARIES} ${LIBEV_LIBRARIES})


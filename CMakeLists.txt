cmake_minimum_required (VERSION 2.6) 
project (leveling-glass)

# set the toolschain info
set(CMAKE_TOOLCHAIN_FILE, "Toolchain-RaspberryPi.cmake")

# add third party directory into the module search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty.cmake)

# find the CMake modules we need
find_package(Log4Cxx REQUIRED)
find_package(LibEV REQUIRED)

# set the global compiler flags
set(CMAKE_CXX_FLAGS "-Werror")

# add our own directory as an include
include_directories(.)

# add the module 
include_directories(${LOG4CXX_INCLUDE_DIRS})
include_directories(${LIBEV_INCLUDE_DIRS})

# add the subdirectories
add_subdirectory (bluetooth)
add_subdirectory (control) 
add_subdirectory (audio)
add_subdirectory (app)

# define the executable
add_executable (leveling-glass main.cpp)

# add the libraries to the executable
target_link_libraries (leveling-glass app bluetooth control audio pthread ${LOG4CXX_LIBRARIES} ${LIBEV_LIBRARIES}) 

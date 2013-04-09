CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 
PROJECT(leveling-glass)

# set the toolschain info
SET(CMAKE_TOOLCHAIN_FILE, "Toolchain-RaspberryPi.cmake")

# add third party directory into the module search path
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty.cmake)

# find the CMake modules we need
FIND_PACKAGE(Log4Cxx REQUIRED)
FIND_PACKAGE(LibEV REQUIRED)

# set the global compiler flags
SET(CMAKE_CXX_FLAGS "-Werror")

IF(DEFINED ENV{VERSION})
	# use the version provided by the external build environment if available
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVERSION=$ENV{VERSION}")
ELSE()
	# hardcode to internal version
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVERSION=INTERNAL")	
ENDIF()

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

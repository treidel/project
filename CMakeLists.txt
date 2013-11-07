CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 

# set the toolschain info
SET(CMAKE_TOOLCHAIN_FILE "Toolchain-BeagleBone-Black.cmake")

#define the project
PROJECT(leveling-glass)

# add third party directory into the module search path
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty.cmake)

# find the CMake modules we need
FIND_PACKAGE(Log4Cxx REQUIRED)
FIND_PACKAGE(LibEV REQUIRED)
FIND_PACKAGE(LibINIConfig REQUIRED)

# set the global compiler flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

IF(DEFINED ENV{VERSION})
       # use the version provided by the external build environment if available
       SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVERSION=\\\"\${VERSION}\\\"")
ENDIF()

# add our own directory as an include
INCLUDE_DIRECTORIES(.)

# add the module 
INCLUDE_DIRECTORIES(${LOG4CXX_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${LIBEV_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${LIBINICONFIG_INCLUDE_DIRS})

# add the subdirectories
ADD_SUBDIRECTORY(bluetooth)
ADD_SUBDIRECTORY(control) 
ADD_SUBDIRECTORY(audio)
ADD_SUBDIRECTORY(app)
ADD_SUBDIRECTORY(proto)

# define the executable
ADD_EXECUTABLE(leveling-glass main.cpp common.cpp config.cpp log.cpp)

# add the libraries to the executable
TARGET_LINK_LIBRARIES(leveling-glass app proto bluetooth control audio pthread ${LOG4CXX_LIBRARIES} ${LIBINICONFIG_LIBRARIES} ${LIBEV_LIBRARIES}) 

# support for make install
INSTALL(TARGETS leveling-glass DESTINATION /usr/bin COMPONENT binaries)
INSTALL(FILES config/log.cfg DESTINATION /etc/leveling-glass COMPONENT config)
INSTALL(FILES config/leveling-glass.conf DESTINATION /etc/init COMPONENT config)
INSTALL(FILES config/leveling-glass.ini DESTINATION /etc/leveling-glass COMPONENT config)

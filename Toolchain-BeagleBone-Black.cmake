# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
# this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER "arm-linux-gnueabi-gcc")
SET(CMAKE_CXX_COMPILER "arm-linux-gnueabi-g++")

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH "${CMAKE_SOURCE_DIR}/staging")

# add an extra directory to the header search path
INCLUDE_DIRECTORIES("${CMAKE_FIND_ROOT_PATH}/usr/include")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# add extra directories to the system library search path
LIST(APPEND CMAKE_SYSTEM_LIBRARY_PATH "${CMAKE_FIND_ROOT_PATH}/usr/lib/arm-linux-gnueabihf")

# add the extra directories to the linker search path
SET(LINKER_FLAGS "-Wl,-dynamic-linker,/lib/ld-linux-armhf.so.3 -Wl,-rpath-link,${CMAKE_FIND_ROOT_PATH}/usr/lib:${CMAKE_FIND_ROOT_PATH}/lib:${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf")
SET(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS}" CACHE STRING "linker flags" FORCE)

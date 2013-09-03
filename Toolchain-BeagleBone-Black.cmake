# this one is important
SET(CMAKE_SYSTEM_NAME Linux)

# specify the cross compiler
SET(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
SET(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")

# ensure we have a target environment
IF(NOT(DEFINED ENV{STAGING}))
    MESSAGE(FATAL_ERROR "STAGING not defined")
ENDIF()

# setup the root path 
SET(CMAKE_FIND_ROOT_PATH "$ENV{STAGING}")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# force the compiler to use the neon FPU instructions
SET(CMAKE_C_FLAGS "-march=armv7-a -mfloat-abi=hard -mfpu=neon")
SET(CMAKE_CXX_FLAGS "-march=armv7-a -mfloat-abi=hard -mfpu=neon")

# add extra directories to the system library search path
LIST(APPEND CMAKE_SYSTEM_LIBRARY_PATH "/usr/lib/arm-linux-gnueabihf")

# add the extra directories to the linker search path
SET(LINKER_FLAGS "-Wl,-dynamic-linker,/lib/ld-linux-armhf.so.3 -Wl,-rpath-link,${CMAKE_FIND_ROOT_PATH}/usr/lib:${CMAKE_FIND_ROOT_PATH}/lib:${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf")
SET(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS}" CACHE STRING "linker flags" FORCE)

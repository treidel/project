cmake_minimum_required(VERSION 2.6)

# find the CMake modules we need
find_package(ALSA REQUIRED)
find_package(LibEV REQUIRED)

# include the definitions from our CMake modules
include(${ALSA_INCLUDE_DIRS} ${LIBEV_INCLUDE_DIRS})

# include the parent directory as an include directory
include_directories(../)

# define the bluetooth library
add_library(audio STATIC audio_capturemgr.cpp audio_captureinstance.cpp audio_channel.cpp audio_processor.cpp)

# include our dependency libraries
target_link_libraries(audio ${ALSA_LIBRARIES} ${LIBEV_LIBRARIES})


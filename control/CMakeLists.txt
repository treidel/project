cmake_minimum_required(VERSION 2.6)

# find the CMake modules we need
find_package(Protobuf REQUIRED)

# include the parent directory as an include directory
include_directories(../)

# include the protobuf include directories
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# generate the C message files
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS control.proto)

# define the control library
add_library(control STATIC control.cpp ${PROTO_SRCS} ${PROTO_HDRS})

# include our dependency libraries
target_link_libraries(control ${PROTOBUF_LITE_LIBRARY})


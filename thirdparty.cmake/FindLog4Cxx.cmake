# - Find log4cxx
# Find the libcxx libraries
#
#  This module defines the following variables:
#     LOG4CXX_FOUND       - True if LOG4CXX_INCLUDE_DIR & LOG4CXX_LIBRARY are found
#     LOG4CXX_LIBRARIES   - Set when LOG4CXX_LIBRARY is found
#     LOG4CXX_INCLUDE_DIRS - Set when LOG4CXX_INCLUDE_DIR is found
#
#     LOG4CXX_INCLUDE_DIR - where to find asoundlib.h, etc.
#     LOG4CXX_LIBRARY     - the asound library
#

find_path(LOG4CXX_INCLUDE_DIR NAMES log4cxx.h
          PATH_SUFFIXES log4cxx
          DOC "The LOG4CXX include directory"
)

find_library(LOG4CXX_LIBRARY NAMES log4cxx
          DOC "The LOG4CXX library"
)

# handle the QUIETLY and REQUIRED arguments and set LOG4CXX_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LOG4CXX DEFAULT_MSG LOG4CXX_LIBRARY LOG4CXX_INCLUDE_DIR)

if(LOG4CXX_FOUND)
  set( LOG4CXX_LIBRARIES ${LOG4CXX_LIBRARY} )
  set( LOG4CXX_INCLUDE_DIRS ${LOG4CXX_INCLUDE_DIR} )
endif()

mark_as_advanced(LOG4CXX_INCLUDE_DIR LOG4CXX_LIBRARY)


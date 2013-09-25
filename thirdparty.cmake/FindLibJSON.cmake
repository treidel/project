# - Find libjson
# Find the libev libraries
#
#  This module defines the following variables:
#     LIBJSON_FOUND       - True if LIBJSON_INCLUDE_DIR & LIBJSON_LIBRARY are found
#     LIBJSON_LIBRARIES   - Set when LIBJSON_LIBRARY is found
#     LIBJSON_INCLUDE_DIRS - Set when LIBJSON_INCLUDE_DIR is found
#
#     LIBJSON_INCLUDE_DIR - where to find json.h, etc.
#     LIBJSON_LIBRARY     - the json library
#

find_path(LIBJSON_INCLUDE_DIR NAMES json.h
          PATH_SUFFIXES json
          DOC "The LIBJSON include directory"
)

find_library(LIBJSON_LIBRARY NAMES json
          DOC "The LIBJSON library"
)

# handle the QUIETLY and REQUIRED arguments and set LIBJSON_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBJSON DEFAULT_MSG LIBJSON_LIBRARY LIBJSON_INCLUDE_DIR)

if(LIBJSON_FOUND)
  set( LIBJSON_LIBRARIES ${LIBJSON_LIBRARY} )
  set( LIBJSON_INCLUDE_DIRS ${LIBJSON_INCLUDE_DIR} )
endif()

mark_as_advanced(LIBJSON_INCLUDE_DIR LIBJSON_LIBRARY)


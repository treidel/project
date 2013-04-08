# - Find libev
# Find the libev libraries
#
#  This module defines the following variables:
#     LIBEV_FOUND       - True if LIBEV_INCLUDE_DIR & LIBEV_LIBRARY are found
#     LIBEV_LIBRARIES   - Set when LIBEV_LIBRARY is found
#     LIBEV_INCLUDE_DIRS - Set when LIBEV_INCLUDE_DIR is found
#
#     LIBEV_INCLUDE_DIR - where to find asoundlib.h, etc.
#     LIBEV_LIBRARY     - the asound library
#

find_path(LIBEV_INCLUDE_DIR NAMES ev.h
          PATH_SUFFIXES libev
          DOC "The LIBEV include directory"
)

find_library(LIBEV_LIBRARY NAMES ev
          DOC "The LIBEV library"
)

# handle the QUIETLY and REQUIRED arguments and set LIBEV_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBEV DEFAULT_MSG LIBEV_LIBRARY LIBEV_INCLUDE_DIR)

if(LIBEV_FOUND)
  set( LIBEV_LIBRARIES ${LIBEV_LIBRARY} )
  set( LIBEV_INCLUDE_DIRS ${LIBEV_INCLUDE_DIR} )
endif()

mark_as_advanced(LIBEV_INCLUDE_DIR LIBEV_LIBRARY)


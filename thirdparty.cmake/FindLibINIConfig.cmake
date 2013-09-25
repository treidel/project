# - Find libini_config
# Find the libini_config libraries
#
#  This module defines the following variables:
#     LIBINICONFIG_FOUND       - True if LIBINICONFIG_INCLUDE_DIR & LIBINICONFIG_LIBRARY are found
#     LIBINICONFIG_LIBRARIES   - Set when LIBINICONFIG_LIBRARY is found
#     LIBINICONFIG_INCLUDE_DIRS - Set when LIBINICONFIG_INCLUDE_DIR is found
#
#     LIBINICONFIG_INCLUDE_DIR - where to find ini_config.h, etc.
#     LIBINICONFIG_LIBRARY     - the ini_config library
#

find_path(LIBINICONFIG_INCLUDE_DIR NAMES ini_config.h
          DOC "The LIBINI include directory"
)

find_library(LIBINICONFIG_LIBRARY NAMES ini_config
          DOC "The LIBINI library"
)

# handle the QUIETLY and REQUIRED arguments and set LIBINICONFIG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBINICONFIG DEFAULT_MSG LIBINICONFIG_LIBRARY LIBINICONFIG_INCLUDE_DIR)

if(LIBINICONFIG_FOUND)
  set( LIBINICONFIG_LIBRARIES ${LIBINICONFIG_LIBRARY} )
  set( LIBINICONFIG_INCLUDE_DIRS ${LIBINICONFIG_INCLUDE_DIR} )
endif()

mark_as_advanced(LIBINICONFIG_INCLUDE_DIR LIBINICONFIG_LIBRARY)


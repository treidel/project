# - Find bluez
# Find the bluez libraries
#
#  This module defines the following variables:
#     BLUEZ_FOUND       - True if BLUEZ_INCLUDE_DIR & BLUEZ_LIBRARY are found
#     BLUEZ_LIBRARIES   - Set when BLUEZ_LIBRARY is found
#     BLUEZ_INCLUDE_DIRS - Set when BLUEZ_INCLUDE_DIR is found
#
#     BLUEZ_INCLUDE_DIR - where to find asoundlib.h, etc.
#     BLUEZ_LIBRARY     - the asound library
#

find_path(BLUEZ_INCLUDE_DIR NAMES bluetooth.h
          PATH_SUFFIXES bluetooth
          DOC "The BLUEZ include directory"
)

find_library(BLUEZ_LIBRARY NAMES bluetooth
          DOC "The BLUEZ library"
)

# handle the QUIETLY and REQUIRED arguments and set BLUEZ_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BLUEZ DEFAULT_MSG BLUEZ_LIBRARY BLUEZ_INCLUDE_DIR)

if(BLUEZ_FOUND)
  set( BLUEZ_LIBRARIES ${BLUEZ_LIBRARY} )
  set( BLUEZ_INCLUDE_DIRS ${BLUEZ_INCLUDE_DIR} )
endif()

mark_as_advanced(BLUEZ_INCLUDE_DIR BLUEZ_LIBRARY)


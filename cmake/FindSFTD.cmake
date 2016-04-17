# - Try to find sftd
# Once done this will define
#  LIBSFTD_FOUND - System has sftd
#  LIBSFTD_INCLUDE_DIRS - The sftd include directories
#  LIBSFTD_LIBRARIES - The libraries needed to use sftd
#
# It also adds an imported target named `3ds::sftd`.
# Linking it is the same as target_link_libraries(target ${LIBSFTD_LIBRARIES}) and target_include_directories(target ${LIBSFTD_INCLUDE_DIRS})


# DevkitPro paths are broken on windows, so we have to fix those
macro(msys_to_cmake_path MsysPath ResultingPath)
    string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
endmacro()

if(NOT DEVKITPRO)
    msys_to_cmake_path("$ENV{DEVKITPRO}" DEVKITPRO)
endif()

find_path(LIBSFTD_INCLUDE_DIR sftd.h
          PATH_SUFFIXES include )

find_library(LIBSFTD_LIBRARY NAMES sftd libsftd.a
          PATH_SUFFIXES lib)

set(LIBSFTD_LIBRARIES ${LIBSFTD_LIBRARY} )
set(LIBSFTD_INCLUDE_DIRS ${LIBSFTD_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBSFTD_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(SFTD  DEFAULT_MSG
                                  LIBSFTD_LIBRARY LIBSFTD_INCLUDE_DIR)

mark_as_advanced(LIBSFTD_INCLUDE_DIR LIBSFTD_LIBRARY )
if(SFTD_FOUND)
    set(SFTD ${LIBSFTD_INCLUDE_DIR}/..)
    message(STATUS "setting SFTD to ${SFTD}")

    add_library(3ds::sftd STATIC IMPORTED GLOBAL)
    set_target_properties(3ds::sftd PROPERTIES
        IMPORTED_LOCATION "${LIBSFTD_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSFTD_INCLUDE_DIR}"
    )
endif()

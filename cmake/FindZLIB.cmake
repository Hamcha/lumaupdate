# - Try to find zlib
# Once done this will define
#  ZLIB_FOUND - System has sftd
#  ZLIB_INCLUDE_DIRS - The sftd include directories
#  ZLIB_LIBRARIES - The libraries needed to use sftd
#
# It also adds an imported target named `3ds::sftd`.
# Linking it is the same as target_link_libraries(target ${ZLIB_LIBRARIES}) and target_include_directories(target ${ZLIB_INCLUDE_DIRS})


# DevkitPro paths are broken on windows, so we have to fix those
macro(msys_to_cmake_path MsysPath ResultingPath)
    string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
endmacro()

if(NOT DEVKITPRO)
    msys_to_cmake_path("$ENV{DEVKITPRO}" DEVKITPRO)
endif()

set(ZLIB_PATHS ${DEVKITPRO}/portlibs/armv6k)

find_path(ZLIB_INCLUDE_DIR zlib.h
          PATHS ${ZLIB_PATHS}
          PATH_SUFFIXES include)

find_library(ZLIB_LIBRARY NAMES ZLIB libz.a
          PATHS ${ZLIB_PATHS}
          PATH_SUFFIXES lib)

set(ZLIB_LIBRARIES ${ZLIB_LIBRARY} )
set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZLIB_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ZLIB DEFAULT_MSG
                                  ZLIB_LIBRARY ZLIB_INCLUDE_DIR)

mark_as_advanced(ZLIB_INCLUDE_DIR ZLIB_LIBRARY )
if(ZLIB_FOUND)
    set(ZLIB ${ZLIB_INCLUDE_DIR}/..)
    message(STATUS "setting ZLIB to ${ZLIB}")

    add_library(3ds::ports::zlib STATIC IMPORTED GLOBAL)
    set_target_properties(3ds::ports::zlib PROPERTIES
        IMPORTED_LOCATION "${ZLIB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIR}"
    )
endif()

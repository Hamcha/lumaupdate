# - Try to find freetype2
# Once done this will define
#  FREETYPE_FOUND - System has sftd
#  FREETYPE_INCLUDE_DIRS - The sftd include directories
#  FREETYPE_LIBRARIES - The libraries needed to use sftd
#
# It also adds an imported target named `3ds::sftd`.
# Linking it is the same as target_link_libraries(target ${FREETYPE_LIBRARIES}) and target_include_directories(target ${FREETYPE_INCLUDE_DIRS})


# DevkitPro paths are broken on windows, so we have to fix those
macro(msys_to_cmake_path MsysPath ResultingPath)
    string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
endmacro()

if(NOT DEVKITPRO)
    msys_to_cmake_path("$ENV{DEVKITPRO}" DEVKITPRO)
endif()

set(FREETYPE_PATHS ${DEVKITPRO}/portlibs/armv6k)

find_path(FREETYPE_INCLUDE_DIR freetype2/ft2build.h
          PATHS ${FREETYPE_PATHS}
          PATH_SUFFIXES include)

find_library(FREETYPE_LIBRARY NAMES freetype libfreetype.a
          PATHS ${FREETYPE_PATHS}
          PATH_SUFFIXES lib)

set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY} )
set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set FREETYPE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(FREETYPE DEFAULT_MSG
                                  FREETYPE_LIBRARY FREETYPE_INCLUDE_DIR)

mark_as_advanced(FREETYPE_INCLUDE_DIR FREETYPE_LIBRARY )
if(FREETYPE_FOUND)
    set(FREETYPE ${FREETYPE_INCLUDE_DIR}/..)
    message(STATUS "setting FREETYPE to ${FREETYPE}")

    add_library(3ds::ports::freetype STATIC IMPORTED GLOBAL)
    set_target_properties(3ds::ports::freetype PROPERTIES
        IMPORTED_LOCATION "${FREETYPE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIR}"
    )
endif()

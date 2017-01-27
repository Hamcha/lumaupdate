# - Try to find libpng
# Once done this will define
#  PNG_FOUND - System has sftd
#  PNG_INCLUDE_DIRS - The sftd include directories
#  PNG_LIBRARIES - The libraries needed to use sftd
#
# It also adds an imported target named `3ds::sftd`.
# Linking it is the same as target_link_libraries(target ${PNG_LIBRARIES}) and target_include_directories(target ${PNG_INCLUDE_DIRS})


# DevkitPro paths are broken on windows, so we have to fix those
macro(msys_to_cmake_path MsysPath ResultingPath)
    string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
endmacro()

if(NOT DEVKITPRO)
    msys_to_cmake_path("$ENV{DEVKITPRO}" DEVKITPRO)
endif()

set(PNG_PATHS ${DEVKITPRO}/portlibs/armv6k)

find_path(PNG_INCLUDE_DIR png.h
          PATHS ${PNG_PATHS}
          PATH_SUFFIXES include)

find_library(PNG_LIBRARY NAMES PNG libpng.a
          PATHS ${PNG_PATHS}
          PATH_SUFFIXES lib)

set(PNG_LIBRARIES ${PNG_LIBRARY} )
set(PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PNG_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PNG DEFAULT_MSG
                                  PNG_LIBRARY PNG_INCLUDE_DIR)

mark_as_advanced(PNG_INCLUDE_DIR PNG_LIBRARY )
if(PNG_FOUND)
    set(PNG ${PNG_INCLUDE_DIR}/..)
    message(STATUS "setting PNG to ${PNG}")

    add_library(3ds::ports::png STATIC IMPORTED GLOBAL)
    set_target_properties(3ds::ports::png PROPERTIES
        IMPORTED_LOCATION "${PNG_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PNG_INCLUDE_DIR}"
    )
endif()

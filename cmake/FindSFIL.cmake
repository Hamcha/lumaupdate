# - Try to find sfil
# Once done this will define
#  LIBSFIL_FOUND - System has sfil
#  LIBSFIL_INCLUDE_DIRS - The sfil include directories
#  LIBSFIL_LIBRARIES - The libraries needed to use sfil
#
# It also adds an imported target named `3ds::sfil`.
# Linking it is the same as target_link_libraries(target ${LIBSFIL_LIBRARIES}) and target_include_directories(target ${LIBSFIL_INCLUDE_DIRS})


# DevkitPro paths are broken on windows, so we have to fix those
macro(msys_to_cmake_path MsysPath ResultingPath)
    string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
endmacro()

if(NOT DEVKITPRO)
    msys_to_cmake_path("$ENV{DEVKITPRO}" DEVKITPRO)
endif()

set(LIBSFIL_PATHS $ENV{CTRULIB} libctru ctrulib ${DEVKITPRO}/libctru ${DEVKITPRO}/ctrulib)

find_path(LIBSFIL_INCLUDE_DIR sfil.h
          PATHS ${LIBSFIL_PATHS}
          PATH_SUFFIXES include )

find_library(LIBSFIL_LIBRARY NAMES sfil libsfil.a
          PATHS ${LIBSFIL_PATHS}
          PATH_SUFFIXES lib)

set(LIBSFIL_LIBRARIES ${LIBSFIL_LIBRARY} )
set(LIBSFIL_INCLUDE_DIRS ${LIBSFIL_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBSFIL_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(SFIL  DEFAULT_MSG
                                  LIBSFIL_LIBRARY LIBSFIL_INCLUDE_DIR)

mark_as_advanced(LIBSFIL_INCLUDE_DIR LIBSFIL_LIBRARY )
if(SFIL_FOUND)
    set(SFIL ${LIBSFIL_INCLUDE_DIR}/..)
    message(STATUS "setting SFIL to ${SFIL}")

    add_library(3ds::sfil STATIC IMPORTED GLOBAL)
    set_target_properties(3ds::sfil PROPERTIES
        IMPORTED_LOCATION "${LIBSFIL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSFIL_INCLUDE_DIR}"
    )
endif()

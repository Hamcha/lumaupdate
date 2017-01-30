cmake_minimum_required(VERSION 3.1)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/DevkitArm3DS.cmake)
set(CMAKE_CXX_STANDARD 11)
project(LumaUpdater C CXX ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(Tools3DS)

# App metadata (for 3dsx and CIA target)

set(APP_TITLE  "Luma Updater")
set(APP_DESC   "Updater for Luma3DS")
set(APP_AUTHOR "Hamcha")

set(APP_ICON  meta/lumaicon.png)
set(APP_RSF   meta/rominfo.rsf)
set(APP_CGFX  meta/luma.cgfx)
set(APP_SOUND meta/jingle.wav)

# Sources
file(GLOB_RECURSE SOURCE_FILES source/*)
add_executable(lumaupdater ${SOURCE_FILES})

# Dependencies
find_package(CTRULIB REQUIRED)
find_package(CITRO3D REQUIRED)
find_package(SF2D REQUIRED)
find_package(SFTD REQUIRED)
find_package(FREETYPE REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
target_link_libraries(lumaupdater
	3ds::sf2d
	3ds::sftd
	3ds::ports::freetype
	3ds::ports::png
	3ds::ports::zlib
	3ds::citro3d
	3ds::ctrulib
)

# Resources
file(GLOB_RECURSE FONT_FILES data/fonts/*.ttf)
file(GLOB_RECURSE CERT_FILES data/certs/*.cer)
add_binary_library(lumaupdater_bin ${FONT_FILES} ${CERT_FILES})
target_link_libraries(lumaupdater lumaupdater_bin)

# Get version if possible
include(FindGit OPTIONAL)
if(GIT_FOUND)
	execute_process(COMMAND
	  "${GIT_EXECUTABLE}" describe --tags --always --dirty
	  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	  OUTPUT_VARIABLE GIT_VERSION
	  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
	set(GIT_VERSION "unknown")
endif()
add_definitions(-DGIT_VERSION="${GIT_VERSION}")

# Final targets
add_3dsx_target(lumaupdater ${APP_TITLE} ${APP_DESC} ${APP_AUTHOR} ${APP_ICON})
add_cia_target(lumaupdater ${APP_RSF} ${APP_CGFX} ${APP_SOUND} ${APP_TITLE} ${APP_DESC} ${APP_AUTHOR} ${APP_ICON})

# Add netload target for testing
add_netload_target(netload lumaupdater)
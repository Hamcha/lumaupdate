cmake_minimum_required(VERSION 3.1)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/DevkitArm3DS.cmake)
set(CMAKE_CXX_STANDARD 11)
project(LumaUpdater C CXX ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(Tools3DS)

# Sources
file(GLOB_RECURSE SOURCE_FILES source/*)
add_executable(lumaupdater ${SOURCE_FILES})

# Dependencies
find_package(CTRULIB REQUIRED)
find_package(CITRO3D REQUIRED)
find_package(SF2D REQUIRED)
find_package(SFTD REQUIRED)
target_link_libraries(lumaupdater 3ds::sf2d 3ds::sftd 3ds::citro3d 3ds::ctrulib)

# Resources
file(GLOB_RECURSE FONT_FILES data/fonts/*.ttf)
file(GLOB_RECURSE CERT_FILES data/certs/*.cer)
target_embed_file(lumaupdater ${DATA_FILES} ${CERT_FILES})

# Luma Updater final targets
add_3dsx_target(lumaupdater)
#TODO CIA

# Add netload target for testing
add_netload_target(netload lumaupdater)
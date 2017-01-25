cmake_minimum_required(VERSION 2.8)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/DevkitArm3DS.cmake)
project(LumaUpdater C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(Tools3DS)

find_package(CTRULIB REQUIRED)
find_package(CITRO3D REQUIRED)
find_package(SF2D REQUIRED)
find_package(SFTD REQUIRED)

file(GLOB_RECURSE SOURCE_FILES
    source/*
)
add_executable(lumaupdater ${SOURCE_FILES})
target_link_libraries(lumaupdater 3ds::sf2d 3ds::sftd 3ds::citro3d 3ds::ctrulib)
add_3dsx_target(lumaupdater)
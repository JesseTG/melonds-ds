cmake_minimum_required(VERSION 3.15)

cmake_policy(VERSION 3.15)

project("melonDS DS"
    VERSION 0.0.0
    DESCRIPTION "An enhanced remake of the melonDS core for libretro that prioritizes upstream compatibility."
    HOMEPAGE_URL "https://melonds.kuribo64.net"
    LANGUAGES C CXX)

include(FetchContent)

find_package(Git)

FetchContent_Declare(
    melonDS
    GIT_REPOSITORY https://github.com/melonDS-emu/melonDS.git
    GIT_TAG 4ba7a2c5
) # TODO: Make GIT_TAG and GIT_REPOSITORY values configurable

FetchContent_GetProperties(melonDS)
set(BUILD_QT_SDL OFF)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${melonDS_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})


FetchContent_Declare(
    libretro-common
    GIT_REPOSITORY https://github.com/libretro/libretro-common.git
    GIT_TAG 20a43ba
) # TODO: Make GIT_TAG and GIT_REPOSITORY values configurable

FetchContent_MakeAvailable(melonDS libretro-common)


add_subdirectory(src)
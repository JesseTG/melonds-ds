cmake_minimum_required(VERSION 3.15)

cmake_policy(VERSION 3.15)

project("melonDS DS"
    VERSION 0.0.0
    DESCRIPTION "An enhanced remake of the melonDS core for libretro that prioritizes upstream compatibility."
    HOMEPAGE_URL "https://melonds.kuribo64.net"
    LANGUAGES C CXX)

include(FetchContent)

find_package(Git)

if (NOT MELONDS_REPOSITORY_URL)
    set(
        MELONDS_REPOSITORY_URL
        "https://github.com/melonDS-emu/melonDS.git"
        CACHE STRING
        "melonDS repository URL. Set this to use a melonDS fork or mirror."
        FORCE
    )
endif ()

if (NOT MELONDS_REPOSITORY_TAG)
    set(
        MELONDS_REPOSITORY_TAG
        "4ba7a2c5"
        CACHE STRING
        "melonDS repository commit hash or tag. Set this when using a new version of melonDS, or when using a custom branch."
        FORCE
    )
endif ()

FetchContent_Declare(
    melonDS
    GIT_REPOSITORY "${MELONDS_REPOSITORY_URL}"
    GIT_TAG "${MELONDS_REPOSITORY_TAG}"
)

FetchContent_GetProperties(melonDS)

set(BUILD_QT_SDL OFF)

if (NOT LIBRETRO_COMMON_REPOSITORY_URL)
    set(
        LIBRETRO_COMMON_REPOSITORY_URL
        "https://github.com/libretro/libretro-common.git"
        CACHE STRING
        "libretro-common repository URL. Set this to use a fork or mirror."
        FORCE
    )
endif ()

if (NOT LIBRETRO_COMMON_REPOSITORY_TAG)
    set(
        LIBRETRO_COMMON_REPOSITORY_TAG
        "20a43ba"
        CACHE STRING
        "libretro-common repository commit hash or tag. Set this when using a new version or a custom branch."
        FORCE
    )
endif ()

FetchContent_Declare(
    libretro-common
    GIT_REPOSITORY "${LIBRETRO_COMMON_REPOSITORY_URL}"
    GIT_TAG "${LIBRETRO_COMMON_REPOSITORY_TAG}"
)
FetchContent_GetProperties(libretro-common)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${FETCHCONTENT_BASE_DIR}/melonds-src/cmake" "${CMAKE_MODULE_PATH}")
FetchContent_MakeAvailable(melonDS libretro-common)
include(cmake/libretro-common.cmake)

option(HAVE_THREADS "Build with thread support" ON)

add_subdirectory(src/libretro)
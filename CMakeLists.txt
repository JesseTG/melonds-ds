cmake_minimum_required(VERSION 3.15)

cmake_policy(VERSION 3.15)
message(STATUS "Using CMake ${CMAKE_VERSION}")
if (CMAKE_MESSAGE_LOG_LEVEL)
    message(STATUS "Message log level is set to ${CMAKE_MESSAGE_LOG_LEVEL}")
endif ()

include(cmake/ParseLibretroInfo.cmake)

project("${MELONDSDS_INFO_corename}"
    VERSION "${MELONDSDS_INFO_display_version}"
    DESCRIPTION "${MELONDSDS_INFO_description}"
    HOMEPAGE_URL "https://melonds.kuribo64.net"
    LANGUAGES C CXX)

include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
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
        "0947e94"
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

set(BUILD_STATIC ON)
set(BUILD_QT_SDL OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
        "10995d5"
        CACHE STRING
        "libretro-common repository commit hash or tag. Set this when using a new version or a custom branch."
        FORCE
    )
endif ()

message(STATUS "Using libretro-common: ${LIBRETRO_COMMON_REPOSITORY_URL} (ref ${LIBRETRO_COMMON_REPOSITORY_TAG})")
message(STATUS "Using melonDS: ${MELONDS_REPOSITORY_URL} (ref ${MELONDS_REPOSITORY_TAG})")

FetchContent_Declare(
    libretro-common
    GIT_REPOSITORY "${LIBRETRO_COMMON_REPOSITORY_URL}"
    GIT_TAG "${LIBRETRO_COMMON_REPOSITORY_TAG}"
)
FetchContent_GetProperties(libretro-common)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${FETCHCONTENT_BASE_DIR}/melonds-src/cmake" "${CMAKE_MODULE_PATH}")
FetchContent_MakeAvailable(melonDS libretro-common)

include(cmake/ConfigureFeatures.cmake)
include(cmake/libretro-common.cmake)
include(cmake/utils.cmake)
include(cmake/ConfigureDependencies.cmake)

add_subdirectory(src/libretro)

dump_cmake_variables()
cmake_minimum_required(VERSION 3.17.5)

cmake_policy(VERSION 3.17.5)
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
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

find_package(Git)

include(cmake/utils.cmake)
include(cmake/FetchDependencies.cmake)
include(cmake/ConfigureFeatures.cmake)
include(cmake/ConfigureDependencies.cmake)

add_subdirectory(src/libretro)

dump_cmake_variables()
cmake_minimum_required(VERSION 3.19)

cmake_policy(VERSION 3.19)
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

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(FetchContent)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

find_package(Git REQUIRED)
include(GitInfo)

message(STATUS "Git revision: ${GIT_REVISION}")
message(STATUS "Git state: ${GIT_STATE}")

option(TRACY_ENABLE "Build with Tracy support." OFF)

include(cmake/utils.cmake)
include(cmake/FetchDependencies.cmake)
include(cmake/ConfigureFeatures.cmake)
include(cmake/ConfigureDependencies.cmake)

# Disabled by default due to https://github.com/JesseTG/melonds-ds/issues/81
# Enable it if you're going to work on it.
option(ENABLE_THREADED_RENDERER "Enable the threaded software renderer." OFF)
option(BUILD_TESTING "Build test suite." OFF)
include(CTest)

add_subdirectory(src/libretro)
include(cmake/GenerateAttributions.cmake)
if (BUILD_TESTING)
    message(STATUS "Enabling test suite.")
    enable_testing()
    add_subdirectory(test)
endif()

dump_cmake_variables()
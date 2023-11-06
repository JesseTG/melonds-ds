cmake_minimum_required(VERSION 3.19)

cmake_policy(VERSION 3.19)
message(STATUS "Using CMake ${CMAKE_VERSION}")
if (CMAKE_MESSAGE_LOG_LEVEL)
    message(STATUS "Message log level is set to ${CMAKE_MESSAGE_LOG_LEVEL}")
endif ()

# Parse the latest version from CHANGELOG.md, so we don't have to specify it in multiple places.
file(STRINGS "CHANGELOG.md" CHANGELOG REGEX "^## \\[([0-9]+\\.[0-9]+\\.[0-9]+)\\].+")
list(TRANSFORM CHANGELOG REPLACE "^## \\[([0-9]+\\.[0-9]+\\.[0-9]+)\\].+" "\\1")
list(SORT CHANGELOG COMPARE NATURAL ORDER DESCENDING)
list(GET CHANGELOG 0 VERSION)

project("melonDS DS"
    VERSION "${VERSION}"
    DESCRIPTION "A remake of the libretro melonDS core that prioritizes standalone parity, reliability, and usability. This core closely follows the feature set of standalone melonDS; it features OpenGL-accelerated upscaling, multiple screen layouts, and Wi-Fi emulation."
    HOMEPAGE_URL "https://melonds.kuribo64.net"
    LANGUAGES C CXX)

configure_file("${CMAKE_SOURCE_DIR}/melondsds_libretro.info.in" "${CMAKE_CURRENT_BINARY_DIR}/melondsds_libretro.info")

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(FetchContent)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

find_package(Git REQUIRED)
include(GitInfo)

option(TRACY_ENABLE "Build with Tracy support." OFF)

if (IOS)
    message(STATUS "JIT is disabled by default in iOS builds.")
    set(DEFAULT_ENABLE_JIT OFF)
else()
    set(DEFAULT_ENABLE_JIT ON)
endif ()
option(ENABLE_JIT "Enable JIT support. Not supported on all platforms." ${DEFAULT_ENABLE_JIT})

if (ANDROID OR IOS OR APPLE)
    message(STATUS "OpenGL is disabled by default on this platform.")
    set(DEFAULT_ENABLE_OPENGL OFF)
else ()
    set(DEFAULT_ENABLE_OPENGL ON)
endif ()

if (ANDROID)
    set(DEFAULT_OPENGL_PROFILE OpenGLES2)
else ()
    set(DEFAULT_OPENGL_PROFILE OpenGL)
endif ()
option(ENABLE_OPENGL "Enable OpenGL support. Not supported on all platforms; defaults to OFF in such case." ${DEFAULT_ENABLE_OPENGL})
set(ENABLE_OGLRENDERER ${ENABLE_OPENGL})
set(OPENGL_PROFILE ${DEFAULT_OPENGL_PROFILE} CACHE STRING "OpenGL profile to use if OpenGL is enabled. Valid values are 'OpenGL', 'OpenGLES2', 'OpenGLES3', 'OpenGLES31', and 'OpenGLES32'.")
set_property(CACHE OPENGL_PROFILE PROPERTY STRINGS OpenGL OpenGLES2 OpenGLES3)

include(cmake/utils.cmake)
include(cmake/FetchDependencies.cmake)
include(cmake/ConfigureFeatures.cmake)
include(cmake/ConfigureDependencies.cmake)

if (NOT GIT_STATE)
    set(GIT_STATE "no-git")
endif ()

set(MELONDSDS_VERSION "${CMAKE_PROJECT_VERSION} (${GIT_STATE}, upstream ${MELONDS_REPOSITORY_TAG}, ${CMAKE_BUILD_TYPE})")
message(STATUS "${CMAKE_PROJECT_NAME} Version: ${MELONDSDS_VERSION}")

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
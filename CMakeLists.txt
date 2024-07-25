cmake_minimum_required(VERSION 3.19)

cmake_policy(VERSION 3.19)
message(STATUS "Using CMake ${CMAKE_VERSION} from ${CMAKE_COMMAND}" )
if (CMAKE_MESSAGE_LOG_LEVEL)
    message(STATUS "Message log level is set to ${CMAKE_MESSAGE_LOG_LEVEL}")
endif ()

# Parse the latest version from CHANGELOG.md, so we don't have to specify it in multiple places.
file(STRINGS "CHANGELOG.md" CHANGELOG REGEX "^## \\[([0-9]+\\.[0-9]+\\.[0-9]+)\\].+")
list(TRANSFORM CHANGELOG REPLACE "^## \\[([0-9]+\\.[0-9]+\\.[0-9]+)\\].+" "\\1")
list(SORT CHANGELOG COMPARE NATURAL ORDER DESCENDING)
list(GET CHANGELOG 0 VERSION)
message(STATUS "Project version is ${VERSION}")

project("melonDS DS"
    VERSION "${VERSION}"
    DESCRIPTION "A remake of the libretro melonDS core that prioritizes standalone parity, reliability, and usability. This core closely follows the feature set of standalone melonDS; it features OpenGL-accelerated upscaling, multiple screen layouts, and Wi-Fi emulation."
    HOMEPAGE_URL "https://melonds.kuribo64.net"
    LANGUAGES C CXX)

message(STATUS "Configuring a ${CMAKE_BUILD_TYPE} build.")
if (APPLE)
    message(STATUS "CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")
endif ()
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(FetchContent)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

find_package(Git REQUIRED)
include(GitInfo)

message(STATUS "GIT_REVISION: ${GIT_REVISION}")
message(STATUS "GIT_STATE: ${GIT_STATE}")
message(STATUS "GIT_BRANCH: ${GIT_BRANCH}")

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

if (ENABLE_OPENGL AND (NOT OPENGL_PROFILE STREQUAL "OpenGL"))
    message(FATAL_ERROR "melonDS does not support OpenGL ES yet")
endif()

if (ANDROID)
    if (NOT ANDROID_PLATFORM)
        set(ANDROID_PLATFORM "android-24")
    endif()

    message(STATUS "Targeting Android platform ${ANDROID_PLATFORM}")
endif ()

include(cmake/utils.cmake)
include(cmake/FetchDependencies.cmake)
include(cmake/ConfigureFeatures.cmake)
include(cmake/ConfigureDependencies.cmake)

set(MELONDSDS_NAME "${CMAKE_PROJECT_NAME}")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(MELONDSDS_VERSION "${CMAKE_PROJECT_VERSION}")
else()
    if (GIT_STATE)
        set(MELONDSDS_VERSION "${GIT_STATE}-${CMAKE_BUILD_TYPE}-upstream-${MELONDS_REPOSITORY_TAG}")
    else ()
        set(MELONDSDS_VERSION "v${CMAKE_PROJECT_VERSION}-nogit-${CMAKE_BUILD_TYPE}-upstream-${MELONDS_REPOSITORY_TAG}")
    endif()
endif()

message(STATUS "${MELONDSDS_NAME} version string: '${MELONDSDS_VERSION}'")
configure_file("${CMAKE_SOURCE_DIR}/melondsds_libretro.info.in" "${CMAKE_CURRENT_BINARY_DIR}/melondsds_libretro.info")

# Formerly off by default due to a bug that has since been fixed.
# This option may be removed in the future.
option(ENABLE_THREADED_RENDERER "Enable the threaded software renderer." ON)
option(BUILD_TESTING "Build test suite." OFF)
include(CTest)

# iOS/tvOS want the library built SHARED, other platforms have been happy with MODULE
option(BUILD_AS_SHARED_LIBRARY "Allow for both linking and loading" OFF)

add_subdirectory(src/libretro)
include(cmake/GenerateAttributions.cmake)

if (BUILD_TESTING)
    message(STATUS "Enabling test suite.")
    enable_testing()
    add_subdirectory(test)
endif()

dump_cmake_variables()

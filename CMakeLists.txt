cmake_minimum_required(VERSION 3.15)

cmake_policy(VERSION 3.15)
message(STATUS "Using CMake ${CMAKE_VERSION}")
if (CMAKE_MESSAGE_LOG_LEVEL)
    message(STATUS "Message log level is set to ${CMAKE_MESSAGE_LOG_LEVEL}")
endif ()

project("melonDS DS"
    VERSION 0.0.0
    DESCRIPTION "An enhanced remake of the melonDS core for libretro that prioritizes upstream compatibility."
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
        "391ad8c"
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

message(STATUS "Using libretro-common repository ${LIBRETRO_COMMON_REPOSITORY_URL}, commit ${LIBRETRO_COMMON_REPOSITORY_TAG}")
message(STATUS "Using melonDS repository ${MELONDS_REPOSITORY_URL}, commit ${MELONDS_REPOSITORY_TAG}")

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

if (OpenGL_OpenGL_FOUND)
    # Upstream melonDS uses GLAD to load OpenGL, but we want to use libretro's loader.
    # So instead of patching melonDS, let's change how it's compiled.

    file(GLOB melonDS_cpp_sources "${melonDS_SOURCE_DIR}/src/*.cpp")
    # Get all C++ source files within melonDS' main directory.
    # Need to specify C++ files because melonDS uses assembly,
    # which chokes if the glsym headers are included.

    # Now ensure that melonDS can find libretro's headers...
    target_include_directories(core SYSTEM PRIVATE "${libretro-common_SOURCE_DIR}/include")

    # ...and that the #defines that libretro needs here are provided.
    # Also, that the header that includes GLAD is *excluded* (PLATFORMOGL_H).
    target_compile_definitions(core PUBLIC PLATFORMOGL_H HAVE_OPENGL OGLRENDERER_ENABLED ENABLE_OGLRENDERER)

    if (APPLE)
        # macOS uses a different OpenGL header path.
        set(MELONDSDS_OPENGL_INCLUDES "-include;OpenGL/gl.h;-include;OpenGL/glext.h")
    else()
        set(MELONDSDS_OPENGL_INCLUDES "-include;GL/gl.h;-include;GL/glext.h")
    endif()

    # Now let's include the relevant headers before each melonDS C++ source file.
    set_source_files_properties(
        ${melonDS_cpp_sources}
        TARGET_DIRECTORY
        core
        PROPERTIES
        COMPILE_OPTIONS "${MELONDSDS_OPENGL_INCLUDES};-include;glsm/glsm.h;-include;glsm/glsmsym.h;-include;glsym/glsym_gl.h")
    # TODO: Adapt for GLES2 and GLES3
endif ()

add_subdirectory(src/libretro)

dump_cmake_variables()
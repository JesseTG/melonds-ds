# Detects the presence of various libraries or functions and sets the appropriate HAVE_* variables.
# Other files are used to actually configure the build.

if (ANDROID)
    set(DEFAULT_OPENGL_PROFILE OpenGLES2)
else ()
    set(DEFAULT_OPENGL_PROFILE OpenGL)
endif ()

option(ENABLE_DYNAMIC "Build with dynamic library support, if supported by the target." ON)
option(ENABLE_EGL "Build with EGL support, if supported by the target." OFF)
option(ENABLE_NETWORKING "Build with networking support, if supported by the target." ON)
option(ENABLE_THREADS "Build with thread support, if supported by the target." ON)
option(ENABLE_ZLIB "Build with zlib support, if supported by the target." ON)
option(ENABLE_GLSM_DEBUG "Enable debug output for GLSM." OFF)
set(OPENGL_PROFILE ${DEFAULT_OPENGL_PROFILE} CACHE STRING "OpenGL profile to use if OpenGL is enabled. Valid values are 'OpenGL', 'OpenGLES2', 'OpenGLES3', 'OpenGLES31', and 'OpenGLES32'.")
set_property(CACHE OPENGL_PROFILE PROPERTY STRINGS OpenGL OpenGLES2 OpenGLES3)

if (ENABLE_THREADS)
    find_package(Threads)

    if (Threads_FOUND)
        set(HAVE_THREADS ON)
    endif ()
endif ()

if (ENABLE_ZLIB)
    find_package(ZLIB)

    if (ZLIB_FOUND)
        set(HAVE_ZLIB ON)
    endif ()
endif ()

if (ENABLE_GLSM_DEBUG)
    set(HAVE_GLSM_DEBUG ON)
endif ()

if (ENABLE_OGLRENDERER)
    # ENABLE_OGLRENDERER is defined by melonDS's CMakeLists.txt
    if (OPENGL_PROFILE STREQUAL "OpenGL")
        if (ENABLE_EGL)
            find_package(OpenGL OPTIONAL_COMPONENTS EGL)
        else ()
            find_package(OpenGL)
        endif ()
    elseif (OPENGL_PROFILE STREQUAL "OpenGLES2")
        # Built-in support for finding OpenGL ES isn't available until CMake 3.27,
        # so we use an external module.
        find_package(OpenGLES OPTIONAL_COMPONENTS V2)
    elseif (OPENGL_PROFILE STREQUAL "OpenGLES3")
        find_package(OpenGLES OPTIONAL_COMPONENTS V3)
    elseif (OPENGL_PROFILE STREQUAL "OpenGLES31")
        find_package(OpenGLES OPTIONAL_COMPONENTS V31)
    elseif (OPENGL_PROFILE STREQUAL "OpenGLES32")
        find_package(OpenGLES OPTIONAL_COMPONENTS V32)
    else()
        get_property(OpenGLProfiles CACHE OPENGL_PROFILE PROPERTY STRINGS)
        message(FATAL_ERROR "Expected an OpenGL profile in '${OpenGLProfiles}', got '${OPENGL_PROFILE}'")
    endif()

    if (ENABLE_EGL AND OpenGL_EGL_FOUND)
        set(HAVE_EGL ON)
    endif()

    if (OpenGL_OpenGL_FOUND)
        set(HAVE_OPENGL ON)
    endif()

    if (OpenGLES_V1_FOUND)
        set(HAVE_OPENGLES ON)
        set(HAVE_OPENGLES1 ON)
    endif ()

    if (OpenGL::GLES2 OR OpenGLES_V2_FOUND)
        set(HAVE_OPENGLES ON)
        set(HAVE_OPENGLES2 ON)
    endif ()

    if (OpenGL::GLES3 OR OpenGLES_V3_FOUND)
        set(HAVE_OPENGLES ON)
        set(HAVE_OPENGLES3 ON)
    endif ()

    if (OpenGLES_V31_FOUND)
        set(HAVE_OPENGLES ON)
        set(HAVE_OPENGLES31 ON)
    endif ()

    if (OpenGLES_V32_FOUND)
        set(HAVE_OPENGLES ON)
        set(HAVE_OPENGLES32 ON)
    endif ()

    check_include_files("GL3/gl3.h;GL3/gl3ext.h" HAVE_OPENGL_MODERN)
endif()

if (ENABLE_NETWORKING)
    set(HAVE_NETWORKING ON)

    if (WIN32)
        list(APPEND CMAKE_REQUIRED_LIBRARIES ws2_32)
        check_symbol_exists(getaddrinfo "winsock2.h;ws2tcpip.h" HAVE_GETADDRINFO)
    else()
        check_symbol_exists(getaddrinfo "sys/types.h;sys/socket.h;netdb.h" HAVE_GETADDRINFO)
    endif ()

    if (NOT HAVE_GETADDRINFO)
        set(HAVE_SOCKET_LEGACY ON)
    endif()
endif ()

check_symbol_exists(strlcpy "bsd/string.h;string.h" HAVE_STRL)
check_symbol_exists(mmap "sys/mman.h" HAVE_MMAP)
check_include_file("sys/mman.h" HAVE_MMAN)

if (ENABLE_DYNAMIC)
    set(HAVE_DYNAMIC ON)
endif ()

if (IOS)
    set(HAVE_COCOATOUCH ON)
endif ()

function(add_common_definitions TARGET)
    if (APPLE)
        target_compile_definitions(${TARGET} PUBLIC GL_SILENCE_DEPRECATION)
        # macOS has deprecated OpenGL, and its headers spit out a lot of warnings
    endif()

    if (HAVE_COCOATOUCH)
        target_compile_definitions(${TARGET} PUBLIC HAVE_COCOATOUCH)
    endif ()

    if (HAVE_DYNAMIC)
        target_compile_definitions(${TARGET} PUBLIC HAVE_DYNAMIC HAVE_DYLIB)
    endif ()

    if (HAVE_EGL)
        target_compile_definitions(${TARGET} PUBLIC HAVE_EGL)
    endif ()

    if (HAVE_GLSM_DEBUG)
        target_compile_definitions(${TARGET} PUBLIC GLSM_DEBUG)
    endif ()

    if (HAVE_MMAP)
        target_compile_definitions(${TARGET} PUBLIC HAVE_MMAP)
    endif ()

    if (HAVE_NETWORKING)
        target_compile_definitions(${TARGET} PUBLIC HAVE_NETWORKING)

        if (HAVE_GETADDRINFO)
            target_compile_definitions(${TARGET} PUBLIC HAVE_GETADDRINFO)
        endif ()

        if (HAVE_SOCKET_LEGACY)
            target_compile_definitions(${TARGET} PUBLIC HAVE_SOCKET_LEGACY)
        endif ()
    endif ()

    if (HAVE_OPENGL)
        target_compile_definitions(${TARGET} PUBLIC HAVE_OPENGL OGLRENDERER_ENABLED CORE ENABLE_OGLRENDERER PLATFORMOGL_H)
    endif ()

    if (HAVE_OPENGL_MODERN)
        target_compile_definitions(${TARGET} PUBLIC HAVE_OPENGL_MODERN)
    endif ()

    if (HAVE_OPENGLES)
        target_compile_definitions(${TARGET} PUBLIC HAVE_OPENGLES)
    endif ()

    if (HAVE_OPENGLES1)
        target_compile_definitions(${TARGET} PUBLIC HAVE_OPENGLES1 HAVE_OPENGLES_1)
    endif ()

    if (HAVE_OPENGLES2)
        target_compile_definitions(${TARGET} PUBLIC HAVE_OPENGLES2 HAVE_OPENGLES_2)
    endif ()

    if (HAVE_OPENGLES3)
        target_compile_definitions(${TARGET} PUBLIC HAVE_OPENGLES3 HAVE_OPENGLES_3)
    endif ()

    if (HAVE_OPENGLES31)
        target_compile_definitions(${TARGET} PUBLIC HAVE_OPENGLES31 HAVE_OPENGLES_31 HAVE_OPENGLES_3_1)
    endif ()

    if (HAVE_OPENGLES32)
        target_compile_definitions(${TARGET} PUBLIC HAVE_OPENGLES32 HAVE_OPENGLES_32 HAVE_OPENGLES_3_2)
    endif ()

    if (HAVE_STRL)
        target_compile_definitions(${TARGET} PUBLIC HAVE_STRL)
    endif ()

    if (HAVE_THREADS)
        target_compile_definitions(${TARGET} PUBLIC HAVE_THREADS)
    endif ()

    if (HAVE_ZLIB)
        target_compile_definitions(${TARGET} PUBLIC HAVE_ZLIB)
    endif ()
endfunction()

# TODO: Detect if ARM NEON is available; if so, define HAVE_NEON and HAVE_ARM_NEON_ASM_OPTIMIZATIONS
# TODO: Detect if libnx is available and we're building for Switch; if so, define HAVE_LIBNX
# TODO: Detect if SSL is available; if so, define HAVE_SSL
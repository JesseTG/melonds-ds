# Detects the presence of various libraries or functions and sets the appropriate HAVE_* variables.
# Other files are used to actually configure the build.

option(ENABLE_EGL "Build with EGL support, if supported by the target." OFF)
option(ENABLE_THREADS "Build with thread support, if supported by the target." ON)
option(ENABLE_ZLIB "Build with zlib support, if supported by the target." ON)

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

if (ENABLE_OGLRENDERER)
    # ENABLE_OGLRENDERER is defined by melonDS's CMakeLists.txt
    find_package(OpenGL)

    if (ENABLE_EGL AND OpenGL_EGL_FOUND)
        set(HAVE_EGL ON)
    endif()

    if (OpenGL_OpenGL_FOUND)
        set(HAVE_OPENGL ON)
    endif()

    if (OpenGL::GLES2)
        set(HAVE_OPENGLES ON)
        set(HAVE_OPENGLES2 ON)
    endif ()

    if (OpenGL::GLES3)
        set(HAVE_OPENGLES ON)
        set(HAVE_OPENGLES3 ON)
    endif ()

    check_include_files("GL3/gl3.h;GL3/gl3ext.h" HAVE_OPENGL_MODERN)
endif()

check_symbol_exists(strlcpy "bsd/string.h;string.h" HAVE_STRL)
check_symbol_exists(mmap "sys/mman.h" HAVE_MMAP)
check_include_file("sys/mman.h" HAVE_MMAN)

# TODO: Detect if ARM NEON is available; if so, define HAVE_NEON and HAVE_ARM_NEON_ASM_OPTIMIZATIONS
# TODO: Detect if libnx is available and we're building for Switch; if so, define HAVE_LIBNX
# TODO: Detect if cocoatouch is available; if so, define HAVE_COCOATOUCH
# TODO: Detect if OpenGL ES is available; if so, define HAVE_OPENGLES(_?[123](_[12])?)?
# TODO: Detect if SSL is available; if so, define HAVE_SSL
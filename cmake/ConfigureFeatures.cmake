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

if (ENABLE_OGLRENDERER)
    # ENABLE_OGLRENDERER is defined by melonDS's CMakeLists.txt
    find_package(OpenGL)

    if (OpenGL_OpenGL_FOUND)
        set(HAVE_OPENGL ON)
    endif()
endif()

check_symbol_exists(strlcpy "bsd/string.h;string.h" HAVE_STRL)
check_symbol_exists(mmap "sys/mman.h" HAVE_MMAP)
check_include_file("sys/mman.h" HAVE_MMAN)

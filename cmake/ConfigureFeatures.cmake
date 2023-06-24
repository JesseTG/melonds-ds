# Detects the presence of various libraries or functions and sets the appropriate HAVE_* variables.
# Other files are used to actually configure the build.

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

check_symbol_exists(strlcpy "string.h;bsd/string.h" HAVE_STRL)